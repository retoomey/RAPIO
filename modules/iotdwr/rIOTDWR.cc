/**
 * @file rIOTDWR.cc
 * @brief Dynamic RAPIO IODataType plugin implementation for TDWR.
 */

#include "rIOTDWR.h"
#include "rTDWRStreamReader.h"
#include <rMultiDataType.h>
#include <rIOURL.h>
#include <filesystem>

using namespace rapio;

extern "C" {
void *
createRAPIOIO(void)
{
  auto * z = new IOTDWR();

  z->initialize();
  return reinterpret_cast<void *>(z);
}
}

IOTDWR::IOTDWR()
{ }

IOTDWR::~IOTDWR()
{ }

std::string
IOTDWR::getHelpString(const std::string&)
{
  return "Reader for Terminal Doppler Weather Radar (TDWR) binary files and streams.\n"
         "Supports Message 0x2b00 (Compressed Base Data) and Message 0x2b01 (Low PRF Base Data).\n"
         "Options:\n"
         "  stream=true       Run streaming listener on directory or stdin\n"
         "  radar=<name>      Specify radar identifier (e.g. TOKC)\n"
         "  noise=<float>     Noise threshold in dB for Low PRF (default: 1.0)\n"
         "  mag=<string>      Magnetic declination offset (e.g. 10W, 7E)\n"
         "  freq=<int>        Transmitter frequency in MHz (default: 5603)";
}

void
IOTDWR::initialize()
{ }

std::shared_ptr<DataType>
IOTDWR::createDataType(IOConfig& config)
{
  URL url(config.getParamURL());
  std::string path = url.getPath();

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

  auto getInt = [&config](const std::string& k1, const std::string& k2, int def) {
      std::string v = config.get(k1, config.get(k2, ""));

      if (!v.empty()) {
        try { return std::stoi(v);
        } catch (...) { }
      }
      return def;
    };

  auto getFloat = [&config](const std::string& k1, const std::string& k2, float def) {
      std::string v = config.get(k1, config.get(k2, ""));

      if (!v.empty()) {
        try { return std::stof(v);
        } catch (...) { }
      }
      return def;
    };

  TDWRStreamOptions opts;

  opts.radarName          = config.get("radar", config.get("site", ""));
  opts.outputDir          = config.get("output", config.get("outputdir", ""));
  opts.pattern            = config.get("pattern", "");
  opts.userVCP            = getInt("vcp", "v", 0);
  opts.useVolumeTime      = (config.get("volumetime", "false") == "true");
  opts.noiseThreshold     = getFloat("noise", "N", 1.0f);
  opts.magneticOffsetStr  = config.get("mag", config.get("magnetic", ""));
  opts.frequencyMHz       = getInt("freq", "frequency", 5603);
  opts.flatDir            = (config.get("flat", "false") == "true");
  opts.uncompressedNetcdf = (config.get("uncompressed", "false") == "true");

  if (isStream) {
    opts.inputDir = path;
    TDWRStreamReader reader(opts);
    reader.listenDirectory();
    return nullptr;
  }

  // Static file mode
  TDWRStreamReader reader(opts);

  reader.getAccumulator().setKeepCompletedRadialSets(true);
  if (!reader.readFile(path)) {
    return nullptr;
  }

  const auto& sets = reader.getAccumulator().getRadialSets();

  if (sets.empty()) {
    return nullptr;
  }

  auto multi = MultiDataType::Create();

  for (const auto& rs : sets) {
    multi->addDataType(rs);
  }
  return MultiDataType::Simplify(multi);
} // IOTDWR::createDataType

std::shared_ptr<DataType>
IOTDWR::createDataTypeFromBuffer(std::vector<char>& buffer)
{
  if (buffer.empty()) {
    return nullptr;
  }

  TDWRStreamOptions opts;
  TDWRStreamReader reader(opts);

  reader.getAccumulator().setKeepCompletedRadialSets(true);
  reader.pushBytes(buffer.data(), buffer.size());
  reader.flush();

  const auto& sets = reader.getAccumulator().getRadialSets();

  if (sets.empty()) {
    return nullptr;
  }

  auto multi = MultiDataType::Create();

  for (const auto& rs : sets) {
    multi->addDataType(rs);
  }
  return MultiDataType::Simplify(multi);
}
