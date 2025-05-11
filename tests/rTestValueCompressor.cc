// Add this at top for any BOOST test
#include "rBOOSTTest.h"

#include "rValueCompressor.h"

#include <vector>

using namespace rapio;

BOOST_AUTO_TEST_SUITE(ValueCompressorTests)

BOOST_AUTO_TEST_CASE(TestValueConversion)
{
  double minVal = -123.45;
  double maxVal = 678.9;
  int precision = 2;

  ValueCompressor converter(minVal, maxVal, precision);

  std::vector<double> values       = { -123.45, 123.45, -12.34, 56.78, 9.99, 678.9 };
  std::vector<unsigned char> bytes = converter.compressVector(values);

  std::vector<double> reconstructed = converter.uncompressVector(bytes);

  BOOST_CHECK_EQUAL_COLLECTIONS(values.begin(), values.end(), reconstructed.begin(), reconstructed.end());
}

BOOST_AUTO_TEST_SUITE_END();
