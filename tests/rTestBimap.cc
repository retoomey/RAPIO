// Add this at top for any BOOST test
#include <boost/test/unit_test.hpp>

#include "rBimap.h"

using namespace rapio;

BOOST_AUTO_TEST_SUITE(BIMAPTEST)

BOOST_AUTO_TEST_CASE(TestInsertAndGetKey)
{
  BimapBoost<std::string, int> bimap;

  bimap.insert("key1", 1);
  bimap.insert("key2", 2);
  bimap.insert("key3", 3);

  BOOST_CHECK_EQUAL(bimap.getKey(2), "key2");
  BOOST_CHECK_EQUAL(bimap.getKey(4), "");
}

BOOST_AUTO_TEST_CASE(TestGetValue)
{
  BimapBoost<std::string, int> bimap;

  bimap.insert("key1", 1);
  bimap.insert("key2", 2);
  bimap.insert("key3", 3);

  BOOST_CHECK_EQUAL(bimap.getValue("key2"), 2);
  BOOST_CHECK_EQUAL(bimap.getValue("key4"), 0);
}

BOOST_AUTO_TEST_SUITE_END();
