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

/** Create a bitset with dimensions
 * FIXME: should pull common code out of this and sparseVectorDims
 */
class BitsetDims : public Bitset {
public:

  /** Create a bitset using a dimension vector.  For example, passing
   * {3,4,5} would create a bitset with max size 60 */
  BitsetDims(std::vector<size_t> dimensions, size_t bitsPerValue) : Bitset(calculateSize(dimensions), bitsPerValue),
    myDimensions(dimensions), myStrides(std::vector<size_t>(dimensions.size(), 1))
  {
    // (AI) Cache strides for our dimensions, this reduces the multiplications/additions
    // when converting dimension coordinates into the linear index
    for (int i = myDimensions.size() - 2; i >= 0; --i) {
      myStrides[i] = myStrides[i + 1] * myDimensions[i + 1];
    }
  }

  /** Calculate total size required to store all dimensions given.
   * Used during creation to make the linear storage. */
  static size_t
  calculateSize(const std::vector<size_t>& values)
  {
    size_t total = 1;

    for (size_t value: values) {
      total *= value;
    }
    return total;
  }

  /** (AI) Calculate index in the dimension space, no checking */
  size_t
  getIndex(std::vector<size_t> indices)
  {
    size_t index = 0;

    for (int i = myDimensions.size() - 1; i >= 0; --i) { // match w2merger ordering
      index += indices[i] * myStrides[i];
    }
    return index;
  }

  /** Quick index for 3D when you know you have 3 dimensions.
   * This is basically a collapsed form of the general getIndex.
   */
  inline size_t
  getIndex3D(size_t x, size_t y, size_t z)
  {
    return ((z * myStrides[2]) + (y * myStrides[1]) + (x * myStrides[0]));
  }

  /** Keeping for moment, this is how w2merger calculates its
   * linear index.  We made these match for consistency:
   * getOldIndex({x,y,z}) == getIndex({x,y,z}) for all values.
   */
  size_t
  getOldIndex(std::vector<size_t> i)
  {
    size_t horsize = myDimensions[1] * myDimensions[2];
    size_t zsize   = myDimensions[2];

    return (i[0] * horsize + i[1] * zsize + i[2]);
  }

  /** Set field at dimensions v.  This will replace any old value.  Note if T is a pointer then
   * it's up to caller to first get(i) and handle memory management. */
  // inline T *
  // set(std::vector<size_t> v, T data)
  // {
  //   return SparseVector<T>::set(getIndex(v), data);
  // }

  /** Get a T* at dimensions v, if any.  This will return nullptr if missing.
   * Note if T is declared as a pointer, you get a handle back, so
   * then (*Value)->field will get the data out. */
  // inline T *
  // get(std::vector<size_t> v)
  // {
  //   return SparseVector<T>::get(getIndex(v));
  // }

protected:

  /** The number in each dimension */
  std::vector<size_t> myDimensions;

  /** Strides for each dimension.  Cache for calculating index */
  std::vector<size_t> myStrides;
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
