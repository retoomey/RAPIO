#pragma once

/** RAPIO API (If your algorithm is EXTERNAL to the rapio repo,
 * you can use this header.  If part of the repo, use the individual
 * headers for compiling speed.  So for example, your alg is
 * checked into WDSSII or HYDRO you should use the RAPIO.h header
 * only in your code.  This will keep you independent of changes. */
// #include <RAPIO.h>
#include <rRAPIOAlgorithm.h>

using namespace rapio;

namespace rapio {
class GribDataType;

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
}
