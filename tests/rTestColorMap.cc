#include "rBOOSTTest.h"
#include "rConfigColorMap.h"
#include "rColorMap.h"
#include "rOS.h"
#include "rURL.h"
#include "rIOXML.h"
#include "rFactory.h"
#include <fstream>
#include <map>

using namespace rapio;

BOOST_AUTO_TEST_SUITE(COLORMAP_TESTS)

// ============================================================================
// TEST 1: The Rewritten PAL Parser (.pal files)
// ============================================================================
BOOST_AUTO_TEST_CASE(TEST_PAL_FILE_PARSING)
{
  std::string tempPalFile = OS::getCurrentDirectory() + "/test_palette.pal";
  std::ofstream out(tempPalFile);
  out << "R 0 100 255 0 0\n";
  out << "R 100 200 0 255 0\n";
  out << "B 200 500\n";
  out << "L 0 0 255\n";     // Blue
  out << "L 255 255 0\n";   // Yellow
  out << "L 0 255 255\n";   // Cyan
  out.close();

  URL palUrl(tempPalFile);
  auto colorMap = ConfigColorMap::readPalColorMap(palUrl);

  BOOST_REQUIRE_MESSAGE(colorMap != nullptr, "ColorMap failed to load from PAL file.");

  unsigned char r, g, b, a;

  colorMap->getColor(5.0, r, g, b, a);
  BOOST_CHECK_EQUAL(r, 255); BOOST_CHECK_EQUAL(g, 0); BOOST_CHECK_EQUAL(b, 0);

  colorMap->getColor(15.0, r, g, b, a);
  BOOST_CHECK_EQUAL(r, 0); BOOST_CHECK_EQUAL(g, 255); BOOST_CHECK_EQUAL(b, 0);

  colorMap->getColor(25.0, r, g, b, a);
  BOOST_CHECK_EQUAL(r, 0); BOOST_CHECK_EQUAL(g, 0); BOOST_CHECK_EQUAL(b, 255);

  colorMap->getColor(35.0, r, g, b, a);
  BOOST_CHECK_EQUAL(r, 255); BOOST_CHECK_EQUAL(g, 255); BOOST_CHECK_EQUAL(b, 0);

  colorMap->getColor(45.0, r, g, b, a);
  BOOST_CHECK_EQUAL(r, 0); BOOST_CHECK_EQUAL(g, 255); BOOST_CHECK_EQUAL(b, 255);

  OS::deleteFile(tempPalFile);
}

// ============================================================================
// TEST 2: WDSS2 XML Parser
// ============================================================================
BOOST_AUTO_TEST_CASE(TEST_W2_COLORMAP_PARSING)
{
  std::shared_ptr<IOXML> xmlParser = std::make_shared<IOXML>();
  Factory<IODataType>::introduce("xml", xmlParser);

  std::string tempW2File = OS::getCurrentDirectory() + "/test_w2.xml";
  std::ofstream out(tempW2File);
  out << "<colorMap>\n";
  out << "  <colorBin upperBound=\"10.0\" name=\"Low\">\n";
  out << "    <color r=\"0x00\" g=\"0x00\" b=\"0xFF\" a=\"0xFF\"/>\n";
  out << "  </colorBin>\n";
  out << "  <colorBin upperBound=\"20.0\" name=\"High\">\n";
  out << "    <color r=\"255\" g=\"0\" b=\"0\" a=\"255\"/>\n";
  out << "  </colorBin>\n";
  out << "</colorMap>\n";
  out.close();

  URL w2Url(tempW2File);
  auto colorMap = ConfigColorMap::readW2ColorMap(w2Url);

  BOOST_REQUIRE_MESSAGE(colorMap != nullptr, "ColorMap failed to load. Is the XML parser registered?");

  unsigned char r, g, b, a;

  colorMap->getColor(5.0, r, g, b, a);
  BOOST_CHECK_EQUAL(r, 0); BOOST_CHECK_EQUAL(g, 0); BOOST_CHECK_EQUAL(b, 255);

  colorMap->getColor(15.0, r, g, b, a);
  BOOST_CHECK_EQUAL(r, 255); BOOST_CHECK_EQUAL(g, 0); BOOST_CHECK_EQUAL(b, 0);

  OS::deleteFile(tempW2File);
}

// ============================================================================
// TEST 3: ParaView XML Parser
// ============================================================================
BOOST_AUTO_TEST_CASE(TEST_PARA_COLORMAP_PARSING)
{
  std::shared_ptr<IOXML> xmlParser = std::make_shared<IOXML>();
  Factory<IODataType>::introduce("xml", xmlParser);

  std::string tempParaFile = OS::getCurrentDirectory() + "/test_para.xml";
  std::ofstream out(tempParaFile);
  out << "<doc>\n";
  out << "  <ColorMap name=\"MyParaMap\">\n";
  out << "    <Point x=\"0.0\" o=\"1.0\" r=\"0.0\" g=\"0.0\" b=\"1.0\"/>\n";
  out << "    <Point x=\"1.0\" o=\"1.0\" r=\"1.0\" g=\"0.0\" b=\"0.0\"/>\n";
  out << "  </ColorMap>\n";
  out << "</doc>\n";
  out.close();

  std::map<std::string, std::string> attributes;
  attributes["name"] = "MyParaMap";
  attributes["lowdata"] = "0.0";
  attributes["highdata"] = "100.0";

  // Directly pass the URL to our new overload, bypassing search paths entirely
  URL paraUrl(tempParaFile);
  auto colorMap = ConfigColorMap::readParaColorMap(paraUrl, attributes);

  BOOST_REQUIRE_MESSAGE(colorMap != nullptr, "ColorMap failed to load. Is the XML parser registered?");

  unsigned char r, g, b, a;

  colorMap->getColor(0.0, r, g, b, a);
  BOOST_CHECK_EQUAL(r, 0); BOOST_CHECK_EQUAL(g, 0); BOOST_CHECK_EQUAL(b, 255);

  colorMap->getColor(100.0, r, g, b, a);
  BOOST_CHECK_EQUAL(r, 255); BOOST_CHECK_EQUAL(g, 0); BOOST_CHECK_EQUAL(b, 0);

  OS::deleteFile(tempParaFile);
}

BOOST_AUTO_TEST_SUITE_END()
