#include "rNIDSTVSTable.h"

#include "rConfigNIDSInfo.h"

#include <rConstants.h>
#include <rError.h>
#include <rProject.h>

#include <algorithm>
#include <cstdlib>
#include <map>
#include <stdexcept>

using namespace rapio;

namespace {
constexpr const char * ALPHABETS      = "?ABCDEFGHIJKLMNOPQRSTUVWXYZ";
constexpr const char * DIGITS         = "0123456789";
constexpr const char * ABC_AND_DIGITS = "?ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
constexpr const char * SYM_DIGITS     = "><0123456789";

constexpr float NAUTICAL_MILES_TO_KM = 1.852f;
constexpr int ROWS_IN_HEADER         = 7;

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
NIDSTVSTable::introduceSelf(IONIDS * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<NIDSTVSTable>();

  owner->introduce("OrpgTVSTable", io);
  owner->introduce("TVS", io);
}

std::shared_ptr<DataType>
NIDSTVSTable::readNIDS(
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
    (info.isValid() && !info.getType().empty()) ? info.getType() : "TVS";

  const std::vector<std::string> lines = extractTabularAlphanumericLines(s, z);

  return decodeTVSLines(lines, d.getLocation(), d.getVolumeStartTime(), typeName);
}

bool
NIDSTVSTable::writeNIDS(
  IOConfig                  & /*config*/,
  std::shared_ptr<DataType> /*dt*/,
  StreamBuffer              & /*z*/)
{
  return false;
}

std::vector<std::string>
NIDSTVSTable::extractTabularAlphanumericLines(
  BlockProductSymbology & s,
  StreamBuffer          & z)
{
  std::vector<std::string> lines;

  // IONIDS::createDataType reads the first 10-byte block header into `s`.
  // Advance past any preceding Product Symbology (Block 1) and/or Graphic
  // Alphanumeric (Block 2) blocks to reach the Tabular Alphanumeric Block (Block 3).
  if (s.myBlockID == 1) {
    if (s.myBlockLength > 10) {
      z.forward(static_cast<size_t>(s.myBlockLength - 10));
    }

    short divider = z.readShort();
    if (divider != -1) {
      throw std::runtime_error("Expected block divider -1 after Product Symbology Block");
    }

    short blockID   = z.readShort();
    int blockLength = z.readInt();

    if (blockID == 2) {
      if (blockLength > 8) {
        z.forward(static_cast<size_t>(blockLength - 8));
      }

      divider = z.readShort();
      if (divider != -1) {
        throw std::runtime_error("Expected block divider -1 after Graphic Alphanumeric Block");
      }

      blockID     = z.readShort();
      blockLength = z.readInt();
    }

    if (blockID != 3) {
      fLogSevere("Expected Tabular Alphanumeric Block ID 3, got {}", blockID);
      return lines;
    }

    // Skip embedded MessageHeaderBlock (18 bytes) and ProductDescBlock (102 bytes).
    z.forward(120);
  } else if (s.myBlockID == 2) {
    if (s.myBlockLength > 10) {
      z.forward(static_cast<size_t>(s.myBlockLength - 10));
    }

    const short divider = z.readShort();
    if (divider != -1) {
      throw std::runtime_error("Expected block divider -1 after Graphic Alphanumeric Block");
    }

    const short blockID = z.readShort();
    /* int blockLength = */ z.readInt();

    if (blockID != 3) {
      fLogSevere("Expected Tabular Alphanumeric Block ID 3, got {}", blockID);
      return lines;
    }

    // Skip embedded MessageHeaderBlock (18 bytes) and ProductDescBlock (102 bytes).
    z.forward(120);
  } else if (s.myBlockID == 3) {
    // 10 bytes of the 128-byte Block 3 header were already consumed into `s`
    // (divider, blockID, blockLength, and the first 2 bytes of embedded MHB).
    z.forward(118);
  } else {
    fLogSevere("Unexpected block ID {} while reading Tabular Alphanumeric Block", s.myBlockID);
    return lines;
  }

  const short divider2 = z.readShort();

  if (divider2 != -1) {
    throw std::runtime_error("Tabular Alphanumeric Block page section doesn't start with divider -1");
  }

  const short numPages = z.readShort();

  for (short pageIdx = 0; pageIdx < numPages; ++pageIdx) {
    while (true) {
      const short numChars = z.readShort();
      if (numChars == -1) {
        break;
      }
      if (numChars > 0) {
        lines.push_back(z.readString(static_cast<size_t>(numChars)));
      } else {
        lines.emplace_back();
      }
    }
  }

  return lines;
} // NIDSTVSTable::extractTabularAlphanumericLines

std::string
NIDSTVSTable::getSubstr(
  const std::string & str,
  const std::string & startDelimiters,
  const std::string & endDelimiters,
  int               & startIndex,
  int               endIndex)
{
  if (str.empty()) {
    return { };
  }

  const int len = static_cast<int>(str.size());

  if ((startIndex < 0) || (startIndex >= len)) {
    return { };
  }

  if ((endIndex != 0) && (startIndex > endIndex)) {
    return { };
  }

  const size_t start = str.find_first_of(startDelimiters, static_cast<size_t>(startIndex));

  if (start == std::string::npos) {
    ++startIndex;
    return { };
  }

  if ((endIndex != 0) && (static_cast<int>(start) > endIndex)) {
    startIndex = len;
    return { };
  }

  const size_t next = str.find_first_of(endDelimiters, start);

  if (next == std::string::npos) {
    startIndex = len;
    return { };
  }

  if ((endIndex != 0) && (static_cast<int>(next) > endIndex)) {
    startIndex = len;
    return { };
  }

  startIndex = static_cast<int>(next) + 1;
  return str.substr(start, next - start);
} // NIDSTVSTable::getSubstr

std::shared_ptr<DataTable>
NIDSTVSTable::decodeTVSLines(
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
    { "Type",               DataColumn::Type::String },
    { "StormId",            DataColumn::Type::String },
    { "Azimuth",            DataColumn::Type::Float  },
    { "Range",              DataColumn::Type::Float  },
    { "AVGDV",              DataColumn::Type::Float  },
    { "LLDV",               DataColumn::Type::Float  },
    { "MXDV",               DataColumn::Type::Float  },
    { "MXDVHgt",            DataColumn::Type::Float  },
    { "Depth",              DataColumn::Type::String },
    { "Base",               DataColumn::Type::String },
    { "Top",                DataColumn::Type::Float  },
    { "MXSHR",              DataColumn::Type::Float  },
    { "MXSHRHgt",           DataColumn::Type::Float  },
    { Constants::Latitude,  DataColumn::Type::Float  },
    { Constants::Longitude, DataColumn::Type::Float  }
  };

  table->setSchema(schema);

  if (lines.size() < 3) {
    return table;
  }

  // Line 2 contains:
  // "   Radar Id   3   Date/Time  05:03:99/22:55:52   Number of TVS/ETVS    3/  0"
  int headerIndex    = 71;
  const int line2Len = static_cast<int>(lines[2].length());
  const int numTVS   =
    std::atoi(getSubstr(lines[2], DIGITS, "/", headerIndex, line2Len - 1).c_str());
  const int numETVS =
    std::atoi(getSubstr(lines[2], DIGITS, " ", headerIndex, line2Len - 1).c_str());

  std::vector<std::string> typeList;
  std::vector<std::string> stormIdList;
  std::vector<std::string> azList;
  std::vector<std::string> rangeList;
  std::vector<std::string> avgdvList;
  std::vector<std::string> lldvList;
  std::vector<std::string> mdvList;
  std::vector<std::string> heightList;
  std::vector<std::string> depthList;
  std::vector<std::string> baseList;
  std::vector<std::string> topList;
  std::vector<std::string> mxshearList;
  std::vector<std::string> hgtmxgtgList;

  const int lastRow = std::min(
    ROWS_IN_HEADER - 1 + numTVS + numETVS,
    static_cast<int>(lines.size()) - 1);

  for (int i = ROWS_IN_HEADER; i <= lastRow; ++i) {
    int index     = 0;
    const int len = static_cast<int>(lines[i].length());

    // Feature type
    typeList.push_back(getSubstr(lines[i], ALPHABETS, " ", index, len - 1));

    // Storm Id
    stormIdList.push_back(getSubstr(lines[i], ALPHABETS, " ", index, len - 1));

    // Azimuth (deg)
    azList.push_back(getSubstr(lines[i], DIGITS, "/", index, len - 1));

    // Range (nm)
    rangeList.push_back(getSubstr(lines[i], DIGITS, " ", index, len - 1));

    // AVGDV (kt)
    avgdvList.push_back(getSubstr(lines[i], DIGITS, " ", index, len - 1));

    // LLDV (kt)
    lldvList.push_back(getSubstr(lines[i], ABC_AND_DIGITS, " ", index, len - 1));

    // MXDV (kt)
    mdvList.push_back(getSubstr(lines[i], ABC_AND_DIGITS, "/", index, len - 1));

    // MXDVHgt (kft)
    heightList.push_back(getSubstr(lines[i], ABC_AND_DIGITS, " ", index, len - 1));

    // Depth handle > 18.9 case (kft)
    std::string tmp2 = getSubstr(lines[i], SYM_DIGITS, " ", index, len - 1);
    std::string tmpD = tmp2;
    if (tmp2.size() < 3) { // has a space after "<" or ">"
      tmpD  = getSubstr(lines[i], DIGITS, " ", index, len - 1);
      tmp2 += tmpD;
    }
    depthList.push_back(tmpD);

    // Base (handle < 0.9 case) (kft)
    std::string tmp  = getSubstr(lines[i], SYM_DIGITS, " ", index, len - 1);
    std::string tmpH = tmp;
    if (tmp.size() < 3) { // " > " or " < "
      tmpH = getSubstr(lines[i], DIGITS, "/", index, len - 1);
      tmp += tmpH;
    }
    baseList.push_back(tmp);

    // Top (kft)
    topList.push_back(getSubstr(lines[i], DIGITS, " ", index, len - 1));

    // MXSHR (E-3/s)
    mxshearList.push_back(getSubstr(lines[i], DIGITS, "/", index, len - 1));

    // MXSHR_Hgt (kft)
    hgtmxgtgList.push_back(getSubstr(lines[i], DIGITS, " ", index, len - 1));
  }

  const size_t totalRows = std::min({
    typeList.size(),
    stormIdList.size(),
    azList.size(),
    rangeList.size(),
    avgdvList.size(),
    lldvList.size(),
    mdvList.size(),
    heightList.size(),
    depthList.size(),
    baseList.size(),
    topList.size(),
    mxshearList.size(),
    hgtmxgtgList.size()
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
    row["Type"]     = typeList[j];
    row["StormId"]  = stormIdList[j];
    row["Azimuth"]  = azDegs;
    row["Range"]    = rangeNM;
    row["AVGDV"]    = safeParseFloat(avgdvList[j]);
    row["LLDV"]     = safeParseFloat(lldvList[j]);
    row["MXDV"]     = safeParseFloat(mdvList[j]);
    row["MXDVHgt"]  = safeParseFloat(heightList[j]);
    row["Depth"]    = depthList[j];
    row["Base"]     = baseList[j];
    row["Top"]      = safeParseFloat(topList[j]);
    row["MXSHR"]    = safeParseFloat(mxshearList[j]);
    row["MXSHRHgt"] = safeParseFloat(hgtmxgtgList[j]);
    row[Constants::Latitude]  = eventLat;
    row[Constants::Longitude] = eventLon;

    table->addRow(row);
  }

  return table;
} // NIDSTVSTable::decodeTVSLines
