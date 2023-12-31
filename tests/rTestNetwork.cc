// Add this at top for any BOOST test
#include <boost/test/unit_test.hpp>

/** Test for network */
#include "rCurlConnection.h"

#include <vector>
#include <iostream>

using namespace rapio;

BOOST_AUTO_TEST_SUITE(NETWORK)

/** Test a rBitset */
BOOST_AUTO_TEST_CASE(NETWORK_READ)
{
  Network::setNetworkEngine("CURL");

  std::vector<char> buffer;

  const std::string testPath = "http://example.com:80/index.html";

  // More complicated test and in boost it's still not working properly
  // const std::string testPath = "https://codeload.github.com/retoomey/RAPIO/zip/refs/heads/master"; // zip of RAPIO

  Network::read(testPath, buffer);
  const size_t curlBuf = buffer.size();

  BOOST_CHECK_GT(curlBuf, 0);
  #if 0
  std::cout << "Buffer back is " << buffer.size() << "\n";
  std::cout << "CURL test\n";
  std::cout << "XXX\n";
  for (size_t i = 0; i < buffer.size(); ++i) {
    std::cout << buffer[i];
  }
  std::cout << "XXX\n";
  #endif

  Network::setNetworkEngine("BOOST");

  std::vector<char> buffer2;
  Network::read(testPath, buffer2);
  const size_t boostBuf = buffer2.size();
  BOOST_CHECK_GT(buffer2.size(), 0);

  BOOST_CHECK_EQUAL(curlBuf, boostBuf);
  #if 0
  std::cout << "BOOST test\n";
  std::cout << "Buffer2 back is " << buffer2.size() << "\n";
  std::cout << "XXX\n";
  for (size_t i = 0; i < buffer2.size(); ++i) {
    std::cout << buffer2[i];
  }
  std::cout << "XXX\n";
  #endif
}

BOOST_AUTO_TEST_SUITE_END();
