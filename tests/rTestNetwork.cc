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

  Network::read("http://example.com:80/index.html", buffer);
  BOOST_CHECK_GT(buffer.size(), 0);
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
  Network::read("http://example.com:80/index.html", buffer2);
  BOOST_CHECK_GT(buffer2.size(), 0);
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
