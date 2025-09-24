#pragma once

#include "rIODataType.h"

namespace rapio {
/**
 * The base class of all Grib/2/3/etc formatters.
 *
 * @author Robert Toomey
 */
class IOWgrib : public IODataType {
public:

  /** Helper for capturing wgrib2_api output */
  static std::vector<std::string>
  capture_vstdout_of_wgrib2(bool useCapture, int argc, const char * argv[]);

  /** Help for grib module */
  virtual std::string
  getHelpString(const std::string& key) override;

  // Registering of classes ---------------------------------------------

  virtual void
  initialize() override;

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(const std::string& param) override;

  /** Do a full read from a param list */
  static std::shared_ptr<DataType>
  readGribDataType(const URL& path);

  // WRITING ------------------------------------------------------------

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    std::map<std::string, std::string>     & params
  ) override;

  virtual
  ~IOWgrib();
};
}
