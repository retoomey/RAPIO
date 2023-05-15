#include <rValueCompressor.h>

using namespace rapio;
using namespace std;

int
ValueCompressor::calculateBytesNeeded(double minVal, double maxVal, int precision)
{
  const double range = maxVal - minVal;
  int requiredBytes  = std::ceil(std::log2(range * precision + 1) / 8);

  return requiredBytes;
}
