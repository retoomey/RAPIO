/**
 * @file rL2Stream.cc
 * @brief Standalone CLI application for Level II / TDWR streaming ingest.
 *
 * rL2Stream serves as the modern RAPIO replacement for legacy MRMS ldm2netcdf.
 * Acts as the architectural glue:
 * - Instantiates rLDMStreamReader for transport chunking, directory monitoring, and decompression.
 * - Instantiates pure data decoders (NEXRADDecoder or TDWRDecoder) selected by flag or auto-detection.
 * - Wires rLDMStreamReader's output callback directly to the chosen decoder's processBuffer().
 */

#include <rLDMStreamReader.h>
#include "rNEXRADDecoder.h"
#include "rTDWRDecoder.h"
#include <rRadialSet.h>
#include <rRAPIOOptions.h>
#include <rRAPIORuntime.h>
#include <rConfigRadarInfo.h>
#include <rPluginNotifier.h>
#include <rFMLRecordNotifier.h>
#include <rConfigParamGroup.h>
#include <rStrings.h>
#include <rError.h>
#include <csignal>
#include <iostream>
#include <memory>
#include <atomic>
#include <unistd.h>

using namespace rapio;

namespace {
std::atomic<bool> g_stopRequested{ false };

void
handleSignal(int sig)
{
  if ((sig == SIGINT) || (sig == SIGTERM)) {
    fLogInfo("Signal {} received, shutting down rL2Stream cleanly...", sig);
    g_stopRequested.store(true);
  }
}

enum class StreamDecoderType {
  AUTO,
  NEXRAD,
  TDWR,
  IGNORED
};
} // namespace

int
main(int argc, char * argv[])
{
  // Initialize RAPIO runtime, configurations and dynamic module paths
  RAPIORuntime::initialize();

  std::signal(SIGINT, handleSignal);
  std::signal(SIGTERM, handleSignal);

  RAPIOOptions opt("rL2Stream");

  opt.setDescription(
    "rL2Stream ingests radar data (NEXRAD Level II or TDWR Base Data) from an LDM directory or stream and outputs NetCDF sweeps.");
  opt.setAuthors("RAPIO / MRMS Team");

  opt.optional("s", "", "Specify the source radar name (e.g. KTLX, KATX, TOKC).");
  opt.optional("i", "", "Specifies the input LDM data directory.");
  opt.optional("o", "", "Specifies the output NetCDF directory.");
  opt.optional("p", "", "Process files matching the given pattern/prefix.");
  opt.optional("v", "0", "Override VCP in data with this user supplied VCP (0 means agnostic).");
  opt.optional("poll", "500", "Polling interval in milliseconds for directory monitoring.");
  opt.optional("N", "1.0", "Noise threshold in dB for TDWR Low PRF reflectivity filtering (default: 1.0).");
  opt.optional("mag", "", "Magnetic declination offset for TDWR (e.g. '10W', '7E').");
  opt.optional("freq", "5603", "Radar transmitter frequency in MHz for TDWR (default: 5603).");
  opt.optional("n", "", "Specify notification target (e.g. 'fml=/path/code_index.fam' or empty for default).");
  opt.optional("l", "", "Legacy alias for -n: Specify notification target.");

  opt.boolean("nexrad", "Explicitly run in NEXRAD Level II mode.");
  opt.boolean("ar2v", "Explicitly run in NEXRAD Level II mode (alias).");
  opt.boolean("legacy", "Support legacy TDWR binary data streams.");
  opt.boolean("tdwr", "Run in TDWR streaming mode.");
  opt.boolean("MSG2", "Output Message2 RDA Status information.");
  opt.boolean("MSG3", "Output Message3 information.");
  opt.boolean("K", "Kill/delete LDM files after using them.");
  opt.boolean("a", "Read old LDM files in directory on startup.");
  opt.boolean("1", "Execute just once across existing files (archive mode).");
  opt.boolean("P", "Read from stdin (piped input stream).");
  opt.boolean("L", "LDM bzipped radial packets according to Nexrad2 LDM protocol.");
  opt.boolean("H", "Specifies that the files are Level II but the 24-byte volume header is missing.");
  opt.boolean("HVAN", "Output HVAzNoise information.");
  opt.boolean("u", "Force write uncompressed NetCDF files.");
  opt.boolean("f", "Store all files flat in output directory rather than in subdirectories.");
  opt.boolean("V", "Mark all elevations with the volume time, not the elevation time.");
  opt.boolean("e", "Separate out extra SAILS cuts with _EXTRA suffix.");
  opt.boolean("addLower", "Output tilts lower than 0.5 deg without _EXTRA suffix.");
  opt.boolean("b", "Enable writing secondary PAR base surveillance scans.");
  opt.boolean("t", "System clock time correction (legacy compatibility).");
  opt.optional("T", "", "Time shift (legacy compatibility).");
  opt.optional("A", "", "Alignment azimuthal resolution (legacy compatibility).");
  opt.boolean("dump", "Dump Level II header info.");
  opt.boolean("dumpall", "Dump all Level II message info.");

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

  std::string radarName = opt.getString("s");
  std::string inputDir  = opt.getString("i");
  std::string outputDir = opt.getString("o");
  std::vector<std::string> odirs;

  if (!outputDir.empty()) {
    Strings::splitWithoutEnds(outputDir, ' ', &odirs);
  }
  std::string pattern = opt.getString("p");
  int userVCP         = opt.getInteger("v");
  int pollIntervalMs  = opt.getInteger("poll");

  bool killFiles          = opt.getBoolean("K");
  bool readAll            = opt.getBoolean("a");
  bool onceOnly           = opt.getBoolean("1");
  bool pipedInput         = opt.getBoolean("P");
  bool uncompressedNetcdf = opt.getBoolean("u");
  bool flatDir            = opt.getBoolean("f");
  bool useVolumeTime      = opt.getBoolean("V");
  bool separateSAILS      = opt.getBoolean("e");
  bool addLower           = (userVCP > 0) ? opt.getBoolean("addLower") : true;
  bool writeMSG2          = opt.getBoolean("MSG2");
  bool writeMSG3          = opt.getBoolean("MSG3");
  bool ldmBzipped         = opt.getBoolean("L");
  bool headerMissing      = opt.getBoolean("H");
  bool writeHVAN          = opt.getBoolean("HVAN");

  float noiseThreshold = static_cast<float>(opt.getDouble("N"));
  std::string magneticOffsetStr = opt.getString("mag");
  int frequencyMHz = opt.getInteger("freq");

  float alignmentResolution = -1.0f;
  std::string aStr = opt.getString("A");

  if (!aStr.empty()) {
    try {
      alignmentResolution = std::stof(aStr);
    } catch (...) {
      alignmentResolution = -1.0f;
    }
  }

  std::string notifierArg = opt.getString("n");

  if (notifierArg.empty()) {
    notifierArg = opt.getString("l");
  }

  if (!notifierArg.empty()) {
    ConfigParamGroupn paramn;
    paramn.process(notifierArg);
    PluginNotifier::theNotifiers = RecordNotifier::createNotifiers();
  }

  if (!pipedInput && inputDir.empty()) {
    std::cerr << "Error: You must provide an input LDM directory with -i or piped stdin with -P.\n";
    return 1;
  }

  // Determine decoder selection: command-line flag, radar metadata, or auto-detection
  StreamDecoderType decoderType = StreamDecoderType::AUTO;

  if (opt.getBoolean("legacy") || opt.getBoolean("tdwr")) {
    decoderType = StreamDecoderType::TDWR;
  } else if (opt.getBoolean("nexrad") || opt.getBoolean("ar2v")) {
    decoderType = StreamDecoderType::NEXRAD;
  } else if (!radarName.empty()) {
    if (ConfigRadarInfo::haveRadar(radarName) && (ConfigRadarInfo::getFreqBand(radarName) == "C")) {
      decoderType = StreamDecoderType::TDWR;
    } else if (ConfigRadarInfo::haveRadar(radarName) && (ConfigRadarInfo::getFreqBand(radarName) == "S")) {
      decoderType = StreamDecoderType::NEXRAD;
    }
  }

  // Instantiate pure data decoders
  auto nexradDecoder = std::make_unique<NEXRADDecoder>();

  nexradDecoder->setRadarName(radarName);
  nexradDecoder->setUserVCP(userVCP);
  nexradDecoder->setUseVolumeTime(useVolumeTime);
  nexradDecoder->setSeparateSAILS(separateSAILS);
  nexradDecoder->setAddLower(addLower);
  nexradDecoder->setWriteMSG2(writeMSG2);
  nexradDecoder->setWriteMSG3(writeMSG3);
  nexradDecoder->setLdmBzipped(ldmBzipped);
  nexradDecoder->setHeaderMissing(headerMissing);
  nexradDecoder->setWriteHVAN(writeHVAN);
  nexradDecoder->setAlignmentResolution(alignmentResolution);
  nexradDecoder->setKeepCompletedRadialSets(false);

  nexradDecoder->setSweepCallback([&](const std::vector<std::shared_ptr<DataType> >& sweepOutputs) {
    if (sweepOutputs.empty()) { return; }
    if (!odirs.empty()) {
      if (odirs.size() == 1) {
        NEXRADDecoder::writeSweepToNetcdf(sweepOutputs, odirs[0], flatDir, uncompressedNetcdf, alignmentResolution);
      } else {
        for (size_t i = 0; i < odirs.size(); ++i) {
          bool uncompressed = (i != 0);
          NEXRADDecoder::writeSweepToNetcdf(sweepOutputs, odirs[i], flatDir, uncompressed, alignmentResolution);
        }
      }
    }
    for (const auto& item : sweepOutputs) {
      if (auto first = std::dynamic_pointer_cast<RadialSet>(item)) {
        fLogInfo("Completed Sweep: Radar={} Elev={:.2f}deg Time={} Moments={}",
        first->getRadarName(),
        first->getElevationDegs(),
        first->getTime().getString(),
        sweepOutputs.size());
        break;
      }
    }
  });

  auto tdwrDecoder = std::make_unique<TDWRDecoder>();

  tdwrDecoder->setRadarName(radarName);
  tdwrDecoder->setUserVCP(userVCP);
  tdwrDecoder->setUseVolumeTime(useVolumeTime);
  tdwrDecoder->setNoiseThreshold(noiseThreshold);
  tdwrDecoder->setMagneticOffset(magneticOffsetStr);
  tdwrDecoder->setFrequency(frequencyMHz);
  tdwrDecoder->setAlignmentResolution(alignmentResolution);
  tdwrDecoder->setKeepCompletedRadialSets(false);

  tdwrDecoder->setSweepCallback([&](const std::vector<std::shared_ptr<DataType> >& sweepOutputs) {
    if (sweepOutputs.empty()) { return; }
    if (!odirs.empty()) {
      if (odirs.size() == 1) {
        TDWRDecoder::writeSweepToNetcdf(sweepOutputs, odirs[0], flatDir, uncompressedNetcdf, alignmentResolution);
      } else {
        for (size_t i = 0; i < odirs.size(); ++i) {
          bool uncompressed = (i != 0);
          TDWRDecoder::writeSweepToNetcdf(sweepOutputs, odirs[i], flatDir, uncompressed, alignmentResolution);
        }
      }
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

  // Instantiate core transport reader
  rLDMStreamReader streamReader;
  LDMStreamOptions ldmOpts;

  ldmOpts.inputDir       = inputDir;
  ldmOpts.pattern        = pattern;
  ldmOpts.killFiles      = killFiles;
  ldmOpts.readAll        = readAll;
  ldmOpts.onceOnly       = onceOnly;
  ldmOpts.pipedInput     = pipedInput;
  ldmOpts.pollIntervalMs = pollIntervalMs;

  streamReader.setOptions(ldmOpts);

  const bool initialAutoMode = (decoderType == StreamDecoderType::AUTO);

  auto handleFileComplete = [&](const std::string& /*filePath*/) {
      if (decoderType == StreamDecoderType::TDWR) {
        tdwrDecoder->resetStreamState();
      } else {
        nexradDecoder->resetStreamState();
      }
      if (initialAutoMode) {
        decoderType = StreamDecoderType::AUTO;
      }
    };

  streamReader.setFileCompleteCallback([&](const std::string& filePath) {
    handleFileComplete(filePath);
  });

  // Wire transport reader output callback directly to the chosen or auto-detected decoder
  streamReader.setPayloadCallback([&](const std::vector<char>& payload) {
    if (payload.empty()) {
      return;
    }

    if (decoderType == StreamDecoderType::AUTO) {
      if ((payload.size() >= 2) && (static_cast<uint8_t>(payload[0]) == 0x2b) &&
      ((static_cast<uint8_t>(payload[1]) == 0x00) || (static_cast<uint8_t>(payload[1]) == 0x01) ))
      {
        fLogInfo("rL2Stream: Auto-detected TDWR payload (message ID 0x2b{:02x})", static_cast<uint8_t>(payload[1]));
        decoderType = StreamDecoderType::TDWR;
      } else if ((payload.size() >= 4) &&
      ((std::strncmp(payload.data(), "AR2V", 4) == 0) ||
      (std::strncmp(payload.data(), "ARCH", 4) == 0)))
      {
        fLogInfo("rL2Stream: Auto-detected NEXRAD Level II payload");
        decoderType = StreamDecoderType::NEXRAD;
      } else if ((payload.size() >= 3) &&
      (payload[0] == 'B') && (payload[1] == 'Z') && (payload[2] == 'h') )
      {
        fLogInfo("rL2Stream: Auto-detected NEXRAD Level II payload (BZIP2)");
        decoderType = StreamDecoderType::NEXRAD;
      } else {
        fLogInfo("rL2Stream: Unknown payload signature, passing to NEXRAD fallback while remaining in AUTO mode");
      }
    }

    if (decoderType == StreamDecoderType::TDWR) {
      tdwrDecoder->processBuffer(payload);
    } else {
      nexradDecoder->processBuffer(payload);
    }
  });

  if (decoderType == StreamDecoderType::TDWR) {
    fLogInfo("rL2Stream: Operating in TDWR stream ingest mode");
  } else if (decoderType == StreamDecoderType::NEXRAD) {
    fLogInfo("rL2Stream: Operating in NEXRAD Level II stream ingest mode");
  } else {
    fLogInfo("rL2Stream: Operating with payload auto-detection");
  }

  // Execute streaming
  if (pipedInput) {
    streamReader.readFd(STDIN_FILENO);
    handleFileComplete("<stdin>");
  } else {
    streamReader.listenDirectory(&g_stopRequested);
  }

  // Final flush
  if (decoderType == StreamDecoderType::TDWR) {
    tdwrDecoder->flush();
  } else if (decoderType == StreamDecoderType::NEXRAD) {
    nexradDecoder->flush();
  } else {
    tdwrDecoder->flush();
    nexradDecoder->flush();
  }

  fLogInfo("rL2Stream finished.");
  return 0;
} // main
