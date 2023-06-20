#pragma once

#include "rIODataType.h"
#include "rGribMessageImp.h"

extern "C" {
#include <grib2.h>
}

#include <iomanip>

namespace rapio {
class GribAction;

/**
 * The base class of all Grib/2/3/etc formatters.
 *
 * @author Robert Toomey
 */
class IOGrib : public IODataType {
public:

  /** Help for grib module */
  virtual std::string
  getHelpString(const std::string& key) override;

  // Registering of classes ---------------------------------------------

  virtual void
  initialize() override;

  /** Get grib2 error into string. */
  static std::string
  getGrib2Error(g2int ierr);

  /** Scan a grib2 file applying a function to each message */
  static bool
  scanGribDataFILE(const URL& loc, GribAction * a);

  /**Scan a grib2 memory buffer applying a function to each message */
  static bool
  scanGribData(std::vector<char>& b, GribAction * a);

  // READING ------------------------------------------------------------
  //

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(const std::string& param) override;

  /** Do a buffer read of a 2D field */
  static std::shared_ptr<Array<float, 2> >
  get2DData(std::shared_ptr<GribMessageImp>& m, size_t fieldNumber);

  /** Do a buffer read of a 3D field using 2D and input vector of levels */
  static std::shared_ptr<Array<float, 3> >
  get3DData(std::vector<std::shared_ptr<GribMessageImp> >& mv, const std::vector<size_t>& fieldN,
    const std::vector<std::string>& levels);

  /** Do a full read from a param list */
  static std::shared_ptr<DataType>
  readGribDataType(const URL& path, int mode);

  // WRITING ------------------------------------------------------------

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    std::map<std::string, std::string>     & params
  ) override;

  virtual
  ~IOGrib();
};
}
