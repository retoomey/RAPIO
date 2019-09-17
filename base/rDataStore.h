#pragma once

#include "rData.h"
#include <cstddef>
#include <stdlib.h>

#include "string.h"

// Memory checks for debugging/optimizing
// #define MEM_CHECK

#ifdef MEM_CHECK
# include "rError.h"
#endif

namespace rapio
{
/** A DataStore STL designed class that allocates data using C functions.
 * Why?
 * 0. Fun.  Here's an example of a C++11 and up custom memory class
 * 1. std::vector does allocation larger than the 'storage' (capacity)
 *    and we have some _massive_ data sets where this larger adds up quite a bit.
 * 2. std::vector preinitializes to a value which is a time waste for some our monsterous data sets.
 * 3. Typically we have static sized data and don't need to dynamically change on the fly.
 *    In otherwords, we size once and we're done.  std::array size needs to be declared at
 *    compile time.
 * 4. I'm actually wanting to add some built in memory tracking ability for ram data.
 * 5. inline allows us direct data access while maintaining API
 *    (compiler should optimize out even a double return inline)
 * 6. Not really any faster/slower than std::vector.  Convenience functions are nice though
 *
 * Implement the Four Horsemen. No, wait Six horsemen now.
 *  --- C++03 ---
 * Default Constructor(s)
 * Copy Constructor
 * Copy Assignment Operator
 * Destructor
 *  --- C++11 ---
 * Move Constructor
 * Move Assignment Operator
 *
 * Originally a drop in replacement for std::vector.  But we've added
 * a lot of convenience functions for various projections into data.
 *
 * @author Robert Toomey
 */
template <typename T> class DataStore : public Data
{
public:

  #ifdef MEM_CHECK
  void
  bytechange(bool plus, size_t v)
  {
    long oldbyte = Log::bytecounter;

    std::string pluss = " + ";
    if (plus) {
      Log::bytecounter += (sizeof(T) * v);
    } else {
      Log::bytecounter -= (sizeof(T) * v);
      pluss = " - ";
    }
    LogSevere("BYTECHANGE: " << &Log::bytecounter << ": " << oldbyte
                             << pluss << v << " items  == " << Log::bytecounter << "\n");
  }

  #endif // ifdef MEM_CHECK

  /** Default Constructor with optional predeclared size */
  DataStore<T>(size_t aSize = 0){
    if (aSize > 0) {
      d = (T *) malloc(sizeof(T) * aSize);
      s = aSize;
      #ifdef MEM_CHECK
      bytechange(true, s);
      LogSevere("D ALLOCATE " << (void *) (this)
                              << " (" << s << ") --> " << d
                              << " Memory: " << Log::bytecounter << "\n");
      #endif
    } else {
      d = nullptr;
      s = 0;
      #ifdef MEM_CHECK
      LogSevere("D ALLOCATE CHEAP " << (void *) (this)
                                    << " (" << s << ") --> " << d
                                    << " Memory: " << Log::bytecounter << "\n");
      #endif
    }
  }

  /** Constructor where default fill value is provided */
  DataStore<T>(size_t aSize, T v) : DataStore(aSize)
  {
    std::fill(begin(), end(), v);
  }

  /** Resize us to a raw ready for data state */
  void
  resize(size_t aSize)
  {
    // If d size is 0, this will malloc
    // If other size is 0, this will free
    #if defined(MEM_CHECK)
    size_t olds = s;
    #endif
    d = (T *) realloc(d, sizeof(T) * aSize);
    s = aSize;
    #if defined(MEM_CHECK)
    if (olds < aSize) {
      bytechange(true, aSize - olds);
    } else {
      bytechange(false, olds - aSize);
    }
    LogSevere("D RESIZE (" << s << ") --> " << d
                           << " MEMORY: " << Log::bytecounter << "\n");
    #endif
  }

  /** Resize us with a default value (slower) */
  void
  resize(size_t aSize, T fill)
  {
    resize(aSize);
    std::fill(begin(), end(), fill);
  }

  // Standard auto iteration.

  /** Simple begin points to the 0 element in memory block */
  T *
  begin()
  {
    return &d[0];
  }

  /** Same as begin, match with boost multi-array */
  T *
  data()
  {
    return &d[0];
  }

  /** Simple end points to the last element in memory block */
  T *
  end()
  {
    // return &d[s - 1];
    return &d[s]; // note it's one past end
  }

  /** C++03 Copy Constructor
   * C++ standard copy constructor will just value copy
   * each field, causing our d pointer to be double deleted
   * ...so we need to make sure our memory is unique */
  DataStore<T>( const DataStore<T> &o)
  {
    if (o.s > 0) {
      d = (T *) malloc(sizeof(T) * o.s);
      #ifdef MEM_CHECK
      bytechange(true, o.s);
      #endif
      memcpy(d, o.d, sizeof(T) * o.s);
      s = o.s;
    } else {
      d = 0;
      s = 0;
    }
    #ifdef MEM_CHECK
    LogSevere("D ALLOCATE COPY (" << s << ") --> " << d
                                  << " MEMORY: " << Log::bytecounter << "\n");
    #endif
  }

  /** C++03 COPY assignment operator */
  DataStore<T>&
  operator = (const DataStore<T>& o)
  {
    // DataStore<float> a(50000)
    // DataStore<float> b(10000)
    // a = b; a become 10000 and a copy of b's contents
    if (this != &o) {
      if (s != o.s) {
        resize(o.s);

        #ifdef MEM_CHECK
        LogSevere("D COPY ASSIGN (" << s << ") --> " << d << "\n");
        #endif

        // Make a copy into us
        if (s > 0) {
          memcpy(d, o.d, sizeof(T) * o.s);
        }
      }
    }
    return *this;
  }

  // C++11

  /** C++11 Move Constructor */
  DataStore<T>(DataStore<T> && other)
  {
    // Steal other's resources
    d = other.d;
    s = other.s;

    // Reset/clear the other
    other.d = nullptr;
    other.s = 0;
  }


  /** C++11 Move Assignment Operator */
  DataStore<T>&
  operator = (DataStore<T>&& o)
  {
    if (this != &o) {
      // Release my resources first...
      // Since we'll move the others into us
      if (d != nullptr) {
        free(d);
        #ifdef MEM_CHECK
        bytechange(false, s);
        #endif

        d = nullptr;
      }
      s = 0;

      // Steal others resources
      d = o.d;
      s = o.s;

      // Reset other's resources
      o.d = nullptr;
      o.s = 0;
    }
    return *this;
  }

  /** Destroy storage on delete */
  virtual ~DataStore()
  {
    if (d != nullptr) {
      #ifdef MEM_CHECK
      bytechange(false, s);
      #endif
      free(d);
      d = nullptr;
    }
    #ifdef MEM_CHECK
    LogSevere("D FREE " << (void *) (this) << " (" << s << ") --> " << d << " MEMORY: " << Log::bytecounter << "\n");
    #endif
  }

  /** Set data directly single dimension */
  inline void
  set(std::size_t i, T v){ d[i] = v; }

  /** Get data directly single dimension */
  inline T
  get(std::size_t i) const { return d[i]; }

  /** Convenience to use [] on us for our data array.  For higher dimensions
   * we stick with get/set functions. */
  inline T&
  operator [] (int i)
  {
    return d[i];
  }

  /** Fill with a given data value (note default is non-initialized, so you should
   *  avoid double filling data values for speed */
  void
  fill(T v)
  {
    std::fill(begin(), end(), v);
  }

  /** Size of the raw data store */
  inline size_t
  size() const { return s; }

protected:

  /** Pointer to data storage. */
  T * d;

  /** Size */
  size_t s;
};
}
