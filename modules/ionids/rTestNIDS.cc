#include "rBOOSTTest.h"
#include "rIONIDS.h"
#include "rBlockMessageHeader.h"
#include "rBlockProductDesc.h"
#include "rBlockProductSymbology.h"
#include "rNIDSHailIndexTable.h"
#include "rNIDSMesoDetectionTable.h"
#include "rNIDSMesoTable.h"
#include "rNIDSSTITable.h"
#include "rNIDSStormCellTable.h"
#include "rNIDSStormStructureTable.h"
#include "rNIDSTVSTable.h"
#include "rNIDSRadialSet.h"
#include "rConfigNIDSInfo.h"
#include <rBinaryIO.h>
#include <rConstants.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <endian.h>

using namespace rapio;

namespace {
// Helper to write Big Endian shorts to our synthetic buffer
void
appendBE16(std::vector<char>& buf, uint16_t val)
{
  uint16_t be    = htobe16(val);
  const char * p = reinterpret_cast<const char *>(&be);

  buf.insert(buf.end(), p, p + 2);
}

// Helper to write Big Endian ints to our synthetic buffer
void
appendBE32(std::vector<char>& buf, uint32_t val)
{
  uint32_t be    = htobe32(val);
  const char * p = reinterpret_cast<const char *>(&be);

  buf.insert(buf.end(), p, p + 4);
}
} // namespace

BOOST_AUTO_TEST_SUITE(rTestNIDSSuite)

BOOST_AUTO_TEST_CASE(TestNIDSMessageHeaderDecoding)
{
  std::vector<char> rawData;

  // Construct a synthetic Message Header Block (18 bytes total)
  appendBE16(rawData, 159);      // Message Code (e.g., 159 for NIDS)
  appendBE16(rawData, 19498);    // Julian Date
  appendBE32(rawData, 43200000); // Time (seconds since midnight * 1000 or similar based on NIDS spec)
  appendBE32(rawData, 5000);     // Message Length
  appendBE16(rawData, 1);        // Source ID
  appendBE16(rawData, 2);        // Dest ID
  appendBE16(rawData, 3);        // Number of Blocks

  MemoryStreamBuffer stream(std::move(rawData));

  stream.setDataBigEndian(); // NIDS Level III is Big Endian

  BlockMessageHeader header;

  header.read(stream);

  BOOST_CHECK_EQUAL(header.getMsgCode(), 159);
  BOOST_CHECK_EQUAL(header.myMsgDate, 19498);
  BOOST_CHECK_EQUAL(header.myMsgTime, 43200000);
  BOOST_CHECK_EQUAL(header.getMsgLength(), 5000);
  BOOST_CHECK_EQUAL(header.myNumBlocks, 3);
}

BOOST_AUTO_TEST_CASE(TestNIDSProductDescDecoding)
{
  std::vector<char> rawData;

  // Construct a synthetic Product Description Block (102 bytes + 2 byte divider = 104 bytes)
  appendBE16(rawData, 0xFFFF); // Block Divider (-1)
  appendBE32(rawData, 35333);  // Latitude (35.333 * 1000)
  appendBE32(rawData, -97278); // Longitude (-97.278 * 1000)
  appendBE16(rawData, 1214);   // Height (0.370 KM * 3280.74 ft/KM)
  appendBE16(rawData, 159);    // Product Code
  appendBE16(rawData, 2);      // Op Mode
  appendBE16(rawData, 212);    // VCP
  appendBE16(rawData, 1);      // Sequence Number
  appendBE16(rawData, 5);      // Volume Scan Number
  appendBE16(rawData, 19498);  // Volume Scan Date
  appendBE32(rawData, 43200);  // Volume Scan Start Time (Seconds)
  appendBE16(rawData, 19498);  // Generation Date
  appendBE32(rawData, 43210);  // Generation Time
  appendBE16(rawData, 1);      // Dep1
  appendBE16(rawData, 2);      // Dep2
  appendBE16(rawData, 1);      // Elevation Number
  appendBE16(rawData, 5);      // Dep3 (Elev angle * 10)

  // 16 Data Thresholds
  for (int i = 0; i < 16; ++i) {
    appendBE16(rawData, i * 10);
  }

  // Dep 4 through 10
  for (int i = 4; i <= 10; ++i) {
    appendBE16(rawData, 0);
  }

  appendBE16(rawData, 0);   // Num Maps
  appendBE32(rawData, 100); // Symbology Offset
  appendBE32(rawData, 200); // Graphic Offset
  appendBE32(rawData, 300); // Tabular Offset

  MemoryStreamBuffer stream(std::move(rawData));

  stream.setDataBigEndian();

  BlockProductDesc desc;

  desc.read(stream);

  // Validate basic geography and codes
  BOOST_CHECK_CLOSE(desc.getLocation().getLatitudeDeg(), 35.333f, 0.01f);
  BOOST_CHECK_CLOSE(desc.getLocation().getLongitudeDeg(), -97.278f, 0.01f);
  BOOST_CHECK_EQUAL(desc.getProductCode(), 159);
  BOOST_CHECK_EQUAL(desc.getVCP(), 212);

  // Validate decoded thresholds array matches our synthetic injection
  std::array<short, 16> thresholds = desc.getEncodedThresholds();

  BOOST_CHECK_EQUAL(thresholds[5], 50);
  BOOST_CHECK_EQUAL(thresholds[15], 150);

  // Validate offsets
  BOOST_CHECK_EQUAL(desc.mySymbologyOffset, 100);
  BOOST_CHECK_EQUAL(desc.myGraphicOffset, 200);
  BOOST_CHECK_EQUAL(desc.myTabularOffset, 300);
}

BOOST_AUTO_TEST_CASE(TestNIDSTVSTableDecoding)
{
  const std::vector<std::string> rawLines = {
    "",
    "                               Tornado Vortex Signature",
    "   Radar Id   3   Date/Time  05:08:03/22:05:44   Number of TVS/ETVS    1/  1   ",
    "",
    "     Feat  Storm   AZ/RAN  AVGDV  LLDV  MXDV/Hgt   Depth    Base/Top   MXSHR/Hgt",
    "     Type    ID   (deg,nm)  (kt)  (kt)  (kt,kft)   (kft)     (kft)     (E-3/s,kft)",
    "",
    "     TVS     A1    180/ 45   65    55    85/12.5    15.2    < 0.9/16.1   42/10.2 ",
    "     ETVS    B2    270/ 30   40    35    50/ 8.0   > 18.9    2.5 /21.4   25/ 6.0 "
  };

  // Build a synthetic Tabular Alphanumeric Block (Block ID == 3)
  std::vector<char> rawData;

  appendBE16(rawData, 0xFFFF); // Block divider (-1)
  appendBE16(rawData, 3);      // Block ID (3 = Tabular Alphanumeric Block)
  appendBE32(rawData, 0);      // Block length (unused by stream parser)

  // Embedded MessageHeaderBlock (18 bytes) + ProductDescBlock (102 bytes) = 120 bytes
  for (int i = 0; i < 60; ++i) {
    appendBE16(rawData, 0);
  }

  appendBE16(rawData, 0xFFFF); // Page section divider (-1)
  appendBE16(rawData, 1);      // Number of pages

  for (const auto & line : rawLines) {
    appendBE16(rawData, static_cast<uint16_t>(line.size()));
    rawData.insert(rawData.end(), line.begin(), line.end());
  }
  appendBE16(rawData, 0xFFFF); // End of page marker (-1)

  MemoryStreamBuffer stream(std::move(rawData));

  stream.setDataBigEndian();

  BlockProductSymbology sym;

  sym.read(stream);
  BOOST_CHECK_EQUAL(sym.myBlockID, 3);

  const std::vector<std::string> extractedLines =
    NIDSTVSTable::extractTabularAlphanumericLines(sym, stream);

  BOOST_REQUIRE_EQUAL(extractedLines.size(), rawLines.size());

  LLH radarLoc(35.333f, -97.278f, 370.0f);
  Time eventTime(1700000000.0);
  std::shared_ptr<DataTable> table =
    NIDSTVSTable::decodeTVSLines(extractedLines, radarLoc, eventTime, "TVS");

  BOOST_REQUIRE(table != nullptr);
  BOOST_CHECK_EQUAL(table->getTypeName(), "TVS");
  BOOST_CHECK_EQUAL(table->getRowCount(), 2u);
  BOOST_CHECK(table->validateRectangularShape());

  // Validate column schema and types
  const std::vector<std::pair<std::string, DataColumn::Type> > expectedSchema = {
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

  const auto & colNames = table->getColumnNames();

  BOOST_REQUIRE_EQUAL(colNames.size(), expectedSchema.size());
  for (size_t i = 0; i < expectedSchema.size(); ++i) {
    BOOST_CHECK_EQUAL(colNames[i], expectedSchema[i].first);
    BOOST_CHECK(table->getColumn(expectedSchema[i].first).getType() == expectedSchema[i].second);
  }

  // Validate row values
  BOOST_CHECK_EQUAL(table->getColumn("Type").getCellAsString(0), "TVS");
  BOOST_CHECK_EQUAL(table->getColumn("StormId").getCellAsString(0), "A1");
  BOOST_CHECK_CLOSE(table->getColumn("Azimuth").getCellAsFloat(0), 180.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Range").getCellAsFloat(0), 45.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("AVGDV").getCellAsFloat(0), 65.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("LLDV").getCellAsFloat(0), 55.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("MXDV").getCellAsFloat(0), 85.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("MXDVHgt").getCellAsFloat(0), 12.5f, 0.01f);
  BOOST_CHECK_EQUAL(table->getColumn("Depth").getCellAsString(0), "15.2");
  BOOST_CHECK_EQUAL(table->getColumn("Base").getCellAsString(0), "<0.9");
  BOOST_CHECK_CLOSE(table->getColumn("Top").getCellAsFloat(0), 16.1f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("MXSHR").getCellAsFloat(0), 42.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("MXSHRHgt").getCellAsFloat(0), 10.2f, 0.01f);

  BOOST_CHECK_EQUAL(table->getColumn("Type").getCellAsString(1), "ETVS");
  BOOST_CHECK_EQUAL(table->getColumn("StormId").getCellAsString(1), "B2");
  BOOST_CHECK_CLOSE(table->getColumn("Azimuth").getCellAsFloat(1), 270.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Range").getCellAsFloat(1), 30.0f, 0.01f);
  BOOST_CHECK_EQUAL(table->getColumn("Depth").getCellAsString(1), "18.9");
  BOOST_CHECK_EQUAL(table->getColumn("Base").getCellAsString(1), "2.5");
  BOOST_CHECK_CLOSE(table->getColumn("Top").getCellAsFloat(1), 21.4f, 0.01f);

  // Azimuth 180 deg, Range 45 NM should be south of radar at roughly same longitude
  BOOST_CHECK_LT(table->getColumn(Constants::Latitude).getCellAsFloat(0), 35.333f);
  BOOST_CHECK_CLOSE(table->getColumn(Constants::Longitude).getCellAsFloat(0), -97.278f, 0.1f);
}

BOOST_AUTO_TEST_CASE(TestNIDSSTITableDecoding)
{
  const std::vector<std::string> rawLines = {
    "STORM ID     A1       B2      ",
    "AZ/RAN     180/ 45   090/ 30  ",
    "FCST MVT   240/ 35   NEW      ",
    "ERR/MEAN   1.5/2.0   0.0/0.0  ",
    "DBZM HGT    58 18.5   45 12.0 "
  };

  // Build a synthetic Graphic Alphanumeric Block (Block ID == 2, Packet 8)
  uint16_t pageLength = 0;

  for (const auto & line : rawLines) {
    pageLength += static_cast<uint16_t>(4 + 6 + line.size());
  }

  std::vector<char> rawData;

  appendBE16(rawData, 0xFFFF); // Block divider (-1)
  appendBE16(rawData, 2);      // Block ID (2 = Graphic Alphanumeric Block)
  appendBE32(rawData, 10 + 4 + pageLength);
  appendBE16(rawData, 1); // Number of pages

  appendBE16(rawData, 1);          // Page number 1
  appendBE16(rawData, pageLength); // Page length

  for (const auto & line : rawLines) {
    appendBE16(rawData, 8);                                      // Packet code 8
    appendBE16(rawData, static_cast<uint16_t>(6 + line.size())); // Data block length
    appendBE16(rawData, 1);                                      // Text color
    appendBE16(rawData, 0);                                      // I start
    appendBE16(rawData, 0);                                      // J start
    rawData.insert(rawData.end(), line.begin(), line.end());
  }

  MemoryStreamBuffer stream(std::move(rawData));

  stream.setDataBigEndian();

  BlockProductSymbology sym;

  sym.read(stream);
  BOOST_CHECK_EQUAL(sym.myBlockID, 2);

  const std::vector<std::string> extractedLines =
    NIDSHailIndexTable::extractGraphicAlphanumericLines(sym, stream);

  BOOST_REQUIRE_EQUAL(extractedLines.size(), rawLines.size());

  LLH radarLoc(35.333f, -97.278f, 370.0f);
  Time eventTime(1700000000.0);
  std::shared_ptr<DataTable> table =
    NIDSSTITable::decodeSTILines(extractedLines, radarLoc, eventTime, "StormTrackInfo");

  BOOST_REQUIRE(table != nullptr);
  BOOST_CHECK_EQUAL(table->getTypeName(), "StormTrackInfo");
  BOOST_CHECK_EQUAL(table->getRowCount(), 2u);
  BOOST_CHECK(table->validateRectangularShape());

  const std::vector<std::pair<std::string, DataColumn::Type> > expectedSchema = {
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

  const auto & colNames = table->getColumnNames();

  BOOST_REQUIRE_EQUAL(colNames.size(), expectedSchema.size());
  for (size_t i = 0; i < expectedSchema.size(); ++i) {
    BOOST_CHECK_EQUAL(colNames[i], expectedSchema[i].first);
    BOOST_CHECK(table->getColumn(expectedSchema[i].first).getType() == expectedSchema[i].second);
  }

  // Row 0: normal moving storm
  BOOST_CHECK_EQUAL(table->getColumn("StormId").getCellAsString(0), "A1");
  BOOST_CHECK_CLOSE(table->getColumn("Azimuth").getCellAsFloat(0), 180.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Range").getCellAsFloat(0), 45.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Dir").getCellAsFloat(0), 240.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Speed").getCellAsFloat(0), 35.0f, 0.01f);
  BOOST_CHECK_EQUAL(table->getColumn("NewStorm").getCellAsString(0), "");
  BOOST_CHECK_CLOSE(table->getColumn("FcstErr").getCellAsFloat(0), 1.5f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("MeanFcstErr").getCellAsFloat(0), 2.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("MaxRefl").getCellAsFloat(0), 58.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("MxRefHt").getCellAsFloat(0), 18.5f, 0.01f);
  BOOST_CHECK_EQUAL(table->getColumn("AlgRank").getCellAsInt(0), 0);

  // Row 1: NEW storm
  BOOST_CHECK_EQUAL(table->getColumn("StormId").getCellAsString(1), "B2");
  BOOST_CHECK_CLOSE(table->getColumn("Azimuth").getCellAsFloat(1), 90.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Range").getCellAsFloat(1), 30.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Dir").getCellAsFloat(1),
    static_cast<float>(Constants::MissingData), 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Speed").getCellAsFloat(1),
    static_cast<float>(Constants::MissingData), 0.01f);
  BOOST_CHECK_EQUAL(table->getColumn("NewStorm").getCellAsString(1), "NEW");
  BOOST_CHECK_CLOSE(table->getColumn("MaxRefl").getCellAsFloat(1), 45.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("MxRefHt").getCellAsFloat(1), 12.0f, 0.01f);
  BOOST_CHECK_EQUAL(table->getColumn("AlgRank").getCellAsInt(1), 1);

  // Azimuth 90 deg, Range 30 NM should be east of radar at roughly same latitude
  BOOST_CHECK_GT(table->getColumn(Constants::Longitude).getCellAsFloat(1), -97.278f);
  BOOST_CHECK_CLOSE(table->getColumn(Constants::Latitude).getCellAsFloat(1), 35.333f, 0.1f);
}

BOOST_AUTO_TEST_CASE(TestNIDSMesoTableDecoding)
{
  const std::vector<std::string> rawLines = {
    "STORM ID     A1       B2      ",
    "FEATURE    MESO     UNC MESO  ",
    "AZ RAN      180  45   270  30 ",
    "BASE TOP    3.5 18.2  4.0 15.0 ",
    "RAD AZDIA   2.1  2.4  1.8  2.0 "
  };

  uint16_t pageLength = 0;

  for (const auto & line : rawLines) {
    pageLength += static_cast<uint16_t>(4 + 6 + line.size());
  }

  std::vector<char> rawData;

  appendBE16(rawData, 0xFFFF); // Block divider (-1)
  appendBE16(rawData, 2);      // Block ID (2 = Graphic Alphanumeric Block)
  appendBE32(rawData, 10 + 4 + pageLength);
  appendBE16(rawData, 1); // Number of pages

  appendBE16(rawData, 1);          // Page number 1
  appendBE16(rawData, pageLength); // Page length

  for (const auto & line : rawLines) {
    appendBE16(rawData, 8);                                      // Packet code 8
    appendBE16(rawData, static_cast<uint16_t>(6 + line.size())); // Data block length
    appendBE16(rawData, 1);                                      // Text color
    appendBE16(rawData, 0);                                      // I start
    appendBE16(rawData, 0);                                      // J start
    rawData.insert(rawData.end(), line.begin(), line.end());
  }

  MemoryStreamBuffer stream(std::move(rawData));

  stream.setDataBigEndian();

  BlockProductSymbology sym;

  sym.read(stream);
  BOOST_CHECK_EQUAL(sym.myBlockID, 2);

  const std::vector<std::string> extractedLines =
    NIDSHailIndexTable::extractGraphicAlphanumericLines(sym, stream);

  BOOST_REQUIRE_EQUAL(extractedLines.size(), rawLines.size());

  LLH radarLoc(35.333f, -97.278f, 370.0f);
  Time eventTime(1700000000.0);
  std::shared_ptr<DataTable> table =
    NIDSMesoTable::decodeMesoLines(extractedLines, radarLoc, eventTime, "OrpgMesoTable");

  BOOST_REQUIRE(table != nullptr);
  BOOST_CHECK_EQUAL(table->getTypeName(), "OrpgMesoTable");
  BOOST_CHECK_EQUAL(table->getRowCount(), 2u);
  BOOST_CHECK(table->validateRectangularShape());

  const std::vector<std::pair<std::string, DataColumn::Type> > expectedSchema = {
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

  const auto & colNames = table->getColumnNames();

  BOOST_REQUIRE_EQUAL(colNames.size(), expectedSchema.size());
  for (size_t i = 0; i < expectedSchema.size(); ++i) {
    BOOST_CHECK_EQUAL(colNames[i], expectedSchema[i].first);
    BOOST_CHECK(table->getColumn(expectedSchema[i].first).getType() == expectedSchema[i].second);
  }

  BOOST_CHECK_EQUAL(table->getColumn("StormId").getCellAsString(0), "A1");
  BOOST_CHECK_CLOSE(table->getColumn("Azimuth").getCellAsFloat(0), 180.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Range").getCellAsFloat(0), 45.0f, 0.01f);
  BOOST_CHECK_EQUAL(table->getColumn("Type").getCellAsString(0), "MESO");
  BOOST_CHECK_CLOSE(table->getColumn("Base").getCellAsFloat(0), 3.5f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Top").getCellAsFloat(0), 18.2f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("RdlDiam").getCellAsFloat(0), 2.1f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("AzDiam").getCellAsFloat(0), 2.4f, 0.01f);

  BOOST_CHECK_EQUAL(table->getColumn("StormId").getCellAsString(1), "B2");
  BOOST_CHECK_EQUAL(table->getColumn("Type").getCellAsString(1), "UNCMESO");
  BOOST_CHECK_CLOSE(table->getColumn("Azimuth").getCellAsFloat(1), 270.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Range").getCellAsFloat(1), 30.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Base").getCellAsFloat(1), 4.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Top").getCellAsFloat(1), 15.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("RdlDiam").getCellAsFloat(1), 1.8f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("AzDiam").getCellAsFloat(1), 2.0f, 0.01f);

  // Verify "NO MESOS" returns an empty table with valid schema
  std::shared_ptr<DataTable> emptyTable =
    NIDSMesoTable::decodeMesoLines({ "NO MESOS" }, radarLoc, eventTime, "OrpgMesoTable");

  BOOST_REQUIRE(emptyTable != nullptr);
  BOOST_CHECK_EQUAL(emptyTable->getRowCount(), 0u);
  BOOST_CHECK_EQUAL(emptyTable->getColumnNames().size(), expectedSchema.size());
}

BOOST_AUTO_TEST_CASE(TestNIDSMesoDetectionTableDecoding)
{
  const std::vector<std::string> rawLines = {
    " CIR STMID  101   A1  202   B2 ",
    " SR   LLRV   5S   35   3L   22 ",
    " AZ    RAN  180   45  270   30 ",
    " HGT  MXRV 12.5   48  8.0   30 ",
    " BASE DPTH  2.5   14  3.0   10 "
  };

  uint16_t pageLength = 0;

  for (const auto & line : rawLines) {
    pageLength += static_cast<uint16_t>(4 + 6 + line.size());
  }

  std::vector<char> rawData;

  appendBE16(rawData, 0xFFFF); // Block divider (-1)
  appendBE16(rawData, 2);      // Block ID (2 = Graphic Alphanumeric Block)
  appendBE32(rawData, 10 + 4 + pageLength);
  appendBE16(rawData, 1); // Number of pages

  appendBE16(rawData, 1);          // Page number 1
  appendBE16(rawData, pageLength); // Page length

  for (const auto & line : rawLines) {
    appendBE16(rawData, 8);                                      // Packet code 8
    appendBE16(rawData, static_cast<uint16_t>(6 + line.size())); // Data block length
    appendBE16(rawData, 1);                                      // Text color
    appendBE16(rawData, 0);                                      // I start
    appendBE16(rawData, 0);                                      // J start
    rawData.insert(rawData.end(), line.begin(), line.end());
  }

  MemoryStreamBuffer stream(std::move(rawData));

  stream.setDataBigEndian();

  BlockProductSymbology sym;

  sym.read(stream);
  BOOST_CHECK_EQUAL(sym.myBlockID, 2);

  const std::vector<std::string> extractedLines =
    NIDSHailIndexTable::extractGraphicAlphanumericLines(sym, stream);

  BOOST_REQUIRE_EQUAL(extractedLines.size(), rawLines.size());

  LLH radarLoc(35.333f, -97.278f, 370.0f);
  Time eventTime(1700000000.0);
  std::shared_ptr<DataTable> table =
    NIDSMesoDetectionTable::decodeMesoDetectionLines(
    extractedLines, radarLoc, eventTime, "OrpgMesoDetectionTable");

  BOOST_REQUIRE(table != nullptr);
  BOOST_CHECK_EQUAL(table->getTypeName(), "OrpgMesoDetectionTable");
  BOOST_CHECK_EQUAL(table->getRowCount(), 2u);
  BOOST_CHECK(table->validateRectangularShape());

  const std::vector<std::pair<std::string, DataColumn::Type> > expectedSchema = {
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

  const auto & colNames = table->getColumnNames();

  BOOST_REQUIRE_EQUAL(colNames.size(), expectedSchema.size());
  for (size_t i = 0; i < expectedSchema.size(); ++i) {
    BOOST_CHECK_EQUAL(colNames[i], expectedSchema[i].first);
    BOOST_CHECK(table->getColumn(expectedSchema[i].first).getType() == expectedSchema[i].second);
  }

  BOOST_CHECK_EQUAL(table->getColumn("CircId").getCellAsString(0), "101");
  BOOST_CHECK_EQUAL(table->getColumn("StormId").getCellAsString(0), "A1");
  BOOST_CHECK_EQUAL(table->getColumn("StrengthRank").getCellAsString(0), "5S");
  BOOST_CHECK_CLOSE(table->getColumn("LLRV").getCellAsFloat(0), 35.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Azimuth").getCellAsFloat(0), 180.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Range").getCellAsFloat(0), 45.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("HgtMaxRV").getCellAsFloat(0), 12.5f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("MaxRV").getCellAsFloat(0), 48.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Base").getCellAsFloat(0), 2.5f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Depth").getCellAsFloat(0), 14.0f, 0.01f);

  BOOST_CHECK_EQUAL(table->getColumn("CircId").getCellAsString(1), "202");
  BOOST_CHECK_EQUAL(table->getColumn("StormId").getCellAsString(1), "B2");
  BOOST_CHECK_EQUAL(table->getColumn("StrengthRank").getCellAsString(1), "3L");
  BOOST_CHECK_CLOSE(table->getColumn("LLRV").getCellAsFloat(1), 22.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Azimuth").getCellAsFloat(1), 270.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Range").getCellAsFloat(1), 30.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("HgtMaxRV").getCellAsFloat(1), 8.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("MaxRV").getCellAsFloat(1), 30.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Base").getCellAsFloat(1), 3.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Depth").getCellAsFloat(1), 10.0f, 0.01f);

  // Verify "NO MESOS" returns an empty table with valid schema
  std::shared_ptr<DataTable> emptyTable =
    NIDSMesoDetectionTable::decodeMesoDetectionLines(
    { "NO MESOS" }, radarLoc, eventTime, "OrpgMesoDetectionTable");

  BOOST_REQUIRE(emptyTable != nullptr);
  BOOST_CHECK_EQUAL(emptyTable->getRowCount(), 0u);
  BOOST_CHECK_EQUAL(emptyTable->getColumnNames().size(), expectedSchema.size());
}

BOOST_AUTO_TEST_CASE(TestNIDSStormStructureTableDecoding)
{
  std::vector<char> rawData;

  // Build a synthetic Tabular Alphanumeric Block (Block ID == 3)
  appendBE16(rawData, 0xFFFF); // Block divider (-1)
  appendBE16(rawData, 3);      // Block ID (3 = Tabular Alphanumeric Block)
  appendBE32(rawData, 0);      // Block length (unused by stream parser)

  // Embedded MessageHeaderBlock (18 bytes) + ProductDescBlock (102 bytes) = 120 bytes
  for (int i = 0; i < 60; ++i) {
    appendBE16(rawData, 0);
  }

  // Stand-alone Tabular Alphanumeric Block (1 page, 1 line of 20 chars)
  const std::string line0 = "STORM STRUCTURE LINE";

  appendBE16(rawData, 0xFFFF); // divider2 (-1)
  appendBE16(rawData, 1);      // numPages
  appendBE16(rawData, static_cast<uint16_t>(line0.size()));
  rawData.insert(rawData.end(), line0.begin(), line0.end());
  appendBE16(rawData, 0xFFFF); // end of page (-1)

  // CellTrendVolScanTimes (Packet 22, 2 volumes: 600 and 605 min after midnight)
  appendBE16(rawData, 22);  // packetCode
  appendBE16(rawData, 6);   // lengthOfBlock
  rawData.push_back(2);     // numVols
  rawData.push_back(1);     // latestVolPtr
  appendBE16(rawData, 600); // volume 0: 10:00 UTC
  appendBE16(rawData, 605); // volume 1: 10:05 UTC

  // CellTrendDataPacket for Storm "A1" at (I=40 km, J=0 km)
  appendBE16(rawData, 21); // packetCode
  appendBE16(rawData, 0);  // lengthOfBlock
  rawData.push_back('A');
  rawData.push_back('1');
  appendBE16(rawData, 40 * 8); // iPos (km * 8)
  appendBE16(rawData, 0);      // jPos (km * 8)

  const short attrVals[8][2] = {
    { 3500, 4000 }, // 1: Top (/100) -> 35, 40 kft
    {  300,  400 }, // 2: Base (/100) -> 3, 4 kft
    {   18,   20 }, // 3: MxRefHgt -> 18, 20 kft
    {   70,   80 }, // 4: ProbHail -> 70, 80 %
    {   20,   30 }, // 5: ProbSHail -> 20, 30 %
    {   45,   55 }, // 6: VIL -> 45, 55 kg/m^2
    {   60,   64 }, // 7: MxdBZ -> 60, 64 dBZ
    { 1500, 1700 }  // 8: CentrHgt (/100) -> 15, 17 kft
  };

  for (int code = 1; code <= 8; ++code) {
    appendBE16(rawData, static_cast<uint16_t>(code));
    rawData.push_back(2); // numVols
    rawData.push_back(1); // latestVolPtr
    appendBE16(rawData, static_cast<uint16_t>(attrVals[code - 1][0]));
    appendBE16(rawData, static_cast<uint16_t>(attrVals[code - 1][1]));
  }

  MemoryStreamBuffer stream(std::move(rawData));

  stream.setDataBigEndian();

  BlockProductSymbology sym;

  sym.read(stream); // Consumes 10 bytes as IONIDS::createDataType does

  std::vector<short> volScanMinutes;
  std::vector<NIDSStormStructureTable::CellTrendPacket> packets;

  NIDSStormStructureTable::extractCellTrendData(sym, stream, volScanMinutes, packets);

  BOOST_REQUIRE_EQUAL(volScanMinutes.size(), 2u);
  BOOST_CHECK_EQUAL(volScanMinutes[0], 600);
  BOOST_CHECK_EQUAL(volScanMinutes[1], 605);
  BOOST_REQUIRE_EQUAL(packets.size(), 1u);
  BOOST_CHECK_EQUAL(packets[0].stormId, "A1");

  LLH radarLoc(35.333f, -97.278f, 370.0f);
  Time eventTime(2024, 5, 1, 10, 5, 0);
  std::shared_ptr<DataTable> table =
    NIDSStormStructureTable::decodeStormStructurePackets(
    volScanMinutes, packets, radarLoc, eventTime, "StormStructure");

  BOOST_REQUIRE(table != nullptr);
  BOOST_CHECK_EQUAL(table->getTypeName(), "StormStructure");
  BOOST_CHECK_EQUAL(table->getRowCount(), 2u);
  BOOST_CHECK(table->validateRectangularShape());

  const std::vector<std::pair<std::string, DataColumn::Type> > expectedSchema = {
    { "StormId",            DataColumn::Type::String  },
    { "Top",                DataColumn::Type::Float   },
    { "Base",               DataColumn::Type::Float   },
    { "MxRefHgt",           DataColumn::Type::Float   },
    { "ProbHail",           DataColumn::Type::Float   },
    { "ProbSHail",          DataColumn::Type::Float   },
    { "VIL",                DataColumn::Type::Float   },
    { "MxdBZ",              DataColumn::Type::Float   },
    { "CentrHgt",           DataColumn::Type::Float   },
    { "AlgRank",            DataColumn::Type::Integer },
    { Constants::Latitude,  DataColumn::Type::Float   },
    { Constants::Longitude, DataColumn::Type::Float   },
    { "EpochSeconds",       DataColumn::Type::Integer }
  };

  const auto & colNames = table->getColumnNames();

  BOOST_REQUIRE_EQUAL(colNames.size(), expectedSchema.size());
  for (size_t i = 0; i < expectedSchema.size(); ++i) {
    BOOST_CHECK_EQUAL(colNames[i], expectedSchema[i].first);
    BOOST_CHECK(table->getColumn(expectedSchema[i].first).getType() == expectedSchema[i].second);
  }

  BOOST_CHECK_EQUAL(table->getColumn("StormId").getCellAsString(0), "A1");
  BOOST_CHECK_CLOSE(table->getColumn("Top").getCellAsFloat(0), 35.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Base").getCellAsFloat(0), 3.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("MxRefHgt").getCellAsFloat(0), 18.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("ProbHail").getCellAsFloat(0), 70.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("ProbSHail").getCellAsFloat(0), 20.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("VIL").getCellAsFloat(0), 45.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("MxdBZ").getCellAsFloat(0), 60.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("CentrHgt").getCellAsFloat(0), 15.0f, 0.01f);
  BOOST_CHECK_EQUAL(table->getColumn("AlgRank").getCellAsInt(0), 0);

  BOOST_CHECK_CLOSE(table->getColumn("Top").getCellAsFloat(1), 40.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("MxdBZ").getCellAsFloat(1), 64.0f, 0.01f);
  BOOST_CHECK_EQUAL(table->getColumn("AlgRank").getCellAsInt(1), 1);

  // EpochSeconds should increase by 300s (5 minutes) between volume 0 and volume 1
  BOOST_CHECK_EQUAL(
    table->getColumn("EpochSeconds").getCellAsInt(1)
    - table->getColumn("EpochSeconds").getCellAsInt(0),
    300);

  // +40 km East (I=40, J=0) should be east of radar
  BOOST_CHECK_GT(table->getColumn(Constants::Longitude).getCellAsFloat(0), -97.278f);
  BOOST_CHECK_CLOSE(table->getColumn(Constants::Latitude).getCellAsFloat(0), 35.333f, 0.1f);
}

BOOST_AUTO_TEST_CASE(TestNIDSStormCellTableDecoding)
{
  const std::vector<std::string> packet8Lines = {
    "STM ID  AZ/RAN  TVS  MESO  POH/POSH  MXSZ  VIL  DBZM  HT  TOP  FCST ",
    " A1     180/ 45  TVS  MESO   80/60   1.50   45   62  18.5 >42.0 240/35 ",
    " B2     090/ 30  NONE NONE  UNKNOWN         30   50  12.0  32.5  NEW   "
  };

  uint16_t pageLength = 0;

  for (const auto & line : packet8Lines) {
    pageLength += static_cast<uint16_t>(4 + 6 + line.size());
  }

  std::vector<char> rawData;

  appendBE16(rawData, 0xFFFF); // Block divider (-1)
  appendBE16(rawData, 2);      // Block ID (2 = Graphic Alphanumeric Block)
  appendBE32(rawData, 10 + 4 + pageLength);
  appendBE16(rawData, 1); // Number of pages

  appendBE16(rawData, 1);          // Page number 1
  appendBE16(rawData, pageLength); // Page length

  for (const auto & line : packet8Lines) {
    appendBE16(rawData, 8);
    appendBE16(rawData, static_cast<uint16_t>(6 + line.size()));
    appendBE16(rawData, 1);
    appendBE16(rawData, 0);
    appendBE16(rawData, 0);
    rawData.insert(rawData.end(), line.begin(), line.end());
  }

  MemoryStreamBuffer stream(std::move(rawData));

  stream.setDataBigEndian();

  BlockProductSymbology sym;

  sym.read(stream);
  BOOST_CHECK_EQUAL(sym.myBlockID, 2);

  // Packet 0 (header) should be skipped, leaving 2 storm cell rows
  const std::vector<std::string> extractedLines =
    NIDSStormCellTable::extractStormCellLines(sym, stream);

  BOOST_REQUIRE_EQUAL(extractedLines.size(), 2u);

  LLH radarLoc(35.333f, -97.278f, 370.0f);
  Time eventTime(1700000000.0);
  std::shared_ptr<DataTable> table =
    NIDSStormCellTable::decodeStormCellLines(
    extractedLines, radarLoc, eventTime, "StormCellTable");

  BOOST_REQUIRE(table != nullptr);
  BOOST_CHECK_EQUAL(table->getTypeName(), "StormCellTable");
  BOOST_CHECK_EQUAL(table->getRowCount(), 2u);
  BOOST_CHECK(table->validateRectangularShape());

  const std::vector<std::pair<std::string, DataColumn::Type> > expectedSchema = {
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

  const auto & colNames = table->getColumnNames();

  BOOST_REQUIRE_EQUAL(colNames.size(), expectedSchema.size());
  for (size_t i = 0; i < expectedSchema.size(); ++i) {
    BOOST_CHECK_EQUAL(colNames[i], expectedSchema[i].first);
    BOOST_CHECK(table->getColumn(expectedSchema[i].first).getType() == expectedSchema[i].second);
  }

  // Row 0: A1 with >42.0 TopFlag and moving storm
  BOOST_CHECK_EQUAL(table->getColumn("StormId").getCellAsString(0), "A1");
  BOOST_CHECK_CLOSE(table->getColumn("Azimuth").getCellAsFloat(0), 180.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Range").getCellAsFloat(0), 45.0f, 0.01f);
  BOOST_CHECK_EQUAL(table->getColumn("TVSType").getCellAsString(0), "TVS");
  BOOST_CHECK_EQUAL(table->getColumn("MesoType").getCellAsString(0), "MESO");
  BOOST_CHECK_CLOSE(table->getColumn("POH").getCellAsFloat(0), 80.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("POSH").getCellAsFloat(0), 60.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("MxHailSz").getCellAsFloat(0), 1.50f, 0.01f);
  BOOST_CHECK_EQUAL(table->getColumn("HailSzRange").getCellAsString(0), "");
  BOOST_CHECK_CLOSE(table->getColumn("VIL").getCellAsFloat(0), 45.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("MaxRefl").getCellAsFloat(0), 62.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("HtMaxRefl").getCellAsFloat(0), 18.5f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Top").getCellAsFloat(0), 42.0f, 0.01f);
  BOOST_CHECK_EQUAL(table->getColumn("TopFlag").getCellAsString(0), "> Top");
  BOOST_CHECK_CLOSE(table->getColumn("Dir").getCellAsFloat(0), 240.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Speed").getCellAsFloat(0), 35.0f, 0.01f);
  BOOST_CHECK_EQUAL(table->getColumn("NewStorm").getCellAsString(0), "");
  BOOST_CHECK_EQUAL(table->getColumn("AlgRank").getCellAsInt(0), 0);

  // Row 1: B2 with UNKNOWN hail and NEW storm
  BOOST_CHECK_EQUAL(table->getColumn("StormId").getCellAsString(1), "B2");
  BOOST_CHECK_EQUAL(table->getColumn("HailSzRange").getCellAsString(1), "UNKNOWN");
  BOOST_CHECK_CLOSE(table->getColumn("VIL").getCellAsFloat(1), 30.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("MaxRefl").getCellAsFloat(1), 50.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("HtMaxRefl").getCellAsFloat(1), 12.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Top").getCellAsFloat(1), 32.5f, 0.01f);
  BOOST_CHECK_EQUAL(table->getColumn("TopFlag").getCellAsString(1), "");
  BOOST_CHECK_CLOSE(table->getColumn("Dir").getCellAsFloat(1),
    static_cast<float>(Constants::MissingData), 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Speed").getCellAsFloat(1),
    static_cast<float>(Constants::MissingData), 0.01f);
  BOOST_CHECK_EQUAL(table->getColumn("NewStorm").getCellAsString(1), "NEW");
  BOOST_CHECK_EQUAL(table->getColumn("AlgRank").getCellAsInt(1), 1);
}

BOOST_AUTO_TEST_CASE(TestNIDSHailIndexTableDecoding)
{
  const std::vector<std::string> packet8Lines = {
    "STORM ID     A1       B2      ",
    "AZ/RAN     180/ 45   270/ 30  ",
    "POSH/POH    60/ 80   UNKNOWN  ",
    "MAX HAIL SIZE   1.75     <0.50 ",
    "0C/-20C TEMP ALT   10.5/22.0 KFT                     14:30 05/20/24 "
  };

  uint16_t pageLength = 0;

  for (const auto & line : packet8Lines) {
    pageLength += static_cast<uint16_t>(4 + 6 + line.size());
  }

  std::vector<char> rawData;

  appendBE16(rawData, 0xFFFF); // Block divider (-1)
  appendBE16(rawData, 2);      // Block ID (2 = Graphic Alphanumeric Block)
  appendBE32(rawData, 10 + 4 + pageLength);
  appendBE16(rawData, 1); // Number of pages

  appendBE16(rawData, 1);          // Page number 1
  appendBE16(rawData, pageLength); // Page length

  for (const auto & line : packet8Lines) {
    appendBE16(rawData, 8);                                      // Packet code 8
    appendBE16(rawData, static_cast<uint16_t>(6 + line.size())); // Data block length
    appendBE16(rawData, 1);                                      // Text color
    appendBE16(rawData, 0);                                      // I start
    appendBE16(rawData, 0);                                      // J start
    rawData.insert(rawData.end(), line.begin(), line.end());
  }

  MemoryStreamBuffer stream(std::move(rawData));

  stream.setDataBigEndian();

  BlockProductSymbology sym;

  sym.read(stream);
  BOOST_CHECK_EQUAL(sym.myBlockID, 2);

  const std::vector<std::string> extractedLines =
    NIDSHailIndexTable::extractGraphicAlphanumericLines(sym, stream);

  BOOST_REQUIRE_EQUAL(extractedLines.size(), packet8Lines.size());

  LLH radarLoc(35.333f, -97.278f, 370.0f);
  Time eventTime(1700000000.0);
  std::shared_ptr<DataTable> table =
    NIDSHailIndexTable::decodeHailIndexLines(
    extractedLines, radarLoc, eventTime, "HailIndex");

  BOOST_REQUIRE(table != nullptr);
  BOOST_CHECK_EQUAL(table->getTypeName(), "HailIndex");
  BOOST_CHECK_EQUAL(table->getRowCount(), 2u);
  BOOST_CHECK(table->validateRectangularShape());

  const std::vector<std::pair<std::string, DataColumn::Type> > expectedSchema = {
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

  const auto & colNames = table->getColumnNames();

  BOOST_REQUIRE_EQUAL(colNames.size(), expectedSchema.size());
  for (size_t i = 0; i < expectedSchema.size(); ++i) {
    BOOST_CHECK_EQUAL(colNames[i], expectedSchema[i].first);
    BOOST_CHECK(table->getColumn(expectedSchema[i].first).getType() == expectedSchema[i].second);
  }

  // Row 0: A1 (180 deg / 45 nm, due south of radar)
  BOOST_CHECK_EQUAL(table->getColumn("StormId").getCellAsString(0), "A1");
  BOOST_CHECK_CLOSE(table->getColumn("Azimuth").getCellAsFloat(0), 180.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Range").getCellAsFloat(0), 45.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("POH").getCellAsFloat(0), 80.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("POSH").getCellAsFloat(0), 60.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("MxExpSz").getCellAsFloat(0), 1.75f, 0.01f);
  BOOST_CHECK_EQUAL(table->getColumn("SzRange").getCellAsString(0), "NA");
  BOOST_CHECK_CLOSE(table->getColumn("0CLevel").getCellAsFloat(0), 10.5f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("-20CLevel").getCellAsFloat(0), 22.0f, 0.01f);
  BOOST_CHECK_EQUAL(table->getColumn("HrHTAChng").getCellAsString(0), "14");
  BOOST_CHECK_EQUAL(table->getColumn("MnHTAChng").getCellAsString(0), "30");
  BOOST_CHECK_EQUAL(table->getColumn("MoHTAChng").getCellAsString(0), "05");
  BOOST_CHECK_EQUAL(table->getColumn("DayHTAChng").getCellAsString(0), "20");
  BOOST_CHECK_EQUAL(table->getColumn("YrsHTAChng").getCellAsString(0), "24");
  BOOST_CHECK_EQUAL(table->getColumn("AlgRank").getCellAsInt(0), 0);
  BOOST_CHECK_LT(table->getColumn(Constants::Latitude).getCellAsFloat(0), 35.333f);
  BOOST_CHECK_CLOSE(table->getColumn(Constants::Longitude).getCellAsFloat(0), -97.278f, 0.1f);

  // Row 1: B2 (270 deg / 30 nm, due west of radar)
  BOOST_CHECK_EQUAL(table->getColumn("StormId").getCellAsString(1), "B2");
  BOOST_CHECK_CLOSE(table->getColumn("Azimuth").getCellAsFloat(1), 270.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("Range").getCellAsFloat(1), 30.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("POH").getCellAsFloat(1), 0.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("POSH").getCellAsFloat(1), 0.0f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("MxExpSz").getCellAsFloat(1),
    static_cast<float>(Constants::MissingData), 0.01f);
  BOOST_CHECK_EQUAL(table->getColumn("SzRange").getCellAsString(1), "<0.5");
  BOOST_CHECK_CLOSE(table->getColumn("0CLevel").getCellAsFloat(1), 10.5f, 0.01f);
  BOOST_CHECK_CLOSE(table->getColumn("-20CLevel").getCellAsFloat(1), 22.0f, 0.01f);
  BOOST_CHECK_EQUAL(table->getColumn("AlgRank").getCellAsInt(1), 1);
  BOOST_CHECK_CLOSE(table->getColumn(Constants::Latitude).getCellAsFloat(1), 35.333f, 0.1f);
  BOOST_CHECK_LT(table->getColumn(Constants::Longitude).getCellAsFloat(1), -97.278f);
}

BOOST_AUTO_TEST_CASE(TestNIDSNullProductRadialSetDecoding)
{
  const int productCode = 170; // DAA (Dual-Pol Digital Accumulation Array)
  const auto info       = ConfigNIDSInfo::getNIDSInfo(productCode);

  BlockMessageHeader hdr;

  hdr.myMsgCode   = productCode;
  hdr.myMsgDate   = 19498;
  hdr.myMsgTime   = 43200;
  hdr.myMsgLength = 140;
  hdr.myNumBlocks = 3;

  BlockProductDesc desc;

  desc.setLocation(LLH(35.333f, -97.278f, 370.0f));
  desc.setProductCode(productCode);
  desc.setVCP(212);
  desc.setVolumeStartTime(Time(1700000000.0));
  desc.setDep(3, 1); // Null product flag set

  BlockProductSymbology sym;

  sym.myBlockID     = 1;
  sym.myBlockLength = 30;
  sym.myNumLayers   = 1;

  // Build a Packet 1 symbology layer (divider -1, layer length, packet code 1, text payload)
  const std::string nullText = "NO PRECIP   ";
  std::vector<char> layerData;

  appendBE16(layerData, 0xFFFF); // Layer divider (-1)
  appendBE32(layerData, static_cast<uint32_t>(sizeof(short) + nullText.size()));
  appendBE16(layerData, 1); // Packet Code 1 (Text/Symbol null product)
  layerData.insert(layerData.end(), nullText.begin(), nullText.end());

  // 1) Verify BlockRadialSet::read handles Packet 1 cleanly without throwing
  {
    std::vector<char> pkt1Buf = layerData;
    MemoryStreamBuffer pkt1Stream(std::move(pkt1Buf));
    pkt1Stream.setDataBigEndian();

    BlockRadialSet block;
    BOOST_CHECK_NO_THROW(block.read(pkt1Stream));
    BOOST_CHECK(block.isNullPacket());
    BOOST_CHECK_EQUAL(block.getNumRadials(), 0u);
  }

  // 2) Verify NIDSRadialSet::readNIDS returns MissingData-filled RadialSet when dep[3] == 1
  IOConfig ioCfg;
  NIDSRadialSet reader;

  {
    std::vector<char> symBuf = layerData;
    MemoryStreamBuffer stream(std::move(symBuf));
    stream.setDataBigEndian();

    std::shared_ptr<DataType> dt = reader.readNIDS(ioCfg, hdr, desc, sym, stream);
    BOOST_REQUIRE(dt != nullptr);

    auto rs = std::dynamic_pointer_cast<RadialSet>(dt);
    BOOST_REQUIRE(rs != nullptr);

    const AngleDegs expectedWidth =
      (info.getBeamWidthDegs() > 0.0f) ? info.getBeamWidthDegs() : 1.0f;
    const size_t expectedRadials = static_cast<size_t>((360.0f / expectedWidth) + 0.5f);
    const size_t expectedGates   = info.getNullProductNumGates();

    BOOST_CHECK_EQUAL(rs->getVCP(), 212);
    BOOST_CHECK_EQUAL(rs->getNumRadials(), expectedRadials);
    BOOST_CHECK_EQUAL(rs->getNumGates(), expectedGates);
    BOOST_CHECK_CLOSE(rs->getElevationDegs(), 0.0f, 0.01f);

    ArrayFloat2DPtr dataPtr = rs->getFloat2DPtr();
    BOOST_REQUIRE(dataPtr != nullptr);

    for (size_t rIdx = 0; rIdx < expectedRadials; ++rIdx) {
      for (size_t gIdx = 0; gIdx < expectedGates; ++gIdx) {
        BOOST_CHECK_EQUAL((*dataPtr)[rIdx][gIdx], static_cast<float>(Constants::MissingData));
      }
    }
  }

  // 3) Verify NIDSRadialSet::readNIDS also handles dep[3] == 0 with a Packet 1 symbology layer
  {
    desc.setDep(3, 0);
    std::vector<char> symBuf = layerData;
    MemoryStreamBuffer stream(std::move(symBuf));
    stream.setDataBigEndian();

    std::shared_ptr<DataType> dt = reader.readNIDS(ioCfg, hdr, desc, sym, stream);
    BOOST_REQUIRE(dt != nullptr);

    auto rs = std::dynamic_pointer_cast<RadialSet>(dt);
    BOOST_REQUIRE(rs != nullptr);
    BOOST_CHECK_EQUAL(rs->getVCP(), 212);
    BOOST_CHECK_EQUAL(rs->getNumRadials(), static_cast<size_t>((360.0f / info.getBeamWidthDegs()) + 0.5f));
    BOOST_CHECK_EQUAL(rs->getNumGates(), info.getNullProductNumGates());
  }
}

BOOST_AUTO_TEST_SUITE_END()
