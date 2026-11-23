#include <rValueCompressor.h>

using namespace rapio;
using namespace std;

int
ValueCompressor::calculateBytesNeeded(double minVal, double maxVal, int precision)
{
  const double range = std::max(0.0, maxVal - minVal);
  int requiredBytes  = static_cast<int>(std::ceil(std::log2(range * precision + 1.0) / 8.0));

  return std::max(1, requiredBytes);
}
