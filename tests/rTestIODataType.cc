// Add this at top for any BOOST test
#include <boost/test/unit_test.hpp>

#include "rIODataType.h"
#include "rIOXML.h"
#include "rIOJSON.h"
#include "rFactory.h"
#include <iostream>
#include <fstream> // g++ 13/14

using namespace rapio;

BOOST_AUTO_TEST_SUITE(_IODataType_)

BOOST_AUTO_TEST_CASE(_IODataType_XML)
{
  // Introduce the XML reader to datatype
  // FIXME: If we make IOXML/IOJSON dynamic we'll have to
  // init the dynamic loading at some point
  // We'll come back add netcdf tests I think at some point
  std::shared_ptr<IOXML> xml = std::make_shared<IOXML>();
  Factory<IODataType>::introduce("xml", xml);

  // 1. Test reading XML from a buffer
  // Read the raw data the hard way so we can send it
  // to the builder to parse
  std::ostringstream buf;
  std::ifstream input("testxml.xml");

  buf << input.rdbuf();
  std::string gotit = buf.str().c_str();
  std::vector<char> buffer;

  for (auto x:gotit) {
    buffer.push_back(x);
  }
  // Code fixes this now buffer.push_back('\0');
  auto outREADBUFFER = IODataType::readBuffer<PTreeData>(buffer, "xml");

  BOOST_CHECK_EQUAL((outREADBUFFER != nullptr), true);

  // 2. Read a URL to a PTreeData using xml parser
  const URL loc("testxml.xml");
  // std::cerr << loc.toString() << "\n";
  auto outREADFILE = IODataType::read<PTreeData>(loc.toString(), "xml");

  BOOST_CHECK_EQUAL((outREADFILE != nullptr), true);
  // FIXME: could check actual nodes.  If it failed it should be zero

  // 3. Write the PTreeData out to a char buffer
  std::vector<char> bufferout;

  IODataType::writeBuffer(outREADFILE, bufferout, "xml");
  // std::cerr << "Writing buffer to xml gives -------------------------------------\n";
  // for(auto x:bufferout){
  //  std::cerr << x;
  // }
  // std::cerr << "\n\n";
  // for(auto x:buffer){
  //  std::cerr << x;
  // }
  // std::cerr << "\n";

  // 4. Create a new PTreeData item from the new char buffer
  auto newone = IODataType::readBuffer<PTreeData>(bufferout, "xml");

  BOOST_CHECK_EQUAL((newone != nullptr), true);

  BOOST_CHECK_EQUAL((bufferout.size() > 0), true);
  BOOST_CHECK_EQUAL((buffer.size() > 0), true);

  // Write the new bufferout to disk.  Git diff will now show
  // the difference if it happened.
  IODataType::write(newone, "testxml.xml");

  // 5. Read back URL to a PTreeData using xml parser
  auto outREADFILE2 = IODataType::read<PTreeData>(loc.toString(), "xml");

  BOOST_CHECK_EQUAL((outREADFILE2 != nullptr), true);
  // FIXME: could check actual nodes.  If it failed it should be zero

  // 6. Write the PTreeData out to a char buffer
  std::vector<char> bufferout2;

  IODataType::writeBuffer(outREADFILE2, bufferout2, "xml");
  // std::cerr << "Writing buffer to xml gives -------------------------------------\n";

  // Finally check the two buffers they should be equal size and > 0
  BOOST_CHECK_EQUAL((bufferout2.size() > 0), true);
  BOOST_CHECK_EQUAL(bufferout.size(), bufferout2.size());
  std::cerr << '*';
  for (auto x:bufferout) {
    std::cerr << x;
  }
  std::cerr << "*\n";
  std::cerr << '*';
  for (auto x:bufferout2) {
    std::cerr << x;
  }
  std::cerr << "*\n";

  /*
   * BOOST_CHECK_EQUAL(t.getSecondsSinceEpoch(), 100);
   * BOOST_CHECK(Arith::feq(t.getFractional(), 0.0));
   * t += TimeDuration::Minutes(2);
   * BOOST_CHECK(t.getSecondsSinceEpoch() == 220);
   */
}

BOOST_AUTO_TEST_CASE(_IODataType_JSON)
{
  // Introduce the JSON reader to datatype
  // FIXME: If we make IOXML/IOJSON dynamic we'll have to
  // init the dynamic loading at some point
  // We'll come back add netcdf tests I think at some point
  std::shared_ptr<IOJSON> json = std::make_shared<IOJSON>();
  Factory<IODataType>::introduce("json", json);

  // 1. Test reading JSON from a buffer
  // Read the raw data the hard way so we can send it
  // to the builder to parse
  std::ostringstream buf;
  std::ifstream input("testjson.json");

  buf << input.rdbuf();
  std::string gotit = buf.str().c_str();
  std::vector<char> buffer;

  for (auto x:gotit) {
    buffer.push_back(x);
  }
  // Code fixes this now buffer.push_back('\0');
  auto outREADBUFFER = IODataType::readBuffer<PTreeData>(buffer, "json");

  BOOST_CHECK_EQUAL((outREADBUFFER != nullptr), true);

  // 2. Read a URL to a PTreeData using xml parser
  const URL loc("testjson.json");
  // std::cerr << loc.toString() << "\n";
  auto outREADFILE = IODataType::read<PTreeData>(loc.toString(), "json");

  BOOST_CHECK_EQUAL((outREADFILE != nullptr), true);
  // FIXME: could check actual nodes.  If it failed it should be zero

  // 3. Write the PTreeData out to a char buffer
  std::vector<char> bufferout;

  IODataType::writeBuffer(outREADFILE, bufferout, "json");
  // std::cerr << "Writing buffer to json gives -------------------------------------\n";
  // for(auto x:bufferout){
  //  std::cerr << x;
  // }
  // std::cerr << "\n\n";
  // for(auto x:buffer){
  //  std::cerr << x;
  // }
  // std::cerr << "\n";

  // 4. Create a new PTreeData item from the new char buffer
  auto newone = IODataType::readBuffer<PTreeData>(bufferout, "json");

  BOOST_CHECK_EQUAL((newone != nullptr), true);

  BOOST_CHECK_EQUAL((bufferout.size() > 0), true);
  BOOST_CHECK_EQUAL((buffer.size() > 0), true);

  // Write the new bufferout to disk.  Git diff will now show
  // the difference if it happened.
  IODataType::write(newone, "testjson.json");

  // 5. Read back URL to a PTreeData using xml parser
  auto outREADFILE2 = IODataType::read<PTreeData>(loc.toString(), "json");

  BOOST_CHECK_EQUAL((outREADFILE2 != nullptr), true);
  // FIXME: could check actual nodes.  If it failed it should be zero

  // 6. Write the PTreeData out to a char buffer
  std::vector<char> bufferout2;

  IODataType::writeBuffer(outREADFILE2, bufferout2, "json");
  // std::cerr << "Writing buffer to json gives -------------------------------------\n";

  // Finally check the two buffers they should be equal size and > 0
  BOOST_CHECK_EQUAL((bufferout2.size() > 0), true);
  BOOST_CHECK_EQUAL(bufferout.size(), bufferout2.size());
  // std::cerr << '*';
  // for (auto x:bufferout) {
  //   std::cerr << x;
  // }
  // std::cerr << "*\n";
  // std::cerr << '*';
  // for (auto x:bufferout2) {
  //   std::cerr << x;
  // }
  // std::cerr << "*\n";
}

BOOST_AUTO_TEST_SUITE_END();
