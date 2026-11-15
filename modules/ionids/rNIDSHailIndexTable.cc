#include "rNIDSHailIndexTable.h"

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
constexpr const char * ABC_SYM_DIG    = "?ABCDEFGHIJKLMNOPQRSTUVWXYZ><0123456789";

constexpr float NAUTICAL_MILES_TO_KM = 1.852f;

float
safeParseFloat(const std::string& s, float fallback = 0.0f)
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
NIDSHailIndexTable::introduceSelf(IONIDS * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<NIDSHailIndexTable>();

  owner->introduce("OrpgHailIndex", io);
  owner->introduce("HailIndex", io);
}

std::shared_ptr<DataType>
NIDSHailIndexTable::readNIDS(
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
    (info.isValid() && !info.getType().empty()) ? info.getType() : "HailIndex";

  const std::vector<std::string> lines = extractGraphicAlphanumericLines(s, z);

  return decodeHailIndexLines(lines, d.getLocation(), d.getVolumeStartTime(), typeName);
}

bool
NIDSHailIndexTable::writeNIDS(
  IOConfig                  & /*config*/,
  std::shared_ptr<DataType> /*dt*/,
  StreamBuffer              & /*z*/)
{
  return false;
}

std::vector<std::string>
NIDSHailIndexTable::extractGraphicAlphanumericLines(
  BlockProductSymbology & s,
  StreamBuffer          & z)
{
  std::vector<std::string> lines;

  // IONIDS::createDataType reads the 10-byte BlockProductSymbology header before
  // invoking readNIDS. If Block ID == 1 (Product Symbology), skip its payload
  // and read the Graphic Alphanumeric Block (Block ID == 2) header. If Block ID == 2,
  // the header was already consumed into `s`.
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
    int bytesRead = 0;

    while (bytesRead < pageLength) {
      const short packetCode        = z.readShort();
      const short lengthOfDataBlock = z.readShort();

      if (packetCode == 8) {
        /* short textColor = */ z.readShort();
        /* short iStart    = */ z.readShort();
        /* short jStart    = */ z.readShort();

        const int textLen = lengthOfDataBlock - 6;
        if (textLen > 0) {
          lines.push_back(z.readString(static_cast<size_t>(textLen)));
        }
      } else {
        if (lengthOfDataBlock > 0) {
          z.forward(static_cast<size_t>(lengthOfDataBlock));
        }
      }

      bytesRead += lengthOfDataBlock + 4;
    }
  }

  return lines;
} // NIDSHailIndexTable::extractGraphicAlphanumericLines

std::string
NIDSHailIndexTable::getSubstr(
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
} // NIDSHailIndexTable::getSubstr

std::shared_ptr<DataTable>
NIDSHailIndexTable::decodeHailIndexLines(
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
    { "POH",                DataColumn::Type::Float   },
    { "POSH",               DataColumn::Type::Float   },
    { "MxExpSz",            DataColumn::Type::Float   },
    { "SzRange",            DataColumn::Type::String  },
    { "0CLevel",            DataColumn::Type::Float   },
    { "-20CLevel",          DataColumn::Type::Float   },
    { "HrHTAChng",          DataColumn::Type::String  },
    { "MnHTAChng",          DataColumn::Type::String  },
    { "MoHTAChng",          DataColumn::Type::String  },
    { "DayHTAChng",         DataColumn::Type::String  },
    { "YrsHTAChng",         DataColumn::Type::String  },
    { "AlgRank",            DataColumn::Type::Integer },
    { Constants::Latitude,  DataColumn::Type::Float   },
    { Constants::Longitude, DataColumn::Type::Float   }
  };

  table->setSchema(schema);

  std::vector<std::string> stormIdList;
  std::vector<std::string> azList;
  std::vector<std::string> rangeList;
  std::vector<std::string> hailSizeList;
  std::vector<std::string> pohList;
  std::vector<std::string> poshList;

  std::string temp0;
  std::string temp20;
  std::string hours;
  std::string min;
  std::string mon;
  std::string day;
  std::string year;

  int columns = 0;

  for (size_t i = 0; i < lines.size(); ++i) {
    const int len = static_cast<int>(lines[i].length());
    int index     = 0;

    switch (i % 5) {
        case 0: {
          columns = 0;
          index   = 11; // Skip "STORM ID"
          while (index < len) {
            std::string temp = getSubstr(lines[i], ALPHABETS, " ", index, len - 1);
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
            azList.push_back(getSubstr(lines[i], DIGITS, "/", index, len - 1));
            rangeList.push_back(getSubstr(lines[i], DIGITS, " ", index, len - 1));
          }
          break;
        }

        case 2: {
          index = 10; // Skip "POSH/POH"
          for (int k = 0; k < columns; ++k) {
            std::string tempStr = getSubstr(lines[i], ABC_AND_DIGITS, "/ ", index, len - 1);
            if (tempStr.find("UNKNOWN") == std::string::npos) {
              poshList.push_back(tempStr);
              pohList.push_back(getSubstr(lines[i], ABC_AND_DIGITS, " ", index, len - 1));
            } else {
              poshList.push_back("UNKNOWN");
              pohList.push_back("UNKNOWN");
            }
          }
          break;
        }

        case 3: {
          index = 15; // Skip "MAX HAIL SIZE"
          for (int k = 0; k < columns; ++k) {
            hailSizeList.push_back(getSubstr(lines[i], ABC_SYM_DIG, " ", index, len - 1));
          }
          break;
        }

        case 4: {
          index  = 19; // Skip "0/ -20 C TEMP ALT"
          temp0  = getSubstr(lines[i], DIGITS, "/", index, len - 1);
          temp20 = getSubstr(lines[i], DIGITS, " ", index, len - 1);
          index += 24;
          hours  = getSubstr(lines[i], DIGITS, ":", index, len - 1);
          min    = getSubstr(lines[i], DIGITS, " ", index, len - 1);
          mon    = getSubstr(lines[i], DIGITS, "/", index, len - 1);
          day    = getSubstr(lines[i], DIGITS, "/", index, len - 1);
          year   = getSubstr(lines[i], DIGITS, " ", index, len - 1);
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
    pohList.size(),
    poshList.size(),
    hailSizeList.size()
  });

  const float temp0Val  = safeParseFloat(temp0);
  const float temp20Val = safeParseFloat(temp20);

  for (size_t j = 0; j < totalRows; ++j) {
    const float azDegs  = safeParseFloat(azList[j]);
    const float rangeNM = safeParseFloat(rangeList[j]);
    const float pohVal  = safeParseFloat(pohList[j]);
    const float poshVal = safeParseFloat(poshList[j]);

    float mxExpSz = static_cast<float>(Constants::MissingData);
    std::string szRange;

    if (hailSizeList[j].find("UNKNOWN") == std::string::npos) {
      if (hailSizeList[j].find('>') != std::string::npos) {
        szRange = ">4.0";
      } else if (hailSizeList[j].find('<') != std::string::npos) {
        szRange = "<0.5";
      } else {
        mxExpSz = safeParseFloat(hailSizeList[j]);
        szRange = "NA";
      }
    } else {
      szRange = "UNK";
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
    row["POH"]                = pohVal;
    row["POSH"]               = poshVal;
    row["MxExpSz"]            = mxExpSz;
    row["SzRange"]            = szRange;
    row["0CLevel"]            = temp0Val;
    row["-20CLevel"]          = temp20Val;
    row["HrHTAChng"]          = hours;
    row["MnHTAChng"]          = min;
    row["MoHTAChng"]          = mon;
    row["DayHTAChng"]         = day;
    row["YrsHTAChng"]         = year;
    row["AlgRank"]            = static_cast<int>(j);
    row[Constants::Latitude]  = eventLat;
    row[Constants::Longitude] = eventLon;

    table->addRow(row);
  }

  return table;
} // NIDSHailIndexTable::decodeHailIndexLines
