#pragma once

#include <rDataType.h>
#include <rDataStore2D.h>

#include <vector>

#include <boost/multi_array.hpp>

namespace rapio {
// Bunch of macros to allow plug and play here
// This uses boost over my DataStore.  It's a bit
// slower but the API is cleaner and expands to dimensions better
// I'm keeping DataStore for now
#define BOOST_ARRAY

#ifdef BOOST_ARRAY

# define RAPIO_1DF boost::multi_array<float, 1>
# define RAPIO_1DI boost::multi_array<int, 1>

# define RAPIO_2DF boost::multi_array<float, 2>

#else

# define RAPIO_1DF DataStore<float>
# define RAPIO_1DI DataStore<int>
# define RAPIO_2DF DataStore2D<float>

#endif // ifdef BOOST_ARRAY

/*
 * Class for storing names to data array objects
 * @author Robert Toomey
 */
template <typename T>
class DataNameLookup : public Data
{
  /** Store names for data */
  std::vector<std::string> myNames;

  /** Store data for names */
  std::vector<T> myNamedData;

public:

  /** Get data for this key */
  T *
  get(const std::string& name)
  {
    size_t count = 0;

    for (auto i:myNames) {
      if (i == name) {
        return (&(myNamedData[count]));
      }
      count++;
    }
    return nullptr;
  }

  /** Get data with a given key */
  void
  add(const std::string& name, T aT)
  {
    // FIXME: duplicate check/replace?
    myNamedData.push_back(aT);
    myNames.push_back(name);
  }
};

/** DataGrid acts as a name to storage database.
 * This has common access/memory storage for multiband RadialSets
 * and LatLonGrids, etc.
 *
 * ALPHA: first pass adding multiband ability
 *
 * @author Robert Toomey */
class DataGrid : public DataType {
public:

  // 1D stuff ----------------------------------------------------------

  /** Get back object so can call methods on it */
  RAPIO_1DF *
  getFloat1D(const std::string& name);

  /** Add named float data with initial size and value */
  void
  addFloat1D(const std::string& name, size_t size, float value);

  /** Add named float data with initial size and value (uninitialized) */
  void
  addFloat1D(const std::string& name, size_t size);

  /** Resize a float data */
  void
  resizeFloat1D(const std::string& name, size_t size, float value);

  /** Get a pointer data for quick transversing */
  RAPIO_1DI *
  getInt1D(const std::string& name);

  /** Add named int data with initial size and value */
  void
  addInt1D(const std::string& name, size_t size, int value);

  /** Add named int data with initial size and value (uninitialized) */
  void
  addInt1D(const std::string& name, size_t size);

  /** Resize a float data */
  void
  resizeInt1D(const std::string& name, size_t size, int value);

  // 2D stuff ----------------------------------------------------------

  /** Get back object so can call methods on it */
  RAPIO_2DF *
  getFloat2D(const std::string& name);

  /** Add named 2D float data with initial size and value */
  void
  addFloat2D(const std::string& name, size_t numx, size_t numy, float value);

  /** Add named float data with initial size and value */
  void
  addFloat2D(const std::string& name, size_t numx, size_t numy);

  /** Resize a 2D float data */
  void
  resizeFloat2D(const std::string& name, size_t rows, size_t cols, float value);

  // ----------------------------------------------------------------------
  // Access of the 'root' 2d data grid
  // FIXME: Only some things have this, not sure its place is here

  /** Sparse2D template wants this method (read) */
  void
  set(size_t i, size_t j, const float& v) override;

  /** Sparse2D template wants this to fill the default data 2d grid */
  void
  fill(const float& value) override;

  /** Get the number of Y for primary 2D data grid */
  size_t
  getY();

  /** Get the number of X for primary 2D data grid */
  size_t
  getX();

  /** Replace missing/range in the primary 2D data grid */
  void
  replaceMissing(const float missing, const float range);

  /** Resizes primary 2D grid to a ROW x COL grid and sets each cell to `value' */
  virtual void
  resize(size_t rows, size_t cols, const float fill = 0);
  // ----------------------------------------------------------------------

protected:

  /** Float 1D name to lookup */
  DataNameLookup<RAPIO_1DF> myFloat1D;

  /** Int 1D name to lookup */
  DataNameLookup<RAPIO_1DI> myInt1D;

  /** Float 2D name to lookup */
  DataNameLookup<RAPIO_2DF> myFloat2D;
};
}
