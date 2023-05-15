// Add this at top for any BOOST test
#include <boost/test/unit_test.hpp>

/** Test for bitset */
#include "rBitset.h"

using namespace rapio;

BOOST_AUTO_TEST_SUITE(BITSET)

/** Test a rBitset */
BOOST_AUTO_TEST_CASE(BITSET_CREATION)
{
  const size_t numValues = 10;
  const size_t numBits   = 5;
  Bitset b(numValues, numBits);

  // Check we got the right size
  BOOST_CHECK_EQUAL(b.size(), numValues);

  // Should be 5 unsigned bits 11111 == 31
  BOOST_CHECK_EQUAL(b.calculateMaxValue(), 31);

  // Should be smallest bits to store 300 or 9 bits  (100101100)
  BOOST_CHECK_EQUAL(b.smallestBitsToStore(300), 9);
  // Should be smallest bits to store 511 or 9 bits  (111111111)
  BOOST_CHECK_EQUAL(b.smallestBitsToStore(300), 9);
  // Should be smallest bits to store 512 or 10 bits  (1000000000)
  BOOST_CHECK_EQUAL(b.smallestBitsToStore(512), 10);

  BOOST_CHECK_EQUAL(b.getNumBits(), numBits);

  // Set all values in bitset
  for (size_t i = 0; i < numValues; i++) {
    b.set<char>(i, i);
  }
  // Test ends
  b.set(0, 31);
  b.set(numValues - 1, 7);
  BOOST_CHECK_EQUAL(b.get<short>(0), 31);
  BOOST_CHECK_EQUAL(b.get<int>(numValues - 1), 7);
  // Test rest
  for (size_t i = 1; i < numValues - 1; i++) {
    auto z = b.get<long>(i);
    BOOST_CHECK_EQUAL(z, i);
  }

  b.setAllBits();
  BOOST_CHECK_EQUAL(b.get<size_t>(5), 31);

  b.clearAllBits();
  BOOST_CHECK_EQUAL(b.get<size_t>(5), 0);
}

BOOST_AUTO_TEST_SUITE_END();
