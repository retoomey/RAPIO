#pragma once

#include <rDataType.h>
#include <rDataStore2D.h>

#include <vector>

namespace rapio {
/** DataGrid stores command access abilities for doing grided data.
 * This has common access/memory storage for multiband RadialSets
 * and LatLonGrids, etc.
 *
 * ALPHA: first pass adding multiband ability
 *
 * @author Robert Toomey */
class DataGrid : public DataType {
public:

  // For the moment, we support int and float only

  /** Get back object so can call methods on it */
  DataStore<float> *
  getFloat1D(const std::string& name)
  {
    // Hunt names for float vector
    size_t count = 0;

    for (auto i:myFloat1DNames) {
      if (i == name) {
        return (&(myFloat1DData[count]));
      }
      count++;
    }
    return nullptr;
  }

  /** Add named float data with initial size and value */
  void
  addFloat1D(const std::string& name, size_t size, float value)
  {
    // FIXME: check existance.
    myFloat1DData.push_back(DataStore<float>(size, value));
    myFloat1DNames.push_back(name);
  }

  /** Add named float data with initial size and value (uninitialized) */
  void
  addFloat1D(const std::string& name, size_t size)
  {
    myFloat1DData.push_back(DataStore<float>(size));
    myFloat1DNames.push_back(name);
  }

  /** Get a pointer data for quick transversing */
  DataStore<int> *
  getInt1D(const std::string& name)
  {
    // Hunt names for int vector
    size_t count = 0;

    for (auto i:myInt1DNames) {
      if (i == name) {
        // return (&(myInt1DData[count][0]));
        return (&myInt1DData[count]);
      }
      count++;
    }
    return nullptr;
  }

  /** Add named int data with initial size and value */
  void
  addInt1D(const std::string& name, size_t size, int value)
  {
    myInt1DData.push_back(DataStore<int>(size, value));
    myInt1DNames.push_back(name);
  }

  /** Add named int data with initial size and value (uninitialized) */
  void
  addInt1D(const std::string& name, size_t size)
  {
    myInt1DData.push_back(DataStore<int>(size));
    myInt1DNames.push_back(name);
  }

  /** Add named 2D float data with initial size and value */
  void
  addFloat2D(const std::string& name, size_t numx, size_t numy, float value)
  {
    // FIXME: check existance.
    myFloat2DData.push_back(DataStore2D<float>(numx, numy));
    myFloat2DNames.push_back(name);
  }

  /** Add named float data with initial size and value */
  void
  addFloat2D(const std::string& name, size_t numx, size_t numy)
  {
    // FIXME: check existance.
    myFloat2DData.push_back(DataStore2D<float>(numx, numy));
    myFloat2DNames.push_back(name);
  }

  /** Get back object so can call methods on it */
  DataStore<float> *
  getFloat2D(const std::string& name)
  {
    // Hunt names for float vector
    size_t count = 0;

    for (auto i:myFloat2DNames) {
      if (i == name) {
        return (&(myFloat2DData[count]));
      }
      count++;
    }
    return nullptr;
  }

protected:

  // Database of vectors.  Vectors work well with netcdf for
  // reading/writing quickly.  We 'could' use more metadata/wrapper class
  // and maybe group these better.  We intend this to be fast and hidden
  // so I'm ok with duplication here at moment.

  /** Store 1D vectors of float data with various sizes */
  std::vector<DataStore<float> > myFloat1DData;

  /** Store 1D vector names */
  std::vector<std::string> myFloat1DNames;

  /** Store 1D vectors of integer data with various sizes */
  std::vector<DataStore<int> > myInt1DData;

  /** Store 1D vector names */
  std::vector<std::string> myInt1DNames;

  /** Store 2D raster vector names */
  std::vector<std::string> myFloat2DNames;

  /** The raster bands of 2D raster data  */
  std::vector<DataStore2D<float> > myFloat2DData;
};
}
