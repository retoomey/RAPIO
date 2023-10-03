#include <rBitset.h>
#include <rError.h>

#include <cmath>

using namespace rapio;
using namespace std;

bool BitsetOutSettings::myFull = false;

namespace {
/** Common code here to reduce mistakes */
template <typename T>
std::unique_ptr<StaticVector>
createSized(size_t maxSize, size_t actualSize)
{
  LogInfo("Created " << sizeof(T) * 8 << " bit  storage for " << actualSize << " bits.\n");
  if (sizeof(T) * 8 < actualSize) {
    LogSevere("This is too small of storage it appears.\n");
  }
  return std::unique_ptr<StaticVector>(new StaticVectorT<T>(maxSize));
}
}

std::unique_ptr<StaticVector>
StaticVector::Create(size_t maxSize, bool trim)
{
  // Factory create the smallest possible for given size...

  const size_t size = StaticVector::smallestBitsToStore(maxSize);

  if (trim) {
    if (size <= 8) {
      return createSized<unsigned char>(maxSize, size);
    } else if (size <= 16) {
      return createSized<unsigned short>(maxSize, size);
    } else if (size <= 32) {
      return createSized<unsigned int>(maxSize, size);
    } else if (size <= 64) {
      return createSized<unsigned long>(maxSize, size);
    } else {
      LogSevere("Can't create trimmed storage size for " << size << " bits, using bitset (slower).\n");
    }
  }
  // We 'should' be using unique_ptr more..but it requires users know how to use them and convert to shared
  LogSevere("Created direct bitset " << size << " storage for " << size << " bits.\n");
  return std::unique_ptr<Bitset>(new Bitset(maxSize, size));
}

ostream&
rapio::operator << (ostream& os, const Bitset& b)
{
  os << "(Bitset:" << b.myNumValues << " values of " << b.myNumBitsPerValue << " bits each)";
  if (BitsetOutSettings::getFull()) {
    for (size_t i = 0; i < b.size(); i++) {
      os << i << ": ";
      b.bits(os, i);
      os << " " << b.get(i) << "\n";
    }
  }
  return (os);
}

unsigned int
StaticVector::smallestBitsToStore(unsigned int x)
{
  // if x is zero, return 1 since it takes 1 bit to store 0
  if (x == 0) {
    return 1;
  }
  // Calculate the log of base 2 of x and add 1 (because storing 0 to x-1)
  return static_cast<unsigned int>(std::log2(x) + 1);
}

bool
Bitset::writeBits(std::ofstream& file) const
{
  // Write a header of the dimensions/strides
  file.write(reinterpret_cast<const char *>(&myNumValues), sizeof(myNumValues));
  file.write(reinterpret_cast<const char *>(&myNumBitsPerValue), sizeof(myNumBitsPerValue));

  // Dimensions
  size_t aSize = myDimensions.size();

  file.write(reinterpret_cast<const char *>(&aSize), sizeof(aSize));
  for (auto d:myDimensions) {
    file.write(reinterpret_cast<const char *>(&d), sizeof(d));
  }

  // Strides
  aSize = myStrides.size();
  file.write(reinterpret_cast<const char *>(&aSize), sizeof(aSize));
  for (auto s:myStrides) {
    file.write(reinterpret_cast<const char *>(&s), sizeof(s));
  }

  // Bits at the end  (FIXME: this is bits not compressed to bytes)
  file << myBits;
  return true;
}

bool
Bitset::readBits(std::ifstream& file)
{
  // Write a header of the dimensions/strides
  file.read(reinterpret_cast<char *>(&myNumValues), sizeof(myNumValues));
  file.read(reinterpret_cast<char *>(&myNumBitsPerValue), sizeof(myNumBitsPerValue));

  // Dimensions
  size_t aSize;

  file.read(reinterpret_cast<char *>(&aSize), sizeof(aSize));
  myDimensions.resize(aSize);
  for (size_t i = 0; i < aSize; ++i) {
    file.read(reinterpret_cast<char *>(&myDimensions[i]), sizeof(size_t));
  }

  // Strides
  file.read(reinterpret_cast<char *>(&aSize), sizeof(aSize));
  myStrides.resize(aSize);
  for (size_t i = 0; i < aSize; ++i) {
    file.read(reinterpret_cast<char *>(&myStrides[i]), sizeof(size_t));
  }

  // Read the bits (FIXME: These are currently 1 byte per bit so we can (human) look at the files
  file >> myBits;
  return true;
}

ostream&
rapio::operator << (ostream& os, const BitsetOutSettings& b)
{
  // We don't actually print anything, just set flags for Bitset
  return os;
}
