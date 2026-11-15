#include "rNIDSMesoDetectionTable.h"

#include "rConfigNIDSInfo.h"
#include "rNIDSHailIndexTable.h"

#include <rConstants.h>
#include <rError.h>
#include <rProject.h>

#include <algorithm>
#include <cstdlib>
#include <map>

using namespace rapio;

namespace {
constexpr float NAUTICAL_MILES_TO_KM = 1.852f;

float
safeParseFloat(const std::string & s, float fallback = 0.0f)
{
  if (s.empty()) {
    return fallback;
  }
  char * endPtr = nullptr;
  float val     = std::strtof(s.c_str(), &endPtr);

  if (endPtr == s.c_str()) {
    return fallback;
  }
  return val;
}

std::string
trimSubstr(const std::string & s, size_t pos, size_t count)
{
  if (pos >= s.size()) {
    return { };
  }
  const std::string sub = s.substr(pos, count);
  const size_t first    = sub.find_first_not_of(" \t\r\n");

  if (first == std::string::npos) {
    return { };
  }
  const size_t last = sub.find_last_not_of(" \t\r\n");

  return sub.substr(first, last - first + 1);
}
} // namespace

void
NIDSMesoDetectionTable::introduceSelf(IONIDS * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<NIDSMesoDetectionTable>();

  owner->introduce("OrpgMesoDetection", io);
  owner->introduce("OrpgMesoDetectionTable", io);
}

std::shared_ptr<DataType>
NIDSMesoDetectionTable::readNIDS(
  IOConfig              & /*config*/,
  BlockMessageHeader    & /*h*/,
  BlockProductDesc      & d,
  BlockProductSymbology & s,
  StreamBuffer          & z)
{
  const int productCode = d.getProductCode();
  const auto info       = ConfigNIDSInfo::getNIDSInfo(productCode);

  if (info.isNullProduct()) {
    fLogSevere("Null product not handled, product number: {}", productCode);
    return nullptr;
  }

  const std::string typeName =
    (info.isValid() && !info.getType().empty()) ? info.getType() : "OrpgMesoDetectionTable";

  const std::vector<std::string> lines =
    NIDSHailIndexTable::extractGraphicAlphanumericLines(s, z);

  return decodeMesoDetectionLines(lines, d.getLocation(), d.getVolumeStartTime(), typeName);
}

bool
NIDSMesoDetectionTable::writeNIDS(
  IOConfig                  & /*config*/,
  std::shared_ptr<DataType> /*dt*/,
  StreamBuffer              & /*z*/)
{
  return false;
}

std::shared_ptr<DataTable>
NIDSMesoDetectionTable::decodeMesoDetectionLines(
  const std::vector<std::string> & lines,
  const LLH                      & radarLoc,
  const Time                     & eventTime,
  const std::string              & typeName)
{
  auto table = std::make_shared<DataTable>();

  table->setTypeName(typeName);
  table->setLocation(radarLoc);
  table->setTime(eventTime);
  table->setDataAttributeValue(Constants::ExpiryInterval, "60", "Minutes");

  const std::vector<std::pair<std::string, DataColumn::Type> > schema = {
    { "CircId",             DataColumn::Type::String },
    { "StormId",            DataColumn::Type::String },
    { "StrengthRank",       DataColumn::Type::String },
    { "LLRV",               DataColumn::Type::Float  },
    { "Azimuth",            DataColumn::Type::Float  },
    { "Range",              DataColumn::Type::Float  },
    { "HgtMaxRV",           DataColumn::Type::Float  },
    { "MaxRV",              DataColumn::Type::Float  },
    { "Base",               DataColumn::Type::Float  },
    { "Depth",              DataColumn::Type::Float  },
    { Constants::Latitude,  DataColumn::Type::Float  },
    { Constants::Longitude, DataColumn::Type::Float  }
  };

  table->setSchema(schema);

  if (lines.empty() || (lines[0].find("NO MESOS") != std::string::npos)) {
    return table;
  }

  std::vector<std::string> circIdList;
  std::vector<std::string> stormIdList;
  std::vector<std::string> strengthRankList;
  std::vector<std::string> llRvList;
  std::vector<std::string> azList;
  std::vector<std::string> rangeList;
  std::vector<std::string> hgtMaxRvList;
  std::vector<std::string> maxRvList;
  std::vector<std::string> baseList;
  std::vector<std::string> depthList;

  for (size_t i = 0; i < lines.size(); ++i) {
    const size_t row = i % 5;
    const size_t len = lines[i].length();

    size_t index = 11; // Skip past headings
    while (index < len) {
      const std::string tempStr1 = trimSubstr(lines[i], index, 4);
      index += 6;
      const std::string tempStr2 = trimSubstr(lines[i], index, 3);
      index += 4;

      if (tempStr1.empty() && tempStr2.empty()) {
        continue;
      }

      switch (row) {
          case 0: // Circ ID and Storm ID
            circIdList.push_back(tempStr1);
            stormIdList.push_back(tempStr2);
            break;

          case 1: // Strength rank and low-level rotational velocity
            strengthRankList.push_back(tempStr1);
            llRvList.push_back(tempStr2);
            break;

          case 2: // Azimuth and range
            azList.push_back(tempStr1);
            rangeList.push_back(tempStr2);
            break;

          case 3: // Height of max RV and max RV
            hgtMaxRvList.push_back(tempStr1);
            maxRvList.push_back(tempStr2);
            break;

          case 4: // Base and depth
            baseList.push_back(tempStr1);
            depthList.push_back(tempStr2);
            break;

          default:
            break;
      }
    }
  }

  const size_t totalRows = std::min({
    circIdList.size(),
    stormIdList.size(),
    strengthRankList.size(),
    llRvList.size(),
    azList.size(),
    rangeList.size(),
    hgtMaxRvList.size(),
    maxRvList.size(),
    baseList.size(),
    depthList.size()
  });

  for (size_t j = 0; j < totalRows; ++j) {
    const float azDegs  = safeParseFloat(azList[j]);
    const float rangeNM = safeParseFloat(rangeList[j]);

    LengthKMs heightKMs = 0.0f;
    AngleDegs eventLat  = 0.0f;
    AngleDegs eventLon  = 0.0f;

    Project::BeamPath_AzRangeToLatLon(
      static_cast<AngleDegs>(radarLoc.getLatitudeDeg()),
      static_cast<AngleDegs>(radarLoc.getLongitudeDeg()),
      azDegs,
      rangeNM * NAUTICAL_MILES_TO_KM,
      0.0f,
      heightKMs,
      eventLat,
      eventLon);

    std::map<std::string, DataValue> row;
    row["CircId"]             = circIdList[j];
    row["StormId"]            = stormIdList[j];
    row["StrengthRank"]       = strengthRankList[j];
    row["LLRV"]               = safeParseFloat(llRvList[j]);
    row["Azimuth"]            = azDegs;
    row["Range"]              = rangeNM;
    row["HgtMaxRV"]           = safeParseFloat(hgtMaxRvList[j]);
    row["MaxRV"]              = safeParseFloat(maxRvList[j]);
    row["Base"]               = safeParseFloat(baseList[j]);
    row["Depth"]              = safeParseFloat(depthList[j]);
    row[Constants::Latitude]  = eventLat;
    row[Constants::Longitude] = eventLon;

    table->addRow(row);
  }

  return table;
} // NIDSMesoDetectionTable::decodeMesoDetectionLines
