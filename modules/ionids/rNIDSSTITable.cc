#include "rNIDSSTITable.h"

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
NIDSSTITable::introduceSelf(IONIDS * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<NIDSSTITable>();

  owner->introduce("OrpgStormTrackInfo", io);
  owner->introduce("StormTrackInfo", io);
}

std::shared_ptr<DataType>
NIDSSTITable::readNIDS(
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
    (info.isValid() && !info.getType().empty()) ? info.getType() : "StormTrackInfo";

  const std::vector<std::string> lines =
    NIDSHailIndexTable::extractGraphicAlphanumericLines(s, z);

  return decodeSTILines(lines, d.getLocation(), d.getVolumeStartTime(), typeName);
}

bool
NIDSSTITable::writeNIDS(
  IOConfig                  & /*config*/,
  std::shared_ptr<DataType> /*dt*/,
  StreamBuffer              & /*z*/)
{
  return false;
}

std::shared_ptr<DataTable>
NIDSSTITable::decodeSTILines(
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
    { "StormId",            DataColumn::Type::String  },
    { "Azimuth",            DataColumn::Type::Float   },
    { "Range",              DataColumn::Type::Float   },
    { "Dir",                DataColumn::Type::Float   },
    { "Speed",              DataColumn::Type::Float   },
    { "NewStorm",           DataColumn::Type::String  },
    { "FcstErr",            DataColumn::Type::Float   },
    { "MeanFcstErr",        DataColumn::Type::Float   },
    { "MaxRefl",            DataColumn::Type::Float   },
    { "MxRefHt",            DataColumn::Type::Float   },
    { "AlgRank",            DataColumn::Type::Integer },
    { Constants::Latitude,  DataColumn::Type::Float   },
    { Constants::Longitude, DataColumn::Type::Float   }
  };

  table->setSchema(schema);

  std::vector<std::string> stormIdList;
  std::vector<std::string> azList;
  std::vector<std::string> rangeList;
  std::vector<std::string> dirList;
  std::vector<std::string> speedList;
  std::vector<std::string> errorList;
  std::vector<std::string> meanErrorList;
  std::vector<std::string> dbzList;
  std::vector<std::string> dbzhtList;

  int columns = 0;

  for (size_t i = 0; i < lines.size(); ++i) {
    const int len = static_cast<int>(lines[i].length());
    int index     = 0;

    switch (i % 5) {
        case 0: {
          columns = 0;
          index   = 11; // Skip "STORM ID"
          while (index < len) {
            std::string temp = NIDSHailIndexTable::getSubstr(
              lines[i], ALPHABETS, " ", index, len - 1);
            if (!temp.empty()) {
              stormIdList.push_back(temp);
              ++columns;
            }
          }
          break;
        }

        case 1: {
          index = 7; // Skip "AZ/RAN"
          for (int k = 0; k < columns; ++k) {
            azList.push_back(NIDSHailIndexTable::getSubstr(
                lines[i], DIGITS, "/", index, len - 1));
            rangeList.push_back(NIDSHailIndexTable::getSubstr(
                lines[i], DIGITS, " ", index, len - 1));
          }
          break;
        }

        case 2: {
          index = 10; // Skip "FCST MVT"
          for (int k = 0; k < columns; ++k) {
            std::string tempStr = NIDSHailIndexTable::getSubstr(
              lines[i], ABC_AND_DIGITS, "/ ", index, len - 1);
            if (tempStr.find("NEW") == std::string::npos) {
              dirList.push_back(tempStr);
              speedList.push_back(NIDSHailIndexTable::getSubstr(
                  lines[i], ABC_AND_DIGITS, " ", index, len - 1));
            } else {
              dirList.push_back("NEW");
              speedList.push_back("NEW");
            }
          }
          break;
        }

        case 3: {
          index = 10; // Skip "ERR/MEAN"
          for (int k = 0; k < columns; ++k) {
            errorList.push_back(NIDSHailIndexTable::getSubstr(
                lines[i], DIGITS, "/", index, len - 1));
            meanErrorList.push_back(NIDSHailIndexTable::getSubstr(
                lines[i], DIGITS, " ", index, len - 1));
          }
          break;
        }

        case 4: {
          index = 9; // Skip "DBZM HGT"
          for (int k = 0; k < columns; ++k) {
            dbzList.push_back(NIDSHailIndexTable::getSubstr(
                lines[i], DIGITS, " ", index, len - 1));
            dbzhtList.push_back(NIDSHailIndexTable::getSubstr(
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
    azList.size(),
    rangeList.size(),
    dirList.size(),
    speedList.size(),
    errorList.size(),
    meanErrorList.size(),
    dbzList.size(),
    dbzhtList.size()
  });

  for (size_t j = 0; j < totalRows; ++j) {
    const float azDegs  = safeParseFloat(azList[j]);
    const float rangeNM = safeParseFloat(rangeList[j]);

    float dirVal   = static_cast<float>(Constants::MissingData);
    float speedVal = static_cast<float>(Constants::MissingData);
    std::string newStorm;

    if (dirList[j].find("NEW") != std::string::npos) {
      newStorm = "NEW";
    } else {
      dirVal   = safeParseFloat(dirList[j]);
      speedVal = safeParseFloat(speedList[j]);
    }

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
    row["StormId"]            = stormIdList[j];
    row["Azimuth"]            = azDegs;
    row["Range"]              = rangeNM;
    row["Dir"]                = dirVal;
    row["Speed"]              = speedVal;
    row["NewStorm"]           = newStorm;
    row["FcstErr"]            = safeParseFloat(errorList[j]);
    row["MeanFcstErr"]        = safeParseFloat(meanErrorList[j]);
    row["MaxRefl"]            = safeParseFloat(dbzList[j]);
    row["MxRefHt"]            = safeParseFloat(dbzhtList[j]);
    row["AlgRank"]            = static_cast<int>(j);
    row[Constants::Latitude]  = eventLat;
    row[Constants::Longitude] = eventLon;

    table->addRow(row);
  }

  return table;
} // NIDSSTITable::decodeSTILines
