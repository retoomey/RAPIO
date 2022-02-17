// Add this at top for any BOOST test
#include <boost/test/unit_test.hpp>

#include "rArray.h"

using namespace rapio;

BOOST_AUTO_TEST_SUITE(ARRAY)

BOOST_AUTO_TEST_CASE(ARRAY_CREATION)
{
  // Creating each type and basic fill check

  // 1d float
  auto float1     = Arrays::CreateFloat1D(100);
  auto float1dims = float1->dims();

  BOOST_CHECK_EQUAL(float1dims.size(), 1);
  BOOST_CHECK_EQUAL(float1dims[0], 100);
  float1->fill(200);
  auto& f1 = float1->ref();

  BOOST_CHECK_EQUAL(f1[50], 200);

  // 2d float
  auto float2     = Arrays::CreateFloat2D(50, 60);
  auto float2dims = float2->dims();

  BOOST_CHECK_EQUAL(float2dims.size(), 2);
  BOOST_CHECK_EQUAL(float2dims[0], 50);
  BOOST_CHECK_EQUAL(float2dims[1], 60);
  float2->fill(100);
  auto& f2 = float2->ref();

  BOOST_CHECK_EQUAL(f2[49][59], 100);

  // 3d float
  auto float3     = Arrays::CreateFloat3D(5, 10, 8);
  auto float3dims = float3->dims();

  BOOST_CHECK_EQUAL(float3dims.size(), 3);
  BOOST_CHECK_EQUAL(float3dims[0], 5);
  BOOST_CHECK_EQUAL(float3dims[1], 10);
  BOOST_CHECK_EQUAL(float3dims[2], 8);
  float3->fill(99);
  auto& f3 = float3->ref();

  BOOST_CHECK_EQUAL(f3[0][0][0], 99);
  BOOST_CHECK_EQUAL(f3[4][9][7], 99);

  // Some assignment checking
  f3[0][0][0] = 75;
  f3[1][0][0] = 80;
  f3[0][1][0] = 90;
  f3[0][0][1] = 92;

  auto& f4 = float3->ref();

  BOOST_CHECK_EQUAL(f4[0][0][0], 75);
  BOOST_CHECK_EQUAL(f4[1][0][0], 80);
  BOOST_CHECK_EQUAL(f4[0][1][0], 90);
  BOOST_CHECK_EQUAL(f4[0][0][1], 92);
}

BOOST_AUTO_TEST_CASE(ARRAY_RESIZE)
{
  // Set resizing 1d float.  It's all boost wrapped
  // so eh maybe I'll check 2d and 3d later
  auto float1 = Arrays::CreateFloat1D(100);

  float1->fill(50);
  float1->resize({ 200 });
  auto& f1 = float1->ref();

  BOOST_CHECK_EQUAL(f1[99], 50);
  float1->fill(100);
  BOOST_CHECK_EQUAL(f1[199], 100);
  float1->resize({ 5 });
  BOOST_CHECK_EQUAL(f1[4], 100);
}

BOOST_AUTO_TEST_SUITE_END();
