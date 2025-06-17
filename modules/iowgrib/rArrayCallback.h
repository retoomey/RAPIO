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
 * from wgribw
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

  /** Add extra wgrib2 args if wanted */
  virtual void
  addExtraArgs(std::vector<std::string>& args) override;

  /** Finalize at the end of a grib2 catalog pass */
  void
  handleFinalize() override;

  /** Return action type for the c module */
  ActionType
  handleGetActionType() override { return ACTION_ARRAY; }

  /** Called with raw data, unprojected */
  virtual void
  handleSetDataArray(float * data, int nlats, int nlons, unsigned int * index) override;

  /** Temp storage of returned 2D Array */
  static std::shared_ptr<Array<float, 2> > myTemp2DArray;

protected:

  /** The match part of wgrib2 args */
  std::string myMatch;
};
}
