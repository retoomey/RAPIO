#pragma once

#include <rData.h>
#include <rUtility.h>

#include <boost/dynamic_bitset.hpp>

namespace rapio {
/** Bitset class that allows convenient storing of N values using B bits each.
 * This could be reimplemented using std::bitset depending on C++ version.
 * The advantage to this is reducing memory for large datasets that have to be
 * stored in RAM.
 *
 * @author Robert Toomey
 */
class Bitset : public Data {
public:

  /** Construct a bitset of using N values of B given size in bits each */
  Bitset(size_t numValues, size_t bitsPerValue) : myNumValues(numValues), myNumBits(bitsPerValue),
    myBits(numValues * bitsPerValue)
  { }

  /** (AI) Calculate the number of bits required to store a given unsigned number.  Usually
   * we can pass a max such as '255' to get 8 returned since 0-255 can be stored in 8 bits.
   * So if you want a Bitset to store X numbers from 0 to 255 in the smallest amount of space,
   * you could call Bitset(X, Bitset::smallestBitsToStore(255)), which would generate
   * a Bitset using 5 bits for each of the X numbers.
   */
  static unsigned int
  smallestBitsToStore(unsigned int x);

  /** Get number of unique values we're storing */
  size_t
  getNumValues() const
  {
    return myNumValues;
  }

  /** Get 'size' of items which is just getNumValues */
  size_t
  size() const
  {
    return myNumValues;
  }

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

  /** Get number of bits per value we use */
  size_t
  getNumBits() const
  {
    return myNumBits;
  }

  /** A guess of deep size in bytes, useful for debugging and memory tracking */
  size_t
  deepsize() const
  {
    size_t size = sizeof(*this);

    size += myBits.capacity() / CHAR_BIT;

    return size;
  }

  /** Get value at i */
  template <typename T>
  T
  get(size_t i) const
  {
    T y         = 0;
    size_t base = i * myNumBits;

    // Pull big endian first (human reading order)
    for (int j = myNumBits - 1; j >= 0; j--) {
      bool bit = myBits[base++];
      y |= bit << j;
    }
    return y;
  }

  /** Set value at i */
  template <typename T>
  void
  set(size_t i, T value)
  {
    size_t base = i * myNumBits;

    // Push big endian first (human reading order)
    for (int j = myNumBits - 1; j >= 0; j--) {
      bool bit = (value >> j) & 1;
      myBits[base++] = bit;
    }
  }

  /** (AI) Calculate the max unsigned value with all bits set.
   * Up to a unsigned long long that is, which hopefully we never go
   * over or you should rethink your choice of data structure. */
  unsigned long long
  calculateMaxValue()
  {
    if (myNumBits >= std::numeric_limits<unsigned long long>::digits) {
      return std::numeric_limits<unsigned long long>::max();
    } else {
      return (1ULL << myNumBits) - 1;
    }
  }

  /** Set all bits to 1 or max value */
  void
  setAllBits()
  {
    myBits.set();
  }

  /** Set all bits to 0 or min value */
  void
  clearAllBits()
  {
    myBits.reset();
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
