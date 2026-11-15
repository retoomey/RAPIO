#include "rNIDSMesoTable.h"

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
constexpr const char * ALPHABETS      = "?ABCDEFGHIJKLMNOPQRSTUVWXYZ";
constexpr const char * DIGITS         = "0123456789";
constexpr const char * ABC_AND_DIGITS = "?ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

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
} // namespace

void
NIDSMesoTable::introduceSelf(IONIDS * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<NIDSMesoTable>();

  owner->introduce("OrpgMeso", io);
  owner->introduce("OrpgMesoTable", io);
}

std::shared_ptr<DataType>
NIDSMesoTable::readNIDS(
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
    (info.isValid() && !info.getType().empty()) ? info.getType() : "OrpgMesoTable";

  const std::vector<std::string> lines =
    NIDSHailIndexTable::extractGraphicAlphanumericLines(s, z);

  return decodeMesoLines(lines, d.getLocation(), d.getVolumeStartTime(), typeName);
}

bool
NIDSMesoTable::writeNIDS(
  IOConfig                  & /*config*/,
  std::shared_ptr<DataType> /*dt*/,
  StreamBuffer              & /*z*/)
{
  return false;
}

std::shared_ptr<DataTable>
NIDSMesoTable::decodeMesoLines(
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
    { "StormId",            DataColumn::Type::String },
    { "Azimuth",            DataColumn::Type::Float  },
    { "Range",              DataColumn::Type::Float  },
    { "Type",               DataColumn::Type::String },
    { "Base",               DataColumn::Type::Float  },
    { "Top",                DataColumn::Type::Float  },
    { "RdlDiam",            DataColumn::Type::Float  },
    { "AzDiam",             DataColumn::Type::Float  },
    { Constants::Latitude,  DataColumn::Type::Float  },
    { Constants::Longitude, DataColumn::Type::Float  }
  };

  table->setSchema(schema);

  if (lines.empty() || (lines[0].find("NO MESOS") != std::string::npos)) {
    return table;
  }

  std::vector<std::string> stormIdList;
  std::vector<std::string> featureList;
  std::vector<std::string> azList;
  std::vector<std::string> rangeList;
  std::vector<std::string> baseList;
  std::vector<std::string> topList;
  std::vector<std::string> radList;
  std::vector<std::string> azdiaList;

  int columns   = 0;
  bool skipNeed = true;

  for (size_t i = 0; i < lines.size(); ++i) {
    const int len = static_cast<int>(lines[i].length());
    int index     = 0;

    switch (i % 5) {
        case 0: {
          columns = 0;
          while (index < len) {
            std::string temp = NIDSHailIndexTable::getSubstr(
              lines[i], ALPHABETS, " ", index, len - 1);
            if (!temp.empty()) {
              if (temp.find("STORM") != std::string::npos) {
                skipNeed = true; // skip "ID"
                index    = 11;
                temp     = NIDSHailIndexTable::getSubstr(
                  lines[i], ALPHABETS, " ", index, len - 1);
                if (!temp.empty()) {
                  stormIdList.push_back(temp);
                  ++columns;
                }
              } else {
                if (columns == 0) {
                  skipNeed = false;
                }
                stormIdList.push_back(temp);
                ++columns;
              }
            }
          }
          break;
        }

        case 1: {
          if (skipNeed) {
            index = 10; // skip "FEATURE"
          }
          for (int k = 0; k < columns; ++k) {
            std::string type = NIDSHailIndexTable::getSubstr(
              lines[i], ABC_AND_DIGITS, " ", index, len - 1);
            if ((type.find("DC") != std::string::npos) ||
              (type.find("UNC") != std::string::npos))
            {
              std::string type2 = NIDSHailIndexTable::getSubstr(
                lines[i], ALPHABETS, " ", index, len - 1);
              featureList.push_back(type + type2);
            } else {
              featureList.push_back(type);
            }
          }
          break;
        }

        case 2: {
          if (skipNeed) {
            index = 11; // skip "AZ RAN"
          }
          for (int k = 0; k < columns; ++k) {
            azList.push_back(NIDSHailIndexTable::getSubstr(
                lines[i], DIGITS, " ", index, len - 1));
            rangeList.push_back(NIDSHailIndexTable::getSubstr(
                lines[i], DIGITS, " ", index, len - 1));
          }
          break;
        }

        case 3: {
          if (skipNeed) {
            index = 11; // skip "BASE TOP"
          }
          for (int k = 0; k < columns; ++k) {
            baseList.push_back(NIDSHailIndexTable::getSubstr(
                lines[i], DIGITS, " ", index, len - 1));
            topList.push_back(NIDSHailIndexTable::getSubstr(
                lines[i], DIGITS, " ", index, len - 1));
          }
          break;
        }

        case 4: {
          if (skipNeed) {
            index = 11; // skip "RAD AZDIA"
          }
          for (int k = 0; k < columns; ++k) {
            radList.push_back(NIDSHailIndexTable::getSubstr(
                lines[i], DIGITS, " ", index, len - 1));
            azdiaList.push_back(NIDSHailIndexTable::getSubstr(
                lines[i], DIGITS, " ", index, len - 1));
          }
          break;
        }

        default:
          break;
    }
  }

  const size_t totalRows = std::min({
    stormIdList.size(),
    featureList.size(),
    azList.size(),
    rangeList.size(),
    baseList.size(),
    topList.size(),
    radList.size(),
    azdiaList.size()
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
    row["StormId"] = stormIdList[j];
    row["Azimuth"] = azDegs;
    row["Range"]   = rangeNM;
    row["Type"]    = featureList[j];
    row["Base"]    = safeParseFloat(baseList[j]);
    row["Top"]     = safeParseFloat(topList[j]);
    row["RdlDiam"] = safeParseFloat(radList[j]);
    row["AzDiam"]  = safeParseFloat(azdiaList[j]);
    row[Constants::Latitude]  = eventLat;
    row[Constants::Longitude] = eventLon;

    table->addRow(row);
  }

  return table;
} // NIDSMesoTable::decodeMesoLines
