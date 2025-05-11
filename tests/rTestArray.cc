// Add this at top for any BOOST test
#include "rBOOSTTest.h"

#include "rArray.h"
#include "rDataArray.h"

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

  auto& f3t = float3->ref();

  BOOST_CHECK_EQUAL(f3t[0][0][0], 75);
  BOOST_CHECK_EQUAL(f3t[1][0][0], 80);
  BOOST_CHECK_EQUAL(f3t[0][1][0], 90);
  BOOST_CHECK_EQUAL(f3t[0][0][1], 92);

  // Add Clone (typed) test
  auto float4 = float3->Clone();

  float3->fill(50); // make old float different
  auto float4dims = float4->dims();

  BOOST_CHECK_EQUAL(float4dims.size(), 3);
  BOOST_CHECK_EQUAL(float4dims[0], 5);
  BOOST_CHECK_EQUAL(float4dims[1], 10);
  BOOST_CHECK_EQUAL(float4dims[2], 8);
  auto& f4 = float4->ref();

  BOOST_CHECK_EQUAL(f4[0][0][0], 75);
  BOOST_CHECK_EQUAL(f4[4][9][7], 99);

  // Add Clone (base generic) test
  auto float5      = float4->CloneBase();
  auto specialized = std::dynamic_pointer_cast<Array<float, 3> >(float5);

  BOOST_REQUIRE(specialized);
  if (specialized) {
    auto& f5 = specialized->ref();
    BOOST_CHECK_EQUAL(f5[0][0][0], 75);
    BOOST_CHECK_EQUAL(f5[4][9][7], 99);
  }
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

BOOST_AUTO_TEST_CASE(DATAARRAY_CREATION)
{
  // DataArray is still coupled with DataGrid, though
  // it could be more independent and more like Array, since
  // it really is just an Array plus attributes and dimension indexes
  // So this feels messier than it should be.
  // FIXME: Maybe clean up the interface make it more like Array
  auto darray1 = std::make_shared<DataArray>();
  auto array1  = darray1->init<float, 2>("Test", "m/s", DataArrayType::FLOAT, { 100, 100 }, { 0, 1 });

  // -----------------------------------------------
  // Set some attributes
  darray1->setString("Field", "Original");
  // Set some data
  array1->fill(50);

  // -----------------------------------------------
  // Clone it (should now have the attribute and the array copy)
  auto darray2 = darray1->Clone();

  // -----------------------------------------------
  // Now change the original attribute and array values
  darray1->setString("Field", "Changed");
  array1->fill(20);

  // -----------------------------------------------
  // Check original DataArray...
  auto& a1 = array1->ref();

  BOOST_CHECK_EQUAL(a1[99][99], 20);
  std::string output = "fail";

  darray1->getString("Field", output);
  BOOST_CHECK_EQUAL(output, "Changed");

  // -----------------------------------------------
  // Check copy ...
  auto array2 = darray2->getArrayDerived<float, 2>();

  BOOST_REQUIRE(array2);
  if (array2) {
    auto& a2 = array2->ref();
    BOOST_CHECK_EQUAL(a2[99][99], 50);
    std::string output = "fail";
    darray2->getString("Field", output);
    BOOST_CHECK_EQUAL(output, "Original");
  }
}

BOOST_AUTO_TEST_SUITE_END();
