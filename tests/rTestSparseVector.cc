// Add this at top for any BOOST test
#include <boost/test/unit_test.hpp>

/** Test for sparse vector and probably the bitset under it as well,
 * we might revamp all the simple data classes into a single test
 * suite at some point */
#include "rSparseVector.h"
#include "rBitset.h"

using namespace rapio;

BOOST_AUTO_TEST_SUITE(SPARSEVECTORBITSET)

/** Test a rBitset */
BOOST_AUTO_TEST_CASE(BITSET)
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

/** Test a rSparseVector */
BOOST_AUTO_TEST_CASE(SPARSEVECTOR)
{
  class teststorage {
public:
    int somestuff;
    int x;
    int y;
    int z;
  };

  const size_t maxSparseItems   = 10;
  const size_t maxSparseItems50 = maxSparseItems / 2;

  // Setting percentage and item values
  SparseVector<teststorage> sv(maxSparseItems);

  BOOST_CHECK_EQUAL(sv.getPercentFull(), 0);
  // Fill first half
  for (int i = 0; i < maxSparseItems50; i++) {
    teststorage A;
    A.somestuff = i;
    sv.set(i, A); // Probably copy/move here
  }
  BOOST_CHECK_EQUAL(sv.getPercentFull(), 50);
  // Fill second half
  for (int i = maxSparseItems50; i < maxSparseItems; i++) {
    teststorage A;
    A.somestuff = i;
    sv.set(i, A); // Probably copy/move here if passing non-pointer
  }
  BOOST_CHECK_EQUAL(sv.getPercentFull(), 100);
  for (int i = 0; i < maxSparseItems; i++) {
    auto A = sv.get(i); // getting back handle
    BOOST_CHECK_EQUAL(A->somestuff, i);
  }

  // Test a 3d array of the index
  SparseVectorDims<teststorage> sv2({ 1, 2, 3 });

  BOOST_CHECK_EQUAL(SparseVectorDims<teststorage>::calculateSize({ 20, 30, 40, 50 }), 20 * 30 * 40 * 50);

  // Set all values and check get index methods
  bool flag = true;

  for (size_t x = 0; x < 1; x++) {
    for (size_t y = 0; y < 2; y++) {
      for (size_t z = 0; z < 3; z++) {
        teststorage A;
        A.x = x;
        A.y = y;
        A.z = z;
        sv2.set({ x, y, z }, A);
        if (sv2.getIndex({ x, y, z }) != sv2.getIndex3D(x, y, z)) {
          flag = false;
        }
        // BOOST_CHECK_EQUAL(sv2.getIndex({x,y,z}), sv2.getIndex3D(x,y,z));
      }
    }
  }

  // The getIndex and getIndex3D should return same index value for all points
  // if not, this test fails. Enable the test in the loop to check if all or some point fails.
  BOOST_CHECK_EQUAL(flag, true);

  flag = true;
  for (size_t x = 0; x < 1; x++) {
    for (size_t y = 0; y < 2; y++) {
      for (size_t z = 0; z < 3; z++) {
        auto A = sv2.get({ x, y, z });
        if ((A->x != x) || (A->y != y) || (A->z != z)) {
          flag = false;
        }
      }
    }
  }
  BOOST_CHECK_EQUAL(flag, true);
}

BOOST_AUTO_TEST_SUITE_END();
