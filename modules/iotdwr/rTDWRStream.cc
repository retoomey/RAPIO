/**
 * @file rTDWRStream.cc
 * @brief Standalone CLI application for TDWR streaming ingest.
 *
 * Replaces the legacy TDWR streaming logic of ldm2netcdf.
 * Ingests TDWR binary streams/files from standard input or a monitored directory,
 * accumulates sweeps into rapio::RadialSet objects, and exports NetCDF sweep products.
 */

#include "rTDWRStreamReader.h"
#include <rRAPIOOptions.h>
#include <rRAPIORuntime.h>
#include <rPluginNotifier.h>
#include <rFMLRecordNotifier.h>
#include <rConfigParamGroup.h>
#include <rError.h>
#include <csignal>
#include <iostream>
#include <atomic>
#include <unistd.h>

using namespace rapio;

namespace {
std::atomic<bool> g_stopRequested{ false };

void
handleSignal(int sig)
{
  if ((sig == SIGINT) || (sig == SIGTERM)) {
    fLogInfo("Signal {} received, shutting down rTDWRStream cleanly...", sig);
    g_stopRequested.store(true);
  }
}
} // namespace

int
main(int argc, char * argv[])
{
  RAPIORuntime::initialize();

  std::signal(SIGINT, handleSignal);
  std::signal(SIGTERM, handleSignal);

  RAPIOOptions opt("rTDWRStream");

  opt.setDescription(
    "rTDWRStream ingests TDWR binary data (0x2b00/0x2b01) from an LDM directory or stdin pipe and outputs NetCDF sweeps.");
  opt.setAuthors("RAPIO / MRMS Team");

  opt.optional("s", "", "Specify the source radar name (e.g. TOKC, TDFW).");
  opt.optional("i", "", "Specifies the input LDM data directory.");
  opt.optional("o", "",
    "Specifies the output NetCDF directory (or multiple space-separated directories; first compressed, subsequent uncompressed).");
  opt.optional("p", "", "Process files matching the given pattern/prefix.");
  opt.optional("v", "0", "Override VCP in data with this user supplied VCP (0 means auto-detect).");
  opt.optional("N", "1.0", "Noise threshold in dB for Low PRF reflectivity filtering (default: 1.0).");
  opt.optional("mag", "", "Magnetic declination offset in degrees or format e.g. '10W' or '7E'.");
  opt.optional("freq", "5603", "Radar transmitter frequency in MHz (default: 5603).");
  opt.optional("poll", "500", "Polling interval in milliseconds for directory monitoring.");
  opt.optional("A", "", "Alignment azimuthal resolution (legacy compatibility).");
  opt.optional("n", "", "Specify notification target (e.g. 'fml=/path/code_index.fam' or empty for default).");
  opt.optional("l", "", "Legacy alias for -n: Specify notification target.");

  opt.boolean("K", "Kill/delete LDM files after using them.");
  opt.boolean("a", "Read old LDM files in directory on startup.");
  opt.boolean("1", "Execute just once across existing files (archive mode).");
  opt.boolean("P", "Read from stdin (piped input stream).");
  opt.boolean("u", "Force write uncompressed NetCDF files.");
  opt.boolean("f", "Store all files flat in output directory rather than in subdirectories.");
  opt.boolean("V", "Mark all elevations with the volume time, not the elevation time.");

  try {
    const bool wantHelp = opt.processArgs(argc, argv);
    if (wantHelp) {
      opt.finalizeArgs(wantHelp);
      return 0;
    }
    opt.finalizeArgs(false);
  } catch (const std::exception& e) {
    std::cerr << "Option parsing error: " << e.what() << "\n";
    return 1;
  }

  TDWRStreamOptions opts;

  opts.radarName         = opt.getString("s");
  opts.inputDir          = opt.getString("i");
  opts.outputDir         = opt.getString("o");
  opts.pattern           = opt.getString("p");
  opts.userVCP           = opt.getInteger("v");
  opts.noiseThreshold    = static_cast<float>(opt.getDouble("N"));
  opts.magneticOffsetStr = opt.getString("mag");
  opts.frequencyMHz      = opt.getInteger("freq");
  opts.pollIntervalMs    = opt.getInteger("poll");

  std::string aStr = opt.getString("A");

  if (!aStr.empty()) {
    try {
      opts.alignmentResolution = std::stof(aStr);
    } catch (...) {
      opts.alignmentResolution = -1.0f;
    }
  }

  opts.killFiles          = opt.getBoolean("K");
  opts.readAll            = opt.getBoolean("a");
  opts.onceOnly           = opt.getBoolean("1");
  opts.pipedInput         = opt.getBoolean("P");
  opts.uncompressedNetcdf = opt.getBoolean("u");
  opts.flatDir            = opt.getBoolean("f");
  opts.useVolumeTime      = opt.getBoolean("V");

  std::string notifierArg = opt.getString("n");

  if (notifierArg.empty()) {
    notifierArg = opt.getString("l");
  }
  if (!notifierArg.empty()) {
    ConfigParamGroupn paramn;
    paramn.process(notifierArg);
    PluginNotifier::theNotifiers = RecordNotifier::createNotifiers();
  }

  if (!opts.pipedInput && opts.inputDir.empty()) {
    std::cerr << "Error: You must provide an input LDM directory with -i or piped stdin with -P.\n";
    return 1;
  }

  TDWRStreamReader reader(opts);

  reader.setSweepCallback([](const std::vector<std::shared_ptr<DataType> >& sweepOutputs) {
    if (sweepOutputs.empty()) {
      return;
    }
    for (const auto& item : sweepOutputs) {
      if (auto rs = std::dynamic_pointer_cast<RadialSet>(item)) {
        fLogInfo("Completed TDWR Sweep: Radar={} Elev={:.2f}deg Time={} Moments={}",
        rs->getRadarName(),
        rs->getElevationDegs(),
        rs->getTime().getString(),
        sweepOutputs.size());
        break;
      }
    }
  });

  if (opts.pipedInput) {
    reader.readFd(STDIN_FILENO);
  } else {
    reader.listenDirectory(&g_stopRequested);
  }

  return 0;
} // main
