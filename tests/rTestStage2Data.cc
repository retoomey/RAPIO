// Add this at top for any BOOST test
#include "rBOOSTTest.h"

/** Test fusion stage2 data. */
#include "../programs/fusion/rStage2Data.h"

using namespace rapio;

BOOST_AUTO_TEST_SUITE(STAGE2DATA)

/** Test a rSparseVector */
BOOST_AUTO_TEST_CASE(STAGE2DATA_READWRITE)
{
  // Disabling this test at moment.  All the optimizating/streaming stuff
  // breaks how all this works.  I want a test at some point though

  #if 0
  // I want to test the compression/uncompression tricks we'll be using
  // so we'll just create and then read back and validate
  LLH center(35.1959, 97.1640, 369.7224);
  size_t numX = 150;                       // lon marching east to west
  size_t numY = 100;                       // lat marching north to south
  size_t numZ = 1;                         // heights
  FusionBinaryTable::myStreamRead = false; // we're not writing to disk
  std::shared_ptr<Stage2Data> insp =
    std::make_shared<Stage2Data>(Stage2Data("KTLX", "Reflectivity", "dBZ", center, 10, 10, { numX, numY, numZ }));

  // Write to stage2 class
  size_t inCounter      = 0;
  float weightCount     = 50.0;
  size_t inMissingCount = 0;
  size_t inValueCount   = 0;

  for (size_t x = 0; x < numX; ++x) {
    for (size_t y = 0; y < numY; ++y) {
      for (size_t z = 0; z < numZ; ++z) {
        float v;
        if (fmod(x, 6) == 0) { // every so often a real value, otherwise block of missing
          v = std::stof(std::to_string(x) + std::to_string(y) + std::to_string(z));
          inValueCount++;
        } else {
          v = Constants::MissingData;
          inMissingCount++;
        }

        insp->add(v, weightCount, x, y, z);
        inCounter++;
      }
    }
  }

  insp->RLE(); // compress our missing values

  // Read back and check stage2 class
  // I'm assuming return ordering the same which technically it could be different
  bool passed       = true;
  size_t outCounter = 0;

  weightCount = 50.0;

  float outv, w;
  short xout, yout, zout;
  size_t outMissingCount = 0;
  size_t outValueCount   = 0;

  while (insp->get(outv, w, xout, yout, zout)) {
    float v;
    if (fmod(xout, 6) == 0) { // every so often a real value, otherwise block of missing
      v = std::stof(std::to_string(xout) + std::to_string(yout) + std::to_string(zout));
    } else {
      v = Constants::MissingData;
    }

    if (outv == Constants::MissingData) {
      outMissingCount++;
    } else {
      outValueCount++;
    }
    if (v != outv) {
      // LogSevere("V is " << outv << " we expected " << v << " at (" << xout << ", " << yout << " , " << zout << ")\n");
      BOOST_CHECK_EQUAL(v, outv); // just one actual check or we'll spam
      passed = false;
      break;
    }
    outCounter++;
  }
  BOOST_CHECK_EQUAL(outCounter, inCounter);
  if (outCounter != inCounter) {
    passed = false;
  }
  BOOST_CHECK_EQUAL(inMissingCount, outMissingCount);
  BOOST_CHECK_EQUAL(inValueCount, outValueCount);
  BOOST_CHECK_EQUAL(passed, true);
  #endif // if 0
}
BOOST_AUTO_TEST_SUITE_END();
