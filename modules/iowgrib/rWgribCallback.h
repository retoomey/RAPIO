#pragma once

#include "patch/f_rapio_callback.h"
#include "rUtility.h"
#include "rURL.h"

#include <memory>

namespace rapio {
/** Root virtual class visitor for wgrib2 match callbacks.
 *
 * This is very similar to the GribAction in the original
 * iowgrib, but I'm naming it differently for now.
 *
 * Note: These are called by wgrib c source, so using
 * Log routines will basically infinite loop. We
 * capture the output of wgrib2 when calling.
 * If you need to print debugging I would do something
 * like std::cout << "\nmymessage\n".  You'll basically
 * break into the current wgrib2 output
 */
class WgribCallback : public Utility {
public:

  /** Create a wgrib callback with a given URL file location */
  WgribCallback(const URL& gribfile) : myFilename(gribfile.toString())
  { }

  virtual
  ~WgribCallback() = default;

  /** Execute the callback, calling wgrib2 */
  virtual void
  execute() = 0;

  /** Initialize at the start of a grib2 catalog pass */
  virtual void
  handleInitialize(int * decode, int * latlon) = 0;

  /** Finalize at the end of a grib2 catalog pass */
  virtual void
  handleFinalize() = 0;

  /** Suggest a minimum LLCoverageArea based on grib2 extents */
  virtual void
  handleSetLatLon(double * lat, double * lon, size_t nx, size_t ny) = 0;

  /** Fill a LLCoverageArea wanted for grid interpolation, if we handle grids. */
  virtual void
  handleGetLLCoverageArea(double * nwLat, double * nwLon, double * seLat, double * seLon, double * dLat, double * dLon,
    int * nLat, int * nLon) = 0;

  /** Handle getting data from wgrib2 */
  virtual void
  handleData(const float * data, int n) = 0;

  /** Return a const char * to the stored URL, valid while we're in scope */
  const char *
  handleGetFileName() const
  {
    return myFilename.c_str();
  }

protected:

  /** Store the filename of the grib2 location */
  std::string myFilename;
};

/** C++ version of the C RapioCallback.
 * This acts as a bridge to a 'true' C++ class. Ok a struct is really just
 * a private class in C++, but...
 * We make this extra one in order a avoid having to create
 * the linker lambas in every subclass, avoid duplication.
 */
struct RAPIOCallbackCPP : public RapioCallback {
  // std::shared_ptr<WgribCallback> myCallback;
  WgribCallback * myCallback;

  // RAPIOCallbackCPP(std::shared_ptr<WgribCallback> m) : myCallback(m)
  RAPIOCallbackCPP(WgribCallback * m) : myCallback(m)
  {
    // Create a vtable for the C calls, linking to the true callback.
    initialize = [](RapioCallback * self, int * decode, int * latlon) {
        static_cast<RAPIOCallbackCPP *>(self)->myCallback->handleInitialize(decode, latlon);
      };

    finalize = [](RapioCallback * self) {
        static_cast<RAPIOCallbackCPP *>(self)->myCallback->handleFinalize();
      };

    setlatlon = [](RapioCallback * self, double * lats, double * lons, size_t nx, size_t ny) {
        static_cast<RAPIOCallbackCPP *>(self)->myCallback->handleSetLatLon(lats, lons, nx, ny);
      };

    getllcoverage = [](RapioCallback * self, double * nwLat, double * nwLon, double * seLat, double * seLon,
      double * dLat, double * dLon, int * nLat, int * nLon) {
        static_cast<RAPIOCallbackCPP *>(self)->myCallback->handleGetLLCoverageArea(nwLat, nwLon, seLat, seLon, dLat,
          dLon, nLat, nLon);
      };

    on_data = [](RapioCallback * self, const float * data, int n) {
        static_cast<RAPIOCallbackCPP *>(self)->myCallback->handleData(data, n);
      };

    getfilename = [](RapioCallback * self) -> const char * {
        return static_cast<RAPIOCallbackCPP *>(self)->myCallback->handleGetFileName();
      };
  }
};
}
