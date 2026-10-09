// Add this at top for any BOOST test
#include "rBOOSTTest.h"

/** Test the fusion stage2 storage/streaming table.

    Stage2Data stores its data in a FusionBinaryTable.  The table supports a
    "stream read" mode (FusionBinaryTable::myStreamRead): readBlock only records
    the data offset, then get() walks rows straight from the file, expanding the
    run-length encoded missing values on the fly.  This is what keeps memory low
    for the massive fusion grids.  Here we build a table, write it to disk, then
    read it back in streaming mode and validate every row. */
#include "rFusionBinaryTable.h"
#include "rConstants.h"
#include "rTime.h"
#include "rLLH.h"

#include <cstdio>
#include <string>
#include <vector>
#include <cmath>

using namespace rapio;

BOOST_AUTO_TEST_SUITE(STAGE2DATA)

BOOST_AUTO_TEST_CASE(STAGE2DATA_STREAMREADWRITE)
{
  const size_t numValues      = 500; // non-missing (value) rows
  const size_t numMissingRuns = 20;  // RLE compressed missing runs
  const std::string filename  = "test_stage2_stream.raw";

  // Make sure we start in non-streaming (in-memory) mode
  FusionBinaryTable::myStreamRead = false;

  auto table = std::make_shared<FusionBinaryTable>();
  table->setString("Radarname", "KTLX");
  table->setString("Typename", "Reflectivity");
  table->setUnits("dBZ");
  table->setLocation(LLH(35.1959, 97.1640, 369.7224));
  table->setTime(Time(1609459200, 0.5));

  // ------------------------------------------------------------
  // Build the value rows (numerator, denominator, x, y, z)
  std::vector<float> expN, expD;
  std::vector<short> expX, expY;
  std::vector<char> expZ;

  for (size_t i = 0; i < numValues; ++i) {
    float n = static_cast<float>(i) * 0.25f;
    float d = 2.0f;
    short x = static_cast<short>(i % 150);
    short y = static_cast<short>(i / 150);
    short z = static_cast<short>(i % 3);

    table->add(n, d, x, y, z);

    expN.push_back(n);
    expD.push_back(d);
    expX.push_back(x);
    expY.push_back(y);
    expZ.push_back(static_cast<char>(z));
  }

  // ------------------------------------------------------------
  // Build the RLE missing runs (xStart, y, z, runLength)
  std::vector<short> expXm, expYm, expLm;
  std::vector<char> expZm;
  size_t expectedMissingExpanded = 0;

  for (size_t r = 0; r < numMissingRuns; ++r) {
    size_t x = r * 5;
    size_t y = 10;
    size_t z = 0;
    size_t l = r + 1; // run lengths 1 .. numMissingRuns

    table->addMissing(x, y, z, l);

    expXm.push_back(static_cast<short>(x));
    expYm.push_back(static_cast<short>(y));
    expZm.push_back(static_cast<char>(z));
    expLm.push_back(static_cast<short>(l));
    expectedMissingExpanded += l;
  }

  BOOST_REQUIRE_EQUAL(table->getValueSize(), numValues);
  BOOST_REQUIRE_EQUAL(table->getMissingSize(), numMissingRuns);

  // ------------------------------------------------------------
  // Write it out to disk
  FILE * fp = fopen(filename.c_str(), "wb");
  BOOST_REQUIRE(fp != nullptr);
  BOOST_REQUIRE(table->writeBlock(fp));
  fclose(fp);

  // ------------------------------------------------------------
  // Read it back in streaming mode.  This does not load the arrays;
  // get() pulls rows directly from the file.
  FusionBinaryTable::myStreamRead = true;

  auto read = std::make_shared<FusionBinaryTable>();
  fp = fopen(filename.c_str(), "rb");
  BOOST_REQUIRE(fp != nullptr);
  BOOST_REQUIRE(read->readBlock(filename, fp));
  fclose(fp); // get() opens its own handle from the recorded offset

  BOOST_CHECK_EQUAL(read->getValueSize(), numValues);
  BOOST_CHECK_EQUAL(read->getMissingSize(), numMissingRuns);

  std::string radarName, typeName, units;
  read->getString("Radarname", radarName);
  read->getString("Typename", typeName);
  units = read->getUnits();

  BOOST_CHECK_EQUAL(radarName, "KTLX");
  BOOST_CHECK_EQUAL(typeName, "Reflectivity");
  BOOST_CHECK_EQUAL(units, "dBZ");

  // In streaming mode nothing should be materialized into the vectors
  BOOST_CHECK_EQUAL(read->myNums.size(), (size_t) 0);
  BOOST_CHECK_EQUAL(read->myXs.size(), (size_t) 0);

  // ------------------------------------------------------------
  // Walk the stream and validate.  Values come first, then the
  // expanded missing runs.
  float n, d;
  short x, y, z;
  size_t outValueCount = 0;
  size_t outMissingCount = 0;
  bool valuesOk  = true;
  bool missingOk = true;

  for (size_t i = 0; i < numValues; ++i) {
    BOOST_REQUIRE(read->get(n, d, x, y, z));
    if ((n != expN[i]) || (d != expD[i]) || (x != expX[i]) || (y != expY[i]) || (static_cast<char>(z) != expZ[i])) {
      valuesOk = false;
    }
    outValueCount++;
  }

  size_t runIndex = 0;
  size_t posInRun = 0;

  for (size_t i = 0; i < expectedMissingExpanded; ++i) {
    BOOST_REQUIRE(read->get(n, d, x, y, z));

    if (n != Constants::MissingData) {
      missingOk = false;
    }
    const short expectX = static_cast<short>(expXm[runIndex] + posInRun);
    const short expectY = expYm[runIndex];
    const char expectZ  = expZm[runIndex];

    if ((x != expectX) || (y != expectY) || (static_cast<char>(z) != expectZ)) {
      missingOk = false;
    }

    if (++posInRun >= static_cast<size_t>(expLm[runIndex])) {
      posInRun = 0;
      runIndex++;
    }
    outMissingCount++;
  }

  // Stream should be exhausted now
  BOOST_CHECK_EQUAL(read->get(n, d, x, y, z), false);

  BOOST_CHECK_EQUAL(outValueCount, numValues);
  BOOST_CHECK_EQUAL(outMissingCount, expectedMissingExpanded);
  BOOST_CHECK_EQUAL(valuesOk, true);
  BOOST_CHECK_EQUAL(missingOk, true);

  // ------------------------------------------------------------
  // Introspection API should also describe the table layout
  auto info = table->getTableInfo();
  BOOST_REQUIRE_EQUAL(info.size(), (size_t) 2);
  BOOST_CHECK_EQUAL(info[0].name, "Data");
  BOOST_CHECK_EQUAL(info[0].size, numValues);
  BOOST_CHECK_EQUAL(info[1].name, "MissingData");
  BOOST_CHECK_EQUAL(info[1].size, numMissingRuns);

  auto infoX = table->getShortVector("X");
  BOOST_REQUIRE_EQUAL(infoX.size(), numValues);
  BOOST_CHECK_EQUAL(infoX[0], expX[0]);
  BOOST_CHECK_EQUAL(infoX[numValues - 1], expX[numValues - 1]);

  // Reset global mode and clean up
  FusionBinaryTable::myStreamRead = false;
  remove(filename.c_str());
}

BOOST_AUTO_TEST_SUITE_END()
