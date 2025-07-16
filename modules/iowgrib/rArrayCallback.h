#pragma once

#include "rWgribCallback.h"
#include "rLLCoverageArea.h"
#include "rURL.h"

#include "rLatLonGrid.h" // temp

#include <iostream>

namespace rapio {
/**
 * Callback that processes grid data.
 *
 * Remember callbacks should directly use
 * std::cout not logging since we capture this
 * from wgrib2
 *
 * @author Robert Toomey
 */
class ArrayCallback : public WgribCallback {
public:

  /** Initialize a Catalog callback */
  ArrayCallback(const URL& u, const std::string& match);

  /** Initialize at the start of a grib2 catalog pass */
  void
  handleInitialize(int * decode, int * latlon) override;

  /** Finalize at the end of a grib2 catalog pass */
  void
  handleFinalize() override;

  /** Return action type for the c module */
  ActionType
  handleGetActionType() override { return ACTION_ARRAY; }
};

/** Handle creating a 2D array from multiple matches */
class Array2DCallback : public ArrayCallback {
public:
  /** Initialize a Catalog callback */
  Array2DCallback(const URL& u, const std::string& match) : ArrayCallback(u, match){ };

  /** Called with raw data, unprojected */
  virtual void
  handleSetDataArray(float * data, int nlats, int nlons, unsigned int * index) override;

  /** Pull the current 2D array, taking ownership */
  static std::shared_ptr<Array<float, 2> >
  pull2DArray()
  {
    auto temp = myTemp2DArray;

    myTemp2DArray = nullptr;
    return temp;
  }

protected:

  /** Temp storage of returned 2D Array */
  static std::shared_ptr<Array<float, 2> > myTemp2DArray;
};

/** Handle creating a 3D array from multiple matches */
class Array3DCallback : public ArrayCallback {
public:

  /** Initialize a Catalog callback */
  Array3DCallback(const URL& u, const std::string& match, std::vector<std::string> z) : ArrayCallback(u, match),
    myLayers(z){ };

  /** Called with raw data, unprojected */
  virtual void
  handleSetDataArray(float * data, int nlats, int nlons, unsigned int * index) override;
  /** Execute us for a given layer */
  virtual void
  executeLayer(size_t layer);

  /** Pull the current 3D array, taking ownership */
  static std::shared_ptr<Array<float, 3> >
  pull3DArray()
  {
    auto temp = myTemp3DArray;

    myTemp3DArray = nullptr;
    return temp;
  }

protected:

  /** The number of layers in the 3D block */
  std::vector<std::string> myLayers;

  /** Current layer number during execute */
  size_t myLayerNumber;

  /** Temp storage of returned 3D Array */
  static std::shared_ptr<Array<float, 3> > myTemp3DArray;
};
}
