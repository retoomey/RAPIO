#pragma once

#include <rData.h>
#include <rUtility.h>
#include <rDimensionMapper.h>

#include <boost/dynamic_bitset.hpp>
#include <cmath>

namespace rapio {
/** A class allowing different ways to store a std::vector of things choosing memory vs speed.
 * Note: Number of elements is fixed on creation at moment.
 *       Assumption is unsigned values at moment.
 * FIXME: Feels like this could merge with Array, but will require some work and testing there.
 *
 * @author Robert Toomey
 */
class StaticVector : public DimensionMapper {
public:

  /** Empty constructor */
  StaticVector() : DimensionMapper({ 0 }){ }

  /** Pass along dimensions if wanted */
  StaticVector(std::vector<size_t> dimensions) : DimensionMapper(dimensions){ }

  /** Single dimension */
  StaticVector(size_t dimension) : DimensionMapper(dimension){ }

  /** Factory method that returns a subclass of StaticVector depending on size.
   * If trim is true, pin to the smallest power of 2 that works.  This is a tradeoff on speed vs
   * memory.  For example, if you require 5 bits per element, than trim will return either
   * a bitset of 5 bits (smaller but slower), vs a std::vector<char> based implementation using
   * 8 bits per element.
   * Note this is meant for dynamic usage where you don't know the size in advance, otherwise
   * just make a std::vector<stuff> big enough.
   */
  static std::unique_ptr<StaticVector>
  Create(size_t maxSize, bool trim = true);

  /** (AI) Calculate the number of bits required to store a given unsigned number.  Usually
   * we can pass a max such as '255' to get 8 returned since 0-255 can be stored in 8 bits.
   * So if you want a Bitset to store X numbers from 0 to 255 in the smallest amount of space,
   * you could call Bitset(X, Bitset::smallestBitsToStore(255)), which would generate
   * a Bitset using 5 bits for each of the X numbers.
   */
  static unsigned int
  smallestBitsToStore(unsigned int x);

  /** Get size of the vector */
  virtual size_t
  size() const = 0;

  /** Get number of bits per value we use */
  virtual size_t
  getNumBits() const = 0;

  /** (AI) Calculate the max unsigned value with all bits set.
   * Up to a unsigned long long that is, which hopefully we never go
   * over or you should rethink your choice of data structure. */
  virtual unsigned long long
  calculateMaxValue() const = 0;

  /** Set all bits to 1 or max value */
  virtual void
  setAllBits() = 0;

  /** Clear all bits to 0 or min value */
  virtual void
  clearAllBits() = 0;

  // Humm could we implement operators here?

  /** Get a value back into a size_t.  Can overflow if not large enough for storage */
  virtual size_t
  get(size_t i) const = 0;

  /** Set a value back from a size_t.  Can overflow if not large enough for storage */
  virtual void
  set(size_t i, size_t v) = 0;

  /** A guess of deep size in bytes, useful for debugging and memory tracking */
  virtual size_t
  deepsize() const = 0;
};

/** Just wrap a std::vector.  This allows us to abstractly
 * pass other methods of storage around. */
template <typename T>
class StaticVectorT : public StaticVector {
public:

  /** Empty constructor */
  StaticVectorT(){ }

  /** Make vector of size */
  StaticVectorT(size_t size) : myData(size){ }

  /** Get size of the vector */
  virtual size_t
  size() const override
  {
    return myData.size();
  }

  /** Get number of bits per value we use */
  virtual size_t
  getNumBits() const override
  {
    return (sizeof(T) * 8);
  }

  /** (AI) Calculate the max unsigned value with all bits set.
   * Up to a unsigned long long that is, which hopefully we never go
   * over or you should rethink your choice of data structure. */
  virtual unsigned long long
  calculateMaxValue() const override
  {
    return static_cast<unsigned long long>(std::pow(2, getNumBits()) - 1);
  }

  /** Set all bits to 1 or max value */
  virtual void
  setAllBits() override
  {
    myData.assign(myData.size(), calculateMaxValue());
  }

  /** Clear all bits to 0 or min value */
  virtual void
  clearAllBits() override
  {
    myData.assign(myData.size(), 0);
  }

  /** Get a value back into a size_t.  Can overflow if not large enough for storage */
  size_t
  get(size_t i) const override
  {
    return static_cast<T>(myData[i]);
  }

  /** Set a value back from a size_t.  Can overflow if not large enough for storage */
  void
  set(size_t i, size_t v) override
  {
    myData[i] = static_cast<T>(v);
  }

  /** A guess of deep size in bytes, useful for debugging and memory tracking */
  virtual size_t
  deepsize() const override
  {
    size_t size = sizeof(*this);

    size += myData.size();

    return size;
  }

protected:

  /** The storage here is just a std::vector */
  std::vector<T> myData;
};

/** Bitset class that allows convenient storing of N values using B bits each.
 * This could be reimplemented using std::bitset depending on C++ version.
 * The advantage to this is reducing memory for large datasets that have to be
 * stored in RAM.
 *
 * @author Robert Toomey
 */
class Bitset : public StaticVector {
public:

  /** Empty constructor */
  Bitset(){ }

  /** Construct a bitset of using N values of B given size in bits each */
  Bitset(size_t numValues, size_t bitsPerValue) : StaticVector(numValues),
    myNumValues(numValues), myNumBits(bitsPerValue),
    myBits(numValues * bitsPerValue)
  { }

  /** Create a bitset of N dimensions */
  Bitset(std::vector<size_t> dimensions, size_t bitsPerValue) : StaticVector(dimensions),
    myNumValues(calculateSize(dimensions)), myNumBits(bitsPerValue),
    myBits(calculateSize(dimensions) * bitsPerValue)
  { }

  /** Get 'size' of items which is just getNumValues */
  size_t
  size() const override
  {
    return myNumValues;
  }

  /** Get number of bits per value we use */
  size_t
  getNumBits() const override
  {
    return myNumBits;
  }

  /** (AI) Calculate the max unsigned value with all bits set.
   * Up to a unsigned long long that is, which hopefully we never go
   * over or you should rethink your choice of data structure. */
  unsigned long long
  calculateMaxValue() const override
  {
    if (myNumBits >= std::numeric_limits<unsigned long long>::digits) {
      return std::numeric_limits<unsigned long long>::max();
    } else {
      return (1ULL << myNumBits) - 1;
    }
  }

  /** Set all bits to 1 or max value */
  void
  setAllBits() override
  {
    myBits.set();
  }

  /** Set all bits to 0 or min value */
  void
  clearAllBits() override
  {
    myBits.reset();
  }

  /** A guess of deep size in bytes, useful for debugging and memory tracking */
  size_t
  deepsize() const override
  {
    size_t size = sizeof(*this);

    size += myBits.capacity() / CHAR_BIT;

    return size;
  }

  /** Get a value back into a size_t.  Can overflow if not large enough for storage */
  size_t
  get(size_t i) const override
  {
    size_t y    = 0;
    size_t base = i * myNumBits;

    // Pull big endian first (human reading order)
    for (int j = myNumBits - 1; j >= 0; j--) {
      bool bit = myBits[base++];
      y |= bit << j;
    }
    return y;
  }

  /** Set a value back from a size_t.  Can overflow if not large enough for storage */
  void
  set(size_t i, size_t value) override
  {
    size_t base = i * myNumBits;

    // Push big endian first (human reading order)
    for (int j = myNumBits - 1; j >= 0; j--) {
      bool bit = (value >> j) & 1;
      myBits[base++] = bit;
    }
  }

  // Unique methods to bitset

  /** Dump bits at location to stream, useful for debugging.
   * Note that boost::dynamic_bitset is just bits, we're the ones breaking it down into N values.
   */
  void
  bits(std::ostream& os, size_t at) const
  {
    size_t count = 0;
    size_t i     = at * myNumBits;

    while (count++ < myNumBits) {
      os << myBits[i++];
    }
  }

  /** Allow operator << to access our internal fields */
  friend std::ostream&
  operator << (std::ostream&, const Bitset&);

protected:

  /** Number of values that we are storing */
  size_t myNumValues;

  /** Number of bits that we are storing */
  size_t myNumBits;

  /** Bit storage for values */
  boost::dynamic_bitset<> myBits;
};

/** (AI) Stream class to toggle full vs brief output of bitset.
 * This general method can be used for setting properties for stream out.
 * It is intended to be used in the simple form:
 * ostream << BitsetOutSetting(flags) << myBitset <<
 * Trying to use other ways may have unintended side effects.
 * We're not using a lock here since we lock our log stream per line for
 * output so that lock should keep things synced on a per line basis.
 * I wouldn't recommend splitting the BitsetOutSetting and bitset call
 * on multiple lines or Log calls since you'll lose thread syncing.
 */
class BitsetOutSettings : public Utility {
public:

  /** Create a BitsetOutSettings, intended only in a stream */
  BitsetOutSettings(bool full)
  {
    // Note another thread creating would race this
    // myMutex.lock();
    myOldValue = myFull;
    myFull     = full;
    // myMutex.unlock();
  }

  /** Destory by reverting back the flag to what we found it as */
  ~BitsetOutSettings()
  {
    // myMutex.lock();
    myFull = myOldValue;
    // myMutex.unlock();
  }

  /** Get flags, read by bitset operator << */
  static bool
  getFull()
  {
    // std::lock_guard<std::mutex> lock(myMutex);
    return myFull;
  }

  /** Allow operator << to access our internal fields */
  friend std::ostream&
  operator << (std::ostream&, const BitsetOutSettings&);

protected:
  /** Flag for full output of bitset or not */
  static bool myFull;

  /** Old value to restore state after destruction */
  bool myOldValue;
};

/** Stream print for a Bitset, uses BitsetOutSettings */
std::ostream&
operator << (std::ostream&,
  const Bitset&);

/** Stream print for a BitsetOutSettings */
std::ostream&
operator << (std::ostream&,
  const BitsetOutSettings&);
}
