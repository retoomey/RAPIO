#include <rBitset.h>

#include <cmath>

using namespace rapio;
using namespace std;

bool BitsetOutSettings::myFull = false;

ostream&
rapio::operator << (ostream& os, const Bitset& b)
{
  os << "(Bitset:" << b.myNumValues << " values of " << b.myNumBits << " bits each)";
  if (BitsetOutSettings::getFull()) {
    for (size_t i = 0; i < b.size(); i++) {
      os << i << ": ";
      b.bits(os, i);
      os << " " << b.get<unsigned long long>(i) << "\n";
    }
  }
  return (os);
}

unsigned int
Bitset::smallestBitsToStore(unsigned int x)
{
  // if x is zero, return 1 since it takes 1 bit to store 0
  if (x == 0) {
    return 1;
  }
  // Calculate the log of base 2 of x and add 1 (because storing 0 to x-1)
  return static_cast<unsigned int>(std::log2(x) + 1);
}

ostream&
rapio::operator << (ostream& os, const BitsetOutSettings& b)
{
  // We don't actually print anything, just set flags for Bitset
  return os;
}
