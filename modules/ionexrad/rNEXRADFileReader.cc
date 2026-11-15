#include "rNEXRADFileReader.h"
#include <rIOURL.h>
#include <rError.h>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace rapio {
NEXRADFileReader::NEXRADFileReader()
{ }

NEXRADFileReader::~NEXRADFileReader()
{ }

std::shared_ptr<DataType>
NEXRADFileReader::readFile(const std::string& filename, IOConfig& config)
{
  URL url(filename);
  std::vector<char> buf;

  if ((IOURL::read(url, buf) <= 0) || buf.empty()) {
    fLogSevere("NEXRADFileReader: Failed to read file '{}'", filename);
    return nullptr;
  }
  return readFromBuffer(buf, config);
}

std::shared_ptr<DataType>
NEXRADFileReader::readFromBuffer(const std::vector<char>& buffer, IOConfig& config)
{
  if (buffer.size() < NEXRADVolumeHeader::size()) {
    fLogSevere("NEXRADFileReader: Buffer too small for Level II volume header (size: {})", buffer.size());
    return nullptr;
  }

  NEXRADDecoder decoder;

  if (!parseBuffer(buffer.data(), buffer.size(), decoder, config)) {
    fLogSevere("NEXRADFileReader: Error parsing Level II buffer");
    return nullptr;
  }

  const auto& radialSets = decoder.getRadialSets();

  if (radialSets.empty()) {
    fLogInfo("NEXRADFileReader: No valid RadialSets extracted from Level II data");
    return nullptr;
  }

  // Check for optional filters in IOConfig
  std::string filterMoment    = config.get("moment", config.get("type", ""));
  std::string filterElevation = config.get("elevation", config.get("tilt", ""));

  auto multi = MultiDataType::Create();

  for (const auto& rs : radialSets) {
    if (!filterMoment.empty() && (rs->getTypeName() != filterMoment)) {
      continue;
    }
    if (!filterElevation.empty()) {
      std::string sub = rs->getSubType();
      if (sub.find(filterElevation) == std::string::npos) {
        continue;
      }
    }
    multi->addDataType(rs);
  }

  return MultiDataType::Simplify(multi);
} // NEXRADFileReader::readFromBuffer

bool
NEXRADFileReader::parseBuffer(const char * data, size_t datalen, NEXRADDecoder& decoder, const IOConfig& config)
{
  decoder.setKeepCompletedRadialSets(true);

  auto getBool = [&config](const std::string& k1, const std::string& k2 = "") {
      std::string v1 = config.get(k1, "");
      std::string v2 = k2.empty() ? "" : config.get(k2, "");

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

  std::string overrideRadar = config.get("radar", config.get("s", ""));

  if (!overrideRadar.empty()) {
    decoder.setRadarName(overrideRadar);
  }

  int userVCP = getInt("vcp", "v", 0);

  if (userVCP > 0) {
    decoder.setUserVCP(userVCP);
  }

  if (getBool("useVolumeTime", "V")) {
    decoder.setUseVolumeTime(true);
  }

  if (getBool("separateSAILS", "e")) {
    decoder.setSeparateSAILS(true);
  }

  bool addLower = (userVCP > 0) ? getBool("addLower") : true;

  decoder.setAddLower(addLower);

  if (getBool("headerMissing", "H")) {
    decoder.setHeaderMissing(true);
  }

  decoder.processBuffer(data, datalen);
  decoder.flush();

  return !decoder.getRadialSets().empty();
} // NEXRADFileReader::parseBuffer

bool
NEXRADFileReader::parseBuffer(const char * data, size_t datalen, NEXRADAccumulator& accumulator)
{
  IOConfig cfg;

  if (!accumulator.getRadarName().empty()) {
    cfg.set("radar", accumulator.getRadarName());
  }
  NEXRADDecoder decoder;

  return parseBuffer(data, datalen, decoder, cfg);
} // NEXRADFileReader::parseBuffer
} // namespace rapio
