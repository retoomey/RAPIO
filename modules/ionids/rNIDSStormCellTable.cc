#include "rNIDSStormCellTable.h"

#include "rConfigNIDSInfo.h"
#include "rNIDSHailIndexTable.h"

#include <rConstants.h>
#include <rError.h>
#include <rProject.h>

#include <cstdlib>
#include <map>
#include <stdexcept>

using namespace rapio;

namespace {
constexpr const char * ALPHABETS      = "?ABCDEFGHIJKLMNOPQRSTUVWXYZ";
constexpr const char * DIGITS         = "0123456789";
constexpr const char * ABC_AND_DIGITS = "?ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
constexpr const char * SYM_DIGITS     = "><0123456789";
constexpr const char * ABC_SYM_DIG    = "?ABCDEFGHIJKLMNOPQRSTUVWXYZ><0123456789";

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
NIDSStormCellTable::introduceSelf(IONIDS * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<NIDSStormCellTable>();

  owner->introduce("OrpgStormCellTable", io);
  owner->introduce("StormCellTable", io);
}

std::shared_ptr<DataType>
NIDSStormCellTable::readNIDS(
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
    (info.isValid() && !info.getType().empty()) ? info.getType() : "StormCellTable";

  const std::vector<std::string> lines = extractStormCellLines(s, z);

  return decodeStormCellLines(lines, d.getLocation(), d.getVolumeStartTime(), typeName);
}

bool
NIDSStormCellTable::writeNIDS(
  IOConfig                  & /*config*/,
  std::shared_ptr<DataType> /*dt*/,
  StreamBuffer              & /*z*/)
{
  return false;
}

std::vector<std::string>
NIDSStormCellTable::extractStormCellLines(
  BlockProductSymbology & s,
  StreamBuffer          & z)
{
  std::vector<std::string> lines;
  short numPages = 0;

  if (s.myBlockID == 1) {
    if (s.myBlockLength > 10) {
      z.forward(static_cast<size_t>(s.myBlockLength - 10));
    }

    const short divider = z.readShort();
    if (divider != -1) {
      throw std::runtime_error("Graphic Alphanumeric Block doesn't start with divider -1");
    }

    const short blockID = z.readShort();
    if (blockID != 2) {
      fLogSevere("Expected Graphic Alphanumeric Block ID 2, got {}", blockID);
      return lines;
    }

    /* int blockLength = */ z.readInt();
    numPages = z.readShort();
  } else if (s.myBlockID == 2) {
    numPages = s.myNumLayers;
  } else {
    fLogSevere("Unexpected block ID {} while reading Graphic Alphanumeric Block", s.myBlockID);
    return lines;
  }

  for (short pageIdx = 0; pageIdx < numPages; ++pageIdx) {
    const short pageNumber = z.readShort();
    if ((pageNumber < 1) && (pageNumber > 48)) {
      throw std::runtime_error("Invalid page number in Graphic Alphanumeric Block");
    }

    const short pageLength = z.readShort();
    int bytesRead    = 0;
    int packet8Count = 0;

    while (bytesRead < pageLength) {
      const short packetCode        = z.readShort();
      const short lengthOfDataBlock = z.readShort();

      if (packetCode == 8) {
        /* short textColor = */ z.readShort();
        /* short iStart    = */ z.readShort();
        /* short jStart    = */ z.readShort();

        const int textLen = lengthOfDataBlock - 6;
        if (textLen > 0) {
          std::string text = z.readString(static_cast<size_t>(textLen));
          // Legacy OrpgStormCellTableFormatter skips Packet 8 index 0 (table header) on each page
          if (packet8Count > 0) {
            lines.push_back(std::move(text));
          }
        }
        ++packet8Count;
      } else {
        if (lengthOfDataBlock > 0) {
          z.forward(static_cast<size_t>(lengthOfDataBlock));
        }
      }

      bytesRead += lengthOfDataBlock + 4;
    }
  }

  return lines;
} // NIDSStormCellTable::extractStormCellLines

std::shared_ptr<DataTable>
NIDSStormCellTable::decodeStormCellLines(
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
    { "TVSType",            DataColumn::Type::String  },
    { "MesoType",           DataColumn::Type::String  },
    { "POH",                DataColumn::Type::Float   },
    { "POSH",               DataColumn::Type::Float   },
    { "MxHailSz",           DataColumn::Type::Float   },
    { "HailSzRange",        DataColumn::Type::String  },
    { "VIL",                DataColumn::Type::Float   },
    { "MaxRefl",            DataColumn::Type::Float   },
    { "HtMaxRefl",          DataColumn::Type::Float   },
    { "Top",                DataColumn::Type::Float   },
    { "TopFlag",            DataColumn::Type::String  },
    { "Dir",                DataColumn::Type::Float   },
    { "Speed",              DataColumn::Type::Float   },
    { "NewStorm",           DataColumn::Type::String  },
    { "AlgRank",            DataColumn::Type::Integer },
    { Constants::Latitude,  DataColumn::Type::Float   },
    { Constants::Longitude, DataColumn::Type::Float   }
  };

  table->setSchema(schema);

  const float missingVal = static_cast<float>(Constants::MissingData);

  for (size_t i = 0; i < lines.size(); ++i) {
    int idx = 0;
    const std::string & line = lines[i];

    const std::string id = NIDSHailIndexTable::getSubstr(line, ALPHABETS, " ", idx);
    if (id.empty()) {
      continue;
    }

    const std::string azStr = NIDSHailIndexTable::getSubstr(line, DIGITS, "/", idx);
    const std::string rgStr = NIDSHailIndexTable::getSubstr(line, DIGITS, " ", idx);
    const float azDegs      = safeParseFloat(azStr);
    const float rgNM        = safeParseFloat(rgStr);

    LengthKMs heightKMs = 0.0f;
    AngleDegs eventLat  = 0.0f;
    AngleDegs eventLon  = 0.0f;
    Project::BeamPath_AzRangeToLatLon(
      static_cast<AngleDegs>(radarLoc.getLatitudeDeg()),
      static_cast<AngleDegs>(radarLoc.getLongitudeDeg()),
      azDegs,
      rgNM * NAUTICAL_MILES_TO_KM,
      0.0f,
      heightKMs,
      eventLat,
      eventLon);

    const std::string tvsType  = NIDSHailIndexTable::getSubstr(line, ALPHABETS, " ", idx);
    const std::string mesoType = NIDSHailIndexTable::getSubstr(line, ABC_AND_DIGITS, " ", idx);

    float pohVal   = missingVal;
    float poshVal  = missingVal;
    float mxHailSz = missingVal;
    std::string hailSzRange;

    const std::string hailProbStr =
      NIDSHailIndexTable::getSubstr(line, ABC_AND_DIGITS, "/ ", idx);
    if (hailProbStr.find("UNKNOWN") == std::string::npos) {
      pohVal = safeParseFloat(hailProbStr, missingVal);

      const std::string severeHailStr =
        NIDSHailIndexTable::getSubstr(line, DIGITS, "/ ", idx);
      if (severeHailStr.find("UNKNOWN") == std::string::npos) {
        poshVal = safeParseFloat(severeHailStr, missingVal);
      }

      const std::string hs = NIDSHailIndexTable::getSubstr(line, ABC_SYM_DIG, " ", idx);
      if ((hs.find('>') != std::string::npos) || (hs.find('<') != std::string::npos)) {
        hailSzRange = hs;
      } else {
        mxHailSz = safeParseFloat(hs, missingVal);
      }
    } else {
      hailSzRange = "UNKNOWN";
    }

    const float vilVal       = safeParseFloat(NIDSHailIndexTable::getSubstr(line, DIGITS, " ", idx));
    const float maxReflVal   = safeParseFloat(NIDSHailIndexTable::getSubstr(line, DIGITS, " ", idx));
    const float htMaxReflVal = safeParseFloat(NIDSHailIndexTable::getSubstr(line, DIGITS, " ", idx));

    const std::string topStr = NIDSHailIndexTable::getSubstr(line, SYM_DIGITS, " ", idx);
    float topVal = 0.0f;
    std::string topFlag;
    const size_t gtPos = topStr.find('>');
    if (gtPos == std::string::npos) {
      topVal = safeParseFloat(topStr);
    } else {
      topVal  = safeParseFloat(topStr.substr(gtPos + 1));
      topFlag = "> Top";
    }

    const std::string dirStr =
      NIDSHailIndexTable::getSubstr(line, ABC_AND_DIGITS, "/ ", idx);
    float dirVal   = missingVal;
    float speedVal = missingVal;
    std::string newStorm;

    if (dirStr.find("NEW") != std::string::npos) {
      newStorm = "NEW";
    } else {
      dirVal   = safeParseFloat(dirStr, missingVal);
      speedVal = safeParseFloat(
        NIDSHailIndexTable::getSubstr(line, DIGITS, " ", idx), missingVal);
    }

    std::map<std::string, DataValue> row;
    row["StormId"]            = id;
    row["Azimuth"]            = azDegs;
    row["Range"]              = rgNM;
    row["TVSType"]            = tvsType;
    row["MesoType"]           = mesoType;
    row["POH"]                = pohVal;
    row["POSH"]               = poshVal;
    row["MxHailSz"]           = mxHailSz;
    row["HailSzRange"]        = hailSzRange;
    row["VIL"]                = vilVal;
    row["MaxRefl"]            = maxReflVal;
    row["HtMaxRefl"]          = htMaxReflVal;
    row["Top"]                = topVal;
    row["TopFlag"]            = topFlag;
    row["Dir"]                = dirVal;
    row["Speed"]              = speedVal;
    row["NewStorm"]           = newStorm;
    row["AlgRank"]            = static_cast<int>(i);
    row[Constants::Latitude]  = eventLat;
    row[Constants::Longitude] = eventLon;

    table->addRow(row);
  }

  return table;
} // NIDSStormCellTable::decodeStormCellLines
