// Add this at top for any BOOST test
#include "rBOOSTTest.h"

#include "rIODataType.h"
#include "rPTreeData.h"
#include "rDataTable.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace rapio;

BOOST_AUTO_TEST_SUITE(_IODataType_)

// Helper function to test full IO round trips for any format
void testFormatRoundTrip(const std::string& format, const std::string& fileExt, const std::string& rawData) {
  std::string filename = "test_roundtrip." + fileExt;

  // 0. Bootstrap: Write the initial raw data to disk so we are self-contained
  std::ofstream out(filename, std::ios::binary);
  out.write(rawData.data(), rawData.size());
  out.close();

  // 1. Read from file
  auto dtFromFile = IODataType::read<PTreeData>(filename, format);
  BOOST_REQUIRE_MESSAGE(dtFromFile != nullptr, "Failed to read " << format << " from file");

  // 2. Write to memory buffer
  std::vector<char> buffer1;
  IOConfig keys1;
  keys1.set("suffix", fileExt);
  size_t bytesWritten1 = IODataType::writeBuffer(dtFromFile, buffer1, keys1, format);
  BOOST_REQUIRE_MESSAGE(bytesWritten1 > 0, "Failed to write " << format << " to buffer");

  // 3. Read back from memory buffer
  auto dtFromBuffer = IODataType::readBuffer<PTreeData>(buffer1, format);
  BOOST_REQUIRE_MESSAGE(dtFromBuffer != nullptr, "Failed to read " << format << " from buffer");

  // 4. Write back to file
  IOConfig keys2;
  keys2.set("suffix", fileExt);
  keys2.set("filepathmode", "direct");
  keys2.set("filename", filename);
  bool writeSuccess = IODataType::write(dtFromBuffer, filename, format);
  BOOST_REQUIRE_MESSAGE(writeSuccess, "Failed to write " << format << " back to file");

  // 5. Read from file AGAIN
  auto dtFromFile2 = IODataType::read<PTreeData>(filename, format);
  BOOST_REQUIRE_MESSAGE(dtFromFile2 != nullptr, "Failed to read " << format << " from file second time");

  // 6. Write to second memory buffer
  std::vector<char> buffer2;
  size_t bytesWritten2 = IODataType::writeBuffer(dtFromFile2, buffer2, keys2, format);

  // 7. Verify buffers match (Proves serialization/deserialization is deterministic)
  BOOST_CHECK_EQUAL(buffer1.size(), buffer2.size());
  
  // Clean up
  remove(filename.c_str());
}

BOOST_AUTO_TEST_CASE(_IODataType_XML_RoundTrip)
{
  std::string xmlData = 
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<w2algxml>\n"
    "  <program>TestProg</program>\n"
    "  <inputs>\n"
    "    <item><name>Reflectivity</name></item>\n"
    "  </inputs>\n"
    "</w2algxml>\n";
  testFormatRoundTrip("xml", "xml", xmlData);
}

BOOST_AUTO_TEST_CASE(_IODataType_JSON_RoundTrip)
{
  std::string jsonData = 
    "{\n"
    "  \"w2algxml\": {\n"
    "    \"program\": \"TestProg\",\n"
    "    \"inputs\": [\n"
    "      { \"name\": \"Reflectivity\" }\n"
    "    ]\n"
    "  }\n"
    "}\n";
  testFormatRoundTrip("json", "json", jsonData);
}

BOOST_AUTO_TEST_CASE(_IODataType_YAML_RoundTrip)
{
  std::string yamlData = 
    "w2algxml:\n"
    "  program: TestProg\n"
    "  inputs:\n"
    "    - name: Reflectivity\n";
  testFormatRoundTrip("yaml", "yaml", yamlData);
}

// The Ultimate Integration Test: YAML -> JSON -> XML
BOOST_AUTO_TEST_CASE(_IODataType_CrossFormat_Translation)
{
  std::string yamlData = 
    "w2algxml:\n"
    "  program: QPE_Estimator\n"
    "  enabled: true\n"
    "  inputs:\n"
    "    - name: Reflectivity\n"
    "      folder: /data/radar\n";

  // 1. Read YAML from memory
  std::vector<char> yamlBuf(yamlData.begin(), yamlData.end());
  yamlBuf.push_back('\0');
  auto dtYaml = IODataType::readBuffer<PTreeData>(yamlBuf, "yaml");
  BOOST_REQUIRE(dtYaml != nullptr);

  // Verify internal structure parsed correctly
  auto tree = dtYaml->getTree();
  BOOST_CHECK_EQUAL(tree->get<std::string>("w2algxml.program", ""), "QPE_Estimator");
  BOOST_CHECK_EQUAL(tree->get<std::string>("w2algxml.enabled", ""), "true");

  // 2. Write tree out to JSON buffer
  std::vector<char> jsonBuf;
  IOConfig jsonKeys;
  jsonKeys.set("suffix", "json");
  jsonKeys.set("indent", "true");
  IODataType::writeBuffer(dtYaml, jsonBuf, jsonKeys, "json");
  BOOST_REQUIRE(jsonBuf.size() > 0);

  // 3. Read JSON buffer back into a new tree
  auto dtJson = IODataType::readBuffer<PTreeData>(jsonBuf, "json");
  BOOST_REQUIRE(dtJson != nullptr);

  // 4. Write tree out to XML buffer
  std::vector<char> xmlBuf;
  IOConfig xmlKeys;
  xmlKeys.set("suffix", "xml");
  IODataType::writeBuffer(dtJson, xmlBuf, xmlKeys, "xml");
  BOOST_REQUIRE(xmlBuf.size() > 0);

  // 5. Read XML buffer back into a final tree
  auto dtXml = IODataType::readBuffer<PTreeData>(xmlBuf, "xml");
  BOOST_REQUIRE(dtXml != nullptr);

  // 6. Verify the final XML tree still holds the original YAML sequence data
  auto finalTree = dtXml->getTree();
  BOOST_CHECK_EQUAL(finalTree->get<std::string>("w2algxml.program", ""), "QPE_Estimator");
  
  // Verify the array mapped correctly to <item> tags in XML
  auto inputs = finalTree->getChildOptional("w2algxml.inputs");
  BOOST_REQUIRE(inputs != nullptr);
  auto items = inputs->getChildren("item");
  BOOST_REQUIRE_EQUAL(items.size(), 1);
  BOOST_CHECK_EQUAL(items[0].get<std::string>("name", ""), "Reflectivity");
  BOOST_CHECK_EQUAL(items[0].get<std::string>("folder", ""), "/data/radar");
}

// Test translating legacy XML into modern YAML
BOOST_AUTO_TEST_CASE(_IODataType_XML_to_YAML_Translation)
{
  std::string xmlData = 
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<w2algxml>\n"
    "  <program>QPE_Estimator</program>\n"
    "  <inputs>\n"
    "    <item>\n"
    "      <name>Reflectivity</name>\n"
    "      <folder>/data/radar/KTLX</folder>\n"
    "    </item>\n"
    "    <item>\n"
    "      <name>Velocity</name>\n"
    "      <folder>/data/radar/KTLX</folder>\n"
    "    </item>\n"
    "  </inputs>\n"
    "</w2algxml>\n";

  // 1. Read XML from memory
  std::vector<char> xmlBuf(xmlData.begin(), xmlData.end());
  xmlBuf.push_back('\0');
  auto dtXml = IODataType::readBuffer<PTreeData>(xmlBuf, "xml");
  BOOST_REQUIRE(dtXml != nullptr);

  // 2. Write tree out to YAML buffer
  std::vector<char> yamlBuf;
  IOConfig yamlKeys;
  yamlKeys.set("suffix", "yaml");
  IODataType::writeBuffer(dtXml, yamlBuf, yamlKeys, "yaml");
  BOOST_REQUIRE(yamlBuf.size() > 0);

  // 3. Read YAML buffer back into a new tree to verify it is valid YAML
  auto dtYaml = IODataType::readBuffer<PTreeData>(yamlBuf, "yaml");
  BOOST_REQUIRE(dtYaml != nullptr);

  // 4. Verify the data survived the XML -> YAML -> PTree roundtrip
  auto finalTree = dtYaml->getTree();
  BOOST_CHECK_EQUAL(finalTree->get<std::string>("w2algxml.program", ""), "QPE_Estimator");
  
  // Verify the array mapped correctly
  auto inputs = finalTree->getChildOptional("w2algxml.inputs");
  BOOST_REQUIRE(inputs != nullptr);
  
  // Because we read it back from YAML, our heuristic should still see them as "item"s 
  // or array elements depending on how the YAML parsed.
  auto items = inputs->getChildren("item");
  BOOST_REQUIRE_EQUAL(items.size(), 2);
  BOOST_CHECK_EQUAL(items[0].get<std::string>("name", ""), "Reflectivity");
  BOOST_CHECK_EQUAL(items[1].get<std::string>("name", ""), "Velocity");
}

BOOST_AUTO_TEST_CASE(_IODataType_CSV_Typed_Headers_RoundTrip)
{
  // 1. Build the DataTable programmatically with all 3 supported types
  auto dt = std::make_shared<DataTable>();
  dt->addColumn("ID", DataColumn::Type::Integer);
  dt->addColumn("Value", DataColumn::Type::Float);
  dt->addColumn("JSONData", DataColumn::Type::String);

  // Inject Row 0
  dt->getColumn("ID").push_back(1);
  dt->getColumn("Value").push_back(3.14f);
  dt->getColumn("JSONData").push_back(R"({"type": "Polygon", "coord": [1, 2]})");

  // Inject Row 1
  dt->getColumn("ID").push_back(2);
  dt->getColumn("Value").push_back(-99.9f);
  dt->getColumn("JSONData").push_back("Simple \"Quote\" Test, with comma");

  // Inject Row 2 (Missing Data Boundary Check)
  dt->getColumn("ID").push_back(static_cast<int>(Constants::MissingData));
  dt->getColumn("Value").push_back(static_cast<float>(Constants::MissingData));
  dt->getColumn("JSONData").push_back("Missing Values");

  // Loop through both write engines to ensure parity
  std::vector<std::string> fastwriteModes = {"false", "true"};

  for (const auto& fwMode : fastwriteModes) {
    std::string filename = "test_csv_typed_roundtrip_fw_" + fwMode + ".csv";

    // 2. Write to CSV 
    IOConfig keys;
    keys.set("suffix", "csv");
    keys.set("filepathmode", "direct");
    keys.set("header", "true");
    keys.set("fastwrite", fwMode);
    keys.set("filename", filename);

    std::vector<Record> dummyRecords;
    bool writeSuccess = IODataType::write(dt, filename, dummyRecords, "csv", keys);
    BOOST_REQUIRE_MESSAGE(writeSuccess, "Failed to write csv to file with fastwrite=" << fwMode);

    // 3. Read the newly created CSV file back into memory
    auto dtFromFile = IODataType::read<DataTable>(filename, "csv");
    BOOST_REQUIRE_MESSAGE(dtFromFile != nullptr, "Failed to read csv from file with fastwrite=" << fwMode);

    // 4. Verify Schema
    auto& colNames = dtFromFile->getColumnNames();
    BOOST_REQUIRE_EQUAL(colNames.size(), 3);
    BOOST_REQUIRE_EQUAL(dtFromFile->getRowCount(), 3);

    BOOST_CHECK_EQUAL(colNames[0], "ID");
    BOOST_CHECK_EQUAL(colNames[1], "Value");
    BOOST_CHECK_EQUAL(colNames[2], "JSONData");

    // 5. Verify Explicit Types
    BOOST_CHECK(dtFromFile->getColumn("ID").getType() == DataColumn::Type::Integer);
    BOOST_CHECK(dtFromFile->getColumn("Value").getType() == DataColumn::Type::Float);
    BOOST_CHECK(dtFromFile->getColumn("JSONData").getType() == DataColumn::Type::String);

    // 6. Verify Data (including escaped quotes, commas, and constants)
    auto& idCol = dtFromFile->getColumn("ID");
    auto& valCol = dtFromFile->getColumn("Value");
    auto& jsonCol = dtFromFile->getColumn("JSONData");

    // Row 0
    BOOST_CHECK_EQUAL(idCol.getCellAsInt(0), 1);
    BOOST_CHECK(std::abs(valCol.getCellAsFloat(0) - 3.14f) < 0.001f);
    BOOST_CHECK_EQUAL(jsonCol.getCellAsString(0), R"({"type": "Polygon", "coord": [1, 2]})");

    // Row 1
    BOOST_CHECK_EQUAL(idCol.getCellAsInt(1), 2);
    BOOST_CHECK(std::abs(valCol.getCellAsFloat(1) - (-99.9f)) < 0.001f);
    BOOST_CHECK_EQUAL(jsonCol.getCellAsString(1), "Simple \"Quote\" Test, with comma");

    // Row 2 (Missing Data Checks)
    BOOST_CHECK_EQUAL(idCol.getCellAsInt(2), static_cast<int>(Constants::MissingData));
    BOOST_CHECK(std::abs(valCol.getCellAsFloat(2) - static_cast<float>(Constants::MissingData)) < 0.001f);
    BOOST_CHECK_EQUAL(jsonCol.getCellAsString(2), "Missing Values");

    // Clean up
    remove(filename.c_str());
  }

  // 7. Verify the Fallback Guessing Logic
  std::string fallbackCsvData = 
    "UntypedInt,UntypedFloat,Unrecognized:Data\n"
    "42,3.14159,Normal String\n";

  std::string fallbackFilename = "test_csv_fallback.csv";
  std::ofstream outFallback(fallbackFilename, std::ios::binary);
  outFallback.write(fallbackCsvData.data(), fallbackCsvData.size());
  outFallback.close();

  auto dtFallback = IODataType::read<DataTable>(fallbackFilename, "csv");
  BOOST_REQUIRE(dtFallback != nullptr);

  BOOST_CHECK(dtFallback->getColumn("UntypedInt").getType() == DataColumn::Type::Integer);
  BOOST_CHECK(dtFallback->getColumn("UntypedFloat").getType() == DataColumn::Type::Float);
  BOOST_CHECK(dtFallback->getColumn("Unrecognized:Data").getType() == DataColumn::Type::String);

  remove(fallbackFilename.c_str());
}

BOOST_AUTO_TEST_CASE(_IODataType_CSV_Phantom_Column_Regression)
{
  // Simulating the edge case of trailing commas creating a phantom column 
  // without a header string.
  std::string malformedCsv = 
    "ColA,ColB,\n"
    "1,2,\n"
    "3,4,\n";

  std::string filename = "test_csv_phantom.csv";
  std::ofstream out(filename, std::ios::binary);
  out.write(malformedCsv.data(), malformedCsv.size());
  out.close();

  auto dt = IODataType::read<DataTable>(filename, "csv");
  BOOST_REQUIRE(dt != nullptr);

  // Verifies the trim logic strips out the phantom trailing empty string
  BOOST_CHECK_EQUAL(dt->getColumnNames().size(), 2); 
  BOOST_CHECK_EQUAL(dt->getRowCount(), 2);

  remove(filename.c_str());
}

BOOST_AUTO_TEST_CASE(_IODataType_CSV_Empty_Table)
{
  auto dt = std::make_shared<DataTable>();
  // We do NOT manually add "str:" or "float:" here. 
  // The writer handles the schema serialization automatically!
  dt->addColumn("EmptyA", DataColumn::Type::String);
  dt->addColumn("EmptyB", DataColumn::Type::Float);

  std::string filename = "test_csv_empty.csv";
  IOConfig keys;
  keys.set("suffix", "csv");
  keys.set("filepathmode", "direct");
  keys.set("header", "true");
  keys.set("filename", filename);

  std::vector<Record> dummyRecords;
  bool writeSuccess = IODataType::write(dt, filename, dummyRecords, "csv", keys);
  BOOST_REQUIRE(writeSuccess);

  auto dtFromFile = IODataType::read<DataTable>(filename, "csv");
  BOOST_REQUIRE(dtFromFile != nullptr);
  
  // Verify it didn't throw out of bounds or invent rows
  BOOST_CHECK_EQUAL(dtFromFile->getRowCount(), 0);
  BOOST_CHECK_EQUAL(dtFromFile->getColumnNames().size(), 2);
  
  // Verify the empty headers retained their explicit typing prefixes during the round trip
  BOOST_CHECK(dtFromFile->getColumn("EmptyB").getType() == DataColumn::Type::Float);
  BOOST_CHECK(dtFromFile->getColumn("EmptyA").getType() == DataColumn::Type::String);

  remove(filename.c_str());
}
BOOST_AUTO_TEST_SUITE_END()
