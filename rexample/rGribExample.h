#pragma once

/** RAPIO API */
#include <RAPIO.h>

using namespace rapio;

class GribExampleAlg : public RAPIOAlgorithm {
public:

  /** Create an example simple algorithm */
  GribExampleAlg(){ };

  // The basic API messages from the system

  /** Declare all algorithm options */
  virtual void
  declareOptions(RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(RAPIOOptions& o) override;

  /** Process a new record/datatype.  See the .cc for RAPIOData info */
  virtual void
  processNewData(RAPIOData& d) override;

  // Tests ------------------------------------

  /** Test get2DArray from grib2 data */
  void
  testGet2DArray(std::shared_ptr<GribDataType> grib2);

  /** Test get3DArray from grib2 data */
  void
  testGet3DArray(std::shared_ptr<GribDataType> grib2);

  /** Test getMessage/getField from grib2 data (for working with groups say N.1, N.2, etc.) */
  void
  testGetMessageAndField(std::shared_ptr<GribDataType> grib2);

  /** Test getLatLonGrid from grib2 */
  void
  testGetLatLonGrid(std::shared_ptr<GribDataType> grib2);

protected:
};
