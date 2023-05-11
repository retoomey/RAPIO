#pragma once

#include <rData.h>
#include <rBitset.h>

#include <iostream>

namespace rapio {
/* SparseVector stores a growing/shrinking sparse vector of a given type.
 * Given a size, allocate a set base memory lookup index into
 * another vector of storage.  W2merger used this as a technique in MRMS
 * in order to save merger RAM in the CONUS.  For example, a full
 * CONUS xyz will take 18 GB of RAM just to store an empty std::vector per
 * xyz location.
 *
 * At 'minimum' storage, this takes N values of size keysize for the keys.
 * w2merger used a size_t which was about 1 GB of base memory. We can use
 * our Bitset class to store only the number of bits required for full
 * lookup size, which saves us RAM with the CONUS and drops us to about .5 GB
 * base size.  The storage at minimum would be an empty vector.
 *
 * At 'full' storage, this takes N values of size keysize that
 * each represent an index into the fully sized std::vector of storage.
 *
 * The storage can hold up to N-1 values since the max key represents empty.
 *
 * Lookup: M, 4, 3, 0, M, ...  key into the vector storage
 * Values: item0, item2, item3 (growing/shrinking) Up to N-1 values
 *
 * Note: Max items is also limited by maximum size_t which should be
 * enough in practice.
 *
 * @author Robert Toomey, (original ideas from Lak)
 *
 */
template <typename T>
class SparseVector : public Data {
public:

  /** Construct a sparse vector of a given maxSize.  Note it's really maxSize-1 since one key is reserved
   * for missing (the max). */
  SparseVector(size_t maxSize) : myLookup(maxSize, Bitset::smallestBitsToStore(maxSize))
  {
    myLookup.setAllBits(); // Make 'empty'
    myMissing = myLookup.calculateMaxValue();
  }

  /** Get a T* at lookup index i, if any.  This will return nullptr if missing.
   * Note if T is declared as a pointer, you get a handle back, so
   * then (*Value)->field will get the data out. */
  inline T *
  get(size_t i)
  {
    const size_t offset = myLookup.get<size_t>(i);

    if (offset == myMissing) {
      return nullptr;
    } else {
      return &myStorage[offset];
    }
  }

  /** Set field at index i.  This will replace any old value.  Note if T is a pointer then
   * it's up to caller to first get(i) and handle memory management. */
  inline T *
  set(size_t i, T data)
  {
    size_t offset = myLookup.get<size_t>(i);

    if (offset == myMissing) {
      offset = myStorage.size();
      myLookup.set<size_t>(i, offset);
      myStorage.push_back(data);
    } else {
      myStorage[offset] = data;
    }
    return &myStorage[offset];
  }

  /** (AI) Percentage full on the sparse storage.  This doesn't count any deep memory
   * stored in each node.  If this number is close to 100% you might be better off
   * as non-sparse. */
  float
  getPercentFull() const
  {
    const size_t num = myLookup.size();

    if (num == 0) {
      return 0.0;
    }
    return static_cast<float>(myStorage.size()) / num * 100.0;
  }

  /** A guess of deep size in bytes, useful for debugging and memory tracking */
  size_t
  deepsize() const
  {
    size_t size = sizeof(*this);

    size += myLookup.deepsize();
    // 'base' size of storage vector.  This will work for basic types but
    // doesn't deepsize the stored class.
    size += myStorage.capacity() * sizeof(T); // We want the actual RAM allocated
    return size;
  }

  /** Allow operator << to access our internal fields */
  template <typename U>
  friend std::ostream&
  operator << (std::ostream&, const SparseVector<U>&);

protected:

  /** Our lookup into our storage vector */
  Bitset myLookup;

  /** Our storage of given typename/class */
  std::vector<T> myStorage;

  /** Missing key in lookup, usually max unsigned of all set bits */
  unsigned long long myMissing;
};

/** Stream print for a SparseVector */
template <typename U>
std::ostream&
operator << (std::ostream& os,
  const SparseVector<U>  & sv)
{
  os << sv.myLookup << "\n";
  os << "Total size of storage: " << sv.myStorage.size() << "\n";
  os << "Total possible size of storage: " << sv.myLookup.size() << "\n";
  os << "Total percent filled: " << sv.getPercentFull() << "\n";
  return (os);
}

/** A subclass of SparseVector that allows referencing items by
 * a simple dimension vector.  SparseVector stores a single
 * dimension storage, this allows dimensional referencing.
 *
 * @author Robert Toomey, (original ideas from Lak)
 */
template <typename T>
class SparseVectorDims : public SparseVector<T> {
public:

  /** Create a sparse vector using a dimension vector.  For example, passing
   * {3,4,5} would create a sparse vector with max size 60 */
  SparseVectorDims(std::vector<size_t> dimensions) : SparseVector<T>(calculateSize(dimensions)),
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
  inline T *
  set(std::vector<size_t> v, T data)
  {
    return SparseVector<T>::set(getIndex(v), data);
  }

  /** Get a T* at dimensions v, if any.  This will return nullptr if missing.
   * Note if T is declared as a pointer, you get a handle back, so
   * then (*Value)->field will get the data out. */
  inline T *
  get(std::vector<size_t> v)
  {
    return SparseVector<T>::get(getIndex(v));
  }

protected:

  /** The number in each dimension */
  std::vector<size_t> myDimensions;

  /** Strides for each dimension.  Cache for calculating index */
  std::vector<size_t> myStrides;
};
}
