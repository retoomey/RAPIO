#pragma once

#include <rIO.h>
#include <rDataType.h>
#include <rConstants.h> // for SentinelDouble
#include <rIODataType.h>

#include <vector>
#include <string>
#include <exception>
#include <memory>
#include "rURL.h"

namespace rapio {
/** Worker classes that handle read and write of individual datatypes.
 * These can be overriden by users of the library. */
class NetcdfType : public IO {
public:

  /** Write DataType from given ncid (subclasses) */
  virtual bool
  write(int                            ncid,
    const DataType                     & dt,
    std::shared_ptr<DataFormatSetting> dfs) = 0;

  /** Read a DataType from given ncid */
  virtual std::shared_ptr<DataType>
  read(
    const int ncid,
    const URL & loc,
    const std::vector<std::string>&) = 0;
};

/**
 * The base class of all Netcdf formatters.
 *
 */

class IONetcdf : public IODataType {
public:

  static float MISSING_DATA;
  static float RANGE_FOLDED;

  // Registering of classes ---------------------------------------------

  /** Introduce self into factories */
  static void
  introduceSelf();

  /** Use this to introduce new subclasses. */
  static void
  introduce(const std::string   & dataType,
    std::shared_ptr<NetcdfType> new_subclass);

  /** Returns the appropriate IO given the type encoded in the
   *  netcdf file. IO objects read and write a particular DataType.
   *
   *  @return 0 if we don't know about this type.
   */
  static std::shared_ptr<NetcdfType>
  getIONetcdf(const std::string& type);

  // READING ------------------------------------------------------------
  //

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createObject(const std::vector<std::string>&) override;

  /** Do a full read from a param list */
  static std::shared_ptr<DataType>
  readNetcdfDataType(const std::vector<std::string>& args);

  // WRITING ------------------------------------------------------------

  // Virtual functions for DataWriter calls....

  /** Encode a DataType for writing */
  std::string
  encode(const rapio::DataType         & dt,
    const std::string                  & directory,
    std::shared_ptr<DataFormatSetting> dfs,
    std::vector<Record>                & records) override;

  /** Encode a DataType for writing */
  static std::string
  writeNetcdfDataType(const rapio::DataType& dt,
    const std::string                      & dir,
    std::shared_ptr<DataFormatSetting>     dfs,
    std::vector<Record>                    & records);

  virtual
  ~IONetcdf();

protected:

public:

  /** The global string attributes we expect to see in every file. */
  enum GlobalAttr { ncTypeName, ncDataType, ncConfigFile,
                    ncDataIdentifier,
                    ncNumGlobals };

private:
};
}
