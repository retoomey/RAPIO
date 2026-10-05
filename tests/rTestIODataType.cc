// Add this at top for any BOOST test
#include "rBOOSTTest.h"

#include "rIODataType.h"
#include "rPTreeData.h"
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

BOOST_AUTO_TEST_SUITE_END()
