#pragma once

#include "rWgribCallback.h"

#include <iostream>

namespace rapio {
/**
 * Callback that dumps the catalog using wgrib2.
 * Technically since we just print out the list
 * we don't need a callback here.
 *
 * Remember callbacks should directly use
 * std::cout not logging since we capture this
 * from wgribw
 *
 * @author Robert Toomey
 */
class CatalogCallback : public WgribCallback {
public:

  /** Initialize a Catalog callback */
  CatalogCallback(const URL& u, const std::string& match, const std::string& dkey);

  /** Initialize at the start of a grib2 catalog pass */
  void
  handleInitialize(int * decode, int * latlon) override;

  /** Finalize at the end of a grib2 catalog pass */
  void
  handleFinalize() override;

  /** Called with raw data, unprojected */
  virtual void
  handleSetDataArray(float * data, int nlats, int nlons, unsigned int * index) override;

  /** Get the match count from the run */
  int getMatchCount(){ return myMatchCount; }

protected:

  /** Match counter */
  int myMatchCount;
};
}
