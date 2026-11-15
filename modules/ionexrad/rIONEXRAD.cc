#include "rIONEXRAD.h"
#include "rNEXRADFileReader.h"
#include "rNEXRADStreamReader.h"
#include <rIOURL.h>
#include <filesystem>

using namespace rapio;

extern "C" {
void *
createRAPIOIO(void)
{
  auto * z = new IONEXRAD();

  z->initialize();
  return reinterpret_cast<void *>(z);
}
}

IONEXRAD::IONEXRAD()
{ }

IONEXRAD::~IONEXRAD()
{ }

std::string
IONEXRAD::getHelpString(const std::string&)
{
  return "Reader for NEXRAD Archive Level II (.ar2v) files supporting two operational modes:\n"
         "  1. Static File I/O: Reads static .ar2v files from disk or buffer.\n"
         "  2. Streaming I/O: Replicates legacy ldm2netcdf streaming, listening to an LDM directory,\n"
         "     pipe, or stream and outputting NetCDF sweeps in real-time.";
}

void
IONEXRAD::initialize()
{ }

std::shared_ptr<DataType>
IONEXRAD::createDataType(IOConfig& config)
{
  URL url(config.getParamURL());
  std::string path = url.getPath();

  if (path.empty()) {
    path = config.get("path", config.get("file", ""));
    url  = URL(path);
  }

  // If path is a directory or streaming mode requested, use streaming reader
  bool isDir = false;
  std::error_code ec;

  if (!path.empty() && std::filesystem::is_directory(path, ec)) {
    isDir = true;
  }

  bool isStream = (isDir ||
    config.get("stream", "false") == "true" ||
    config.get("stream", "") == "1" ||
    config.get("mode", "") == "stream" ||
    config.has("stream"));

  if (isStream) {
    auto getBool = [&config](const std::string& k1, const std::string& k2) {
        std::string v1 = config.get(k1, "");
        std::string v2 = config.get(k2, "");

        return (v1 == "true" || v1 == "1" || v2 == "true" || v2 == "1");
      };

    auto getInt = [&config](const std::string& k1, const std::string& k2, int def) {
        std::string v = config.get(k1, config.get(k2, ""));

        if (!v.empty()) {
          try { return std::stoi(v);
          } catch (...) { }
        }
        return def;
      };

    NEXRADStreamOptions opts;
    opts.inputDir           = path;
    opts.outputDir          = config.get("output", config.get("outputdir", ""));
    opts.radarName          = config.get("radar", config.get("s", ""));
    opts.pattern            = config.get("pattern", config.get("p", ""));
    opts.killFiles          = getBool("kill", "K");
    opts.readAll            = getBool("readAll", "a");
    opts.onceOnly           = getBool("onceOnly", "1");
    opts.userVCP            = getInt("vcp", "v", 0);
    opts.useVolumeTime      = getBool("useVolumeTime", "V");
    opts.flatDir            = getBool("flat", "f");
    opts.uncompressedNetcdf = getBool("uncompressed", "u");
    opts.separateSAILS      = getBool("separateSAILS", "e");
    opts.addLower           = (opts.userVCP > 0) ? getBool("addLower", "addLower") : true;

    NEXRADStreamReader streamReader(opts);
    streamReader.listenDirectory();
    return nullptr;
  }

  NEXRADFileReader reader;

  return reader.readFile(url.toString(), config);
} // IONEXRAD::createDataType

std::shared_ptr<DataType>
IONEXRAD::createDataTypeFromBuffer(std::vector<char>& buffer)
{
  IOConfig config;
  NEXRADFileReader reader;

  return reader.readFromBuffer(buffer, config);
}
