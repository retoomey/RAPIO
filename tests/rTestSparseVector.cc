// Add this at top for any BOOST test
#include "rBOOSTTest.h"

/** Test for sparse vector and probably the bitset under it as well,
 * we might revamp all the simple data classes into a single test
 * suite at some point */
#include "rSparseVector.h"
#include "rBitset.h"

using namespace rapio;

BOOST_AUTO_TEST_SUITE(SPARSEVECTOR)

/** Test a rSparseVector */
BOOST_AUTO_TEST_CASE(SPARSEVECTOR_CREATION)
{
  class teststorage {
public:
    int somestuff = 0;
    size_t x      = 0;
    size_t y      = 0;
    size_t z      = 0;
  };

  const size_t maxSparseItems   = 10;
  const size_t maxSparseItems50 = maxSparseItems / 2;

  // Setting percentage and item values
  SparseVector<teststorage> sv(maxSparseItems);

  BOOST_CHECK_EQUAL(sv.getPercentFull(), 0);
  // Fill first half
  for (size_t i = 0; i < maxSparseItems50; i++) {
    teststorage A;
    A.somestuff = static_cast<int>(i);
    sv.set(i, A); // Probably copy/move here
  }
  BOOST_CHECK_EQUAL(sv.getPercentFull(), 50);
  // Fill second half
  for (size_t i = maxSparseItems50; i < maxSparseItems; i++) {
    teststorage A;
    A.somestuff = static_cast<int>(i);
    sv.set(i, A); // Probably copy/move here if passing non-pointer
  }
  BOOST_CHECK_EQUAL(sv.getPercentFull(), 100);
  for (size_t i = 0; i < maxSparseItems; i++) {
    auto A = sv.get(i); // getting back handle
    BOOST_CHECK_EQUAL(A->somestuff, static_cast<int>(i));
  }

  // Test a 3d array of the index
  SparseVector<teststorage> sv2({ 1, 2, 3 });

  BOOST_CHECK_EQUAL(SparseVector<teststorage>::calculateSize({ 20, 30, 40, 50 }), 20 * 30 * 40 * 50);

  // Set all values and check get index methods
  bool flag = true;

  for (size_t x = 0; x < 1; x++) {
    for (size_t y = 0; y < 2; y++) {
      for (size_t z = 0; z < 3; z++) {
        teststorage A;
        A.x = x;
        A.y = y;
        A.z = z;
        sv2.setDims({ x, y, z }, A);
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
        auto A = sv2.getDims({ x, y, z });
        if ((A->x != x) || (A->y != y) || (A->z != z)) {
          flag = false;
        }
      }
    }
  }
  BOOST_CHECK_EQUAL(flag, true);
}

BOOST_AUTO_TEST_SUITE_END();
