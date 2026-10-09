// Add this at top for any BOOST test
#include "rBOOSTTest.h"

#include "rBinaryTable.h"
#include "rNetcdfBinaryTable.h"
#include "rConstants.h"
#include "rTime.h"
#include "rLLH.h"

#include <netcdf.h>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace rapio;

BOOST_AUTO_TEST_SUITE(_BinaryTable_)

/** Write a generic BinaryTable out to netcdf, read it back, and verify
 *  that the globals and each supported column type survive the trip. */
BOOST_AUTO_TEST_CASE(_BinaryTable_Netcdf_RoundTrip)
{
  auto table = std::make_shared<BinaryTable>();

  table->setTypeName("TestType");
  table->setTime(Time(1609459200, 0.25));
  table->setLocation(LLH(35.333, -97.277, 0.4));

  const float missing = static_cast<float>(Constants::MissingData);

  std::vector<float> floats           = { 1.0f, missing, -3.5f };
  std::vector<unsigned short> ushorts = { 10, 20, 65535 };
  std::vector<unsigned char> uchars   = { 0, 127, 255 };
  std::vector<std::string> strings    = { "alpha", "", "gamma" };

  table->addColumn("FloatCol", "dBZ", floats);
  table->addColumn("UShortCol", "meters", ushorts);
  table->addColumn("UCharCol", "count", uchars);
  table->addColumn("StringCol", "text", strings);

  const std::string filename = "test_binarytable_roundtrip.nc";
  int ncid = -1;

  // 1. Write it out
  BOOST_REQUIRE_EQUAL(nc_create(filename.c_str(), NC_CLOBBER | NC_NETCDF4, &ncid), NC_NOERR);

  NetcdfBinaryTable writer;
  IOConfig keys;

  BOOST_REQUIRE(writer.writeNETCDF(ncid, table, keys));
  BOOST_REQUIRE_EQUAL(nc_close(ncid), NC_NOERR);

  // 2. Read it back
  BOOST_REQUIRE_EQUAL(nc_open(filename.c_str(), NC_NOWRITE, &ncid), NC_NOERR);

  NetcdfBinaryTable reader;
  auto read = reader.readNETCDF(ncid, keys);

  BOOST_REQUIRE_EQUAL(nc_close(ncid), NC_NOERR);
  BOOST_REQUIRE(read != nullptr);

  auto rt = std::dynamic_pointer_cast<BinaryTable>(read);
  BOOST_REQUIRE(rt != nullptr);

  // 3. Globals should round trip
  BOOST_CHECK_EQUAL(rt->getTypeName(), "TestType");
  BOOST_CHECK_EQUAL(rt->getDataType(), "BinaryTable");
  BOOST_CHECK_EQUAL(rt->getTime().getSecondsSinceEpoch(), (time_t) 1609459200);

  // 4. Table layout should round trip
  auto info = rt->getTableInfo();
  BOOST_REQUIRE_EQUAL(info.size(), (size_t) 1);
  BOOST_CHECK_EQUAL(info[0].name, "rows");
  BOOST_CHECK_EQUAL(info[0].size, floats.size());
  BOOST_REQUIRE_EQUAL(info[0].columnNames.size(), (size_t) 4);
  BOOST_CHECK_EQUAL(info[0].columnNames[0], "FloatCol");
  BOOST_CHECK_EQUAL(info[0].columnNames[1], "UShortCol");
  BOOST_CHECK_EQUAL(info[0].columnNames[2], "UCharCol");
  BOOST_CHECK_EQUAL(info[0].columnNames[3], "StringCol");
  BOOST_CHECK_EQUAL(info[0].columnTypes[0], "float");
  BOOST_CHECK_EQUAL(info[0].columnTypes[1], "ushort");
  BOOST_CHECK_EQUAL(info[0].columnTypes[2], "uchar");
  BOOST_CHECK_EQUAL(info[0].columnTypes[3], "string");
  BOOST_CHECK_EQUAL(info[0].columnUnits[0], "dBZ");
  BOOST_CHECK_EQUAL(info[0].columnUnits[3], "text");

  // 5. Column data should round trip exactly
  auto rFloats = rt->getFloatVector("FloatCol");
  auto rUshorts = rt->getUShortVector("UShortCol");
  auto rUchars = rt->getUCharVector("UCharCol");
  auto rStrings = rt->getStringVector("StringCol");

  BOOST_REQUIRE_EQUAL(rFloats.size(), floats.size());
  BOOST_REQUIRE_EQUAL(rUshorts.size(), ushorts.size());
  BOOST_REQUIRE_EQUAL(rUchars.size(), uchars.size());
  BOOST_REQUIRE_EQUAL(rStrings.size(), strings.size());

  for (size_t i = 0; i < floats.size(); ++i) {
    BOOST_CHECK_EQUAL(rFloats[i], floats[i]);
  }
  for (size_t i = 0; i < ushorts.size(); ++i) {
    BOOST_CHECK_EQUAL(rUshorts[i], ushorts[i]);
  }
  for (size_t i = 0; i < uchars.size(); ++i) {
    BOOST_CHECK_EQUAL((int) rUchars[i], (int) uchars[i]);
  }
  for (size_t i = 0; i < strings.size(); ++i) {
    BOOST_CHECK_EQUAL(rStrings[i], strings[i]);
  }

  remove(filename.c_str());
}

BOOST_AUTO_TEST_SUITE_END()
