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
  write(int       ncid,
    const DataType& dt) = 0;

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

// class IONetcdf : public DataWriter, public DataReader {
class IONetcdf : public IODataType {
public:

  static float MISSING_DATA;
  static float RANGE_FOLDED;
  static bool CDM_COMPLIANCE;
  static bool FAA_COMPLIANCE;

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

  /** Get filename from param list */
  static URL
  getFileName(const std::vector<std::string>& params,
    size_t                                  startOfFile);

  static URL
  getFileName(const std::vector<std::string>& params)
  {
    // First is 'netcdf', second is filename
    return (getFileName(params, 1));
  }

  /** Do a full read from a param list */
  static std::shared_ptr<DataType>
  readNetcdfDataType(const std::vector<std::string>& args);

  // WRITING ------------------------------------------------------------

  // Virtual functions for DataWriter calls....
  std::string
  encode(const rapio::DataType& dt,
    const std::string         & directory,
    bool                      useSubDirs,
    std::vector<Record>       & records) override;

  std::string
  encode(const rapio::DataType& dt,
    const std::string         & subtype,
    const std::string         & directory,
    bool                      useSubDirs,
    std::vector<Record>       & records) override;

  // End DataWriter

  static std::string
  encodeStatic(const rapio::DataType& dt,
    const std::string               & subtype,
    const std::string               & dir,
    bool                            usesubdirs,
    std::vector<Record>             & records);

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
