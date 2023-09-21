#pragma once

#include <rData.h>
#include <rBitset.h>

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
class SparseVector : public DimensionMapper {
public:

  /** Construct a sparse vector of a given maxSize.  Note it's really maxSize-1 since one key is reserved
   * for missing (the max). */
  SparseVector(size_t maxSize) : DimensionMapper(maxSize),
    myLookupPtr(StaticVector::Create(maxSize, true)), myLookup(*myLookupPtr)
  {
    myLookup.setAllBits(); // Make 'empty'
    myMissing = myLookup.calculateMaxValue();
  }

  /** Construct a sparse vector of dimensions. */
  SparseVector(std::vector<size_t> dimensions) : DimensionMapper(dimensions),
    myLookupPtr(StaticVector::Create(calculateSize(dimensions), true)), myLookup(*myLookupPtr)
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
    #if 0
    if (i > myLookup.size()) {
      LogSevere("OUT OF RANGE " << i << " > " << myLookup.size());
      return nullptr;
    }
    #endif
    // const size_t offset = myLookup.get(i);
    size_t offset = myLookup.get(i);

    if (offset == myMissing) {
      return nullptr;
    } else {
      #if 0
      if (offset > myStorage.size()) {
        LogSevere("Detected out of bounds " << offset << " and size is " << myStorage.size() << "\n");
        exit(1);
      }
      #endif
      return &myStorage[offset];
    }
  }

  /** Set field at index i.  This will replace any old value.  Note if T is a pointer then
   * it's up to caller to first get(i) and handle memory management. */
  inline T *
  set(size_t i, T data)
  {
    #if 0
    if (i > myLookup.size()) {
      LogSevere("SET OUT OF RANGE " << i << " > " << myLookup.size());
      return nullptr;
    }
    #endif
    size_t offset = myLookup.get(i);

    if (offset == myMissing) {
      offset = myStorage.size();
      myLookup.set(i, offset);
      myStorage.push_back(data);
    } else {
      myStorage[offset] = data;
    }
    #if 0
    if (offset > myStorage.size()) {
      LogSevere("Detected out of bounds " << offset << " and size is " << myStorage.size() << "\n");
      exit(1);
    }
    #endif
    return &myStorage[offset];
  }

  /** Set field at dimensions v.  This will replace any old value.  Note if T is a pointer then
   * it's up to caller to first get(i) and handle memory management. */
  inline T *
  setDims(std::vector<size_t> v, T data)
  {
    return set(getIndex(v), data);
  }

  /** Get a T* at dimensions v, if any.  This will return nullptr if missing.
   * Note if T is declared as a pointer, you get a handle back, so
   * then (*Value)->field will get the data out. */
  inline T *
  getDims(std::vector<size_t> v)
  {
    return get(getIndex(v));
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
  std::unique_ptr<StaticVector> myLookupPtr;

  /** Our lookup reference for speed */
  StaticVector& myLookup;

  /** Our storage of given typename/class */
  std::vector<T> myStorage;

  /** Missing key in lookup, usually max unsigned of all set bits */
  unsigned long long myMissing;
};

/** This just stores pointers in a regular vector but gives us the dimension stuff.
 * Lol.  Well ok maybe we just use a regular vector of pointers at this point, this
 * abstraction came from experimenting with different methods.
 */
template <typename T>
class SparseVector2 : public DimensionMapper  {
public:

  /** Construct a sparse vector of a given maxSize. */
  SparseVector2(size_t maxSize) : DimensionMapper(maxSize), myStorage(maxSize, nullptr)
  { }

  /** Construct a sparse vector of dimensions. */
  SparseVector2(std::vector<size_t> dimensions) : DimensionMapper(dimensions),
    myStorage(calculateSize(dimensions), nullptr)
  { }

  /** Get a T* at lookup index i, if any.  This will return nullptr if missing.
   * Note if T is declared as a pointer, you get a handle back, so
   * then (*Value)->field will get the data out. */
  inline T *
  get(size_t i)
  {
    #if 0
    if (i > myStorage.size()) {
      LogSevere("OUT OF RANGE " << i << " > " << myStorage.size());
      return nullptr;
    }
    #endif
    return myStorage[i];
  }

  /** Set field at index i.  This will replace any old value.  Note if T is a pointer then
   * it's up to caller to first get(i) and handle memory management. */
  inline T *
  set(size_t i, T * newdata)
  {
    #if 0
    if (i > myStorage.size()) {
      LogSevere("SET OUT OF RANGE " << i << " > " << myStorage.size());
      return nullptr;
    }
    #endif
    if (newdata != nullptr) {
      T * data = myStorage[i];
      if ((data != nullptr) && (data != newdata)) {
        delete data;
        //  mySize-- but then adding back new;
      } else {
        mySize++;
      }
      data = myStorage[i] = newdata;
      return data;
    } else {
      clear(i);
      return nullptr;
    }
  }

  /** Clear node */
  void
  clear(size_t i)
  {
    T * data = myStorage[i];

    if (data != nullptr) {
      myStorage[i] = nullptr;
      delete data;
      mySize--;
    }
  }

  /** Set field at dimensions v.  This will replace any old value.  Note if T is a pointer then
   * it's up to caller to first get(i) and handle memory management. */
  inline T *
  setDims(std::vector<size_t> v, T * data)
  {
    return set(getIndex(v), data);
  }

  /** Get a T* at dimensions v, if any.  This will return nullptr if missing.
   * Note if T is declared as a pointer, you get a handle back, so
   * then (*Value)->field will get the data out. */
  inline T *
  getDims(std::vector<size_t> v)
  {
    return get(getIndex(v));
  }

  /** (AI) Percentage full on the sparse storage.  This doesn't count any deep memory
   * stored in each node.  If this number is close to 100% you might be better off
   * as non-sparse. */
  float
  getPercentFull() const
  {
    return (1.0 * mySize) / (1.0 * myStorage.size()) * 100.0;
  }

  /** A guess of deep size in bytes, useful for debugging and memory tracking */
  size_t
  deepsize() const
  {
    size_t size = sizeof(*this);

    // Max size of storage, not including whatever T holds dynamically
    size_t full = myStorage.capacity() * sizeof(T);

    // Some of our cells store nullptr, some not
    float p = getPercentFull() / 100.0;

    size += std::floor(p * full);                   // items of T
    size += std::floor((1.0 - p) * sizeof(void *)); // nullptr

    return size;
  }

  /** Allow operator << to access our internal fields */
  template <typename U>
  friend std::ostream&
  operator << (std::ostream&, const SparseVector2<U>&);

protected:

  /** Our storage of given typename/class */
  std::vector<T *> myStorage;

  /** A size counter of non nullptr nodes */
  size_t mySize;
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

/** Stream print for a SparseVector2 */
template <typename U>
std::ostream&
operator << (std::ostream& os,
  const SparseVector2<U> & sv)
{
  os << "Total size of storage: " << sv.myStorage.size() << "\n";
  os << "Total percent filled: " << sv.getPercentFull() << "\n";
  return (os);
}
}
