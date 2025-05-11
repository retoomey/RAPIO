// Add this at top for any BOOST test
#include "rBOOSTTest.h"

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

  BOOST_CHECK_EQUAL(b.getNumBitsPerValue(), numBits);

  // Set all values in bitset
  for (size_t i = 0; i < numValues; i++) {
    b.set(i, i);
  }
  // Test ends
  b.set(0, 31);
  b.set(numValues - 1, 7);
  BOOST_CHECK_EQUAL(b.get(0), 31);
  BOOST_CHECK_EQUAL(b.get(numValues - 1), 7);
  // Test rest
  for (size_t i = 1; i < numValues - 1; i++) {
    auto z = b.get(i);
    BOOST_CHECK_EQUAL(z, i);
  }

  b.setAllBits();
  BOOST_CHECK_EQUAL(b.get(5), 31);

  b.clearAllBits();
  BOOST_CHECK_EQUAL(b.get(5), 0);

  // Multi dimension
  size_t counter = 0;
  const std::vector<size_t> dims = { 1, 2, 3 };
  Bitset bd(dims, numBits);

  for (size_t x = 0; x < 1; x++) {
    for (size_t y = 0; y < 2; y++) {
      for (size_t z = 0; z < 3; z++) {
        // size_t i = bd.getIndex3D(x,y,z);
        size_t i = bd.getIndex({ x, y, z });
        // std::cout << " IN " << i << " " << counter << "\n";
        bd.set(i, counter);
        counter++;
      }
    }
  }

  bool good = true;

  counter = 0;
  for (size_t x = 0; x < 1; x++) {
    for (size_t y = 0; y < 2; y++) {
      for (size_t z = 0; z < 3; z++) {
        // size_t i = bd.getIndex3D(x,y,z);
        size_t i = bd.getIndex({ x, y, z });
        // std::cout << " OUT " << i << " " << bd.get(i) << "\n";
        good &= (bd.get(i) == counter);
        counter++;
      }
    }
  }

  BOOST_CHECK_EQUAL(true, good);
}

BOOST_AUTO_TEST_SUITE_END();
