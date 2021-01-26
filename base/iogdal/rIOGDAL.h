#pragma once

#include "rIODataType.h"
#include "rDataGrid.h"
#include "rIO.h"

#include <iomanip>

namespace rapio {
/** Worker classes that handle read and write of GDAL types. */
class GDALType : public IO {
public:
  /** Write DataType from given ncid (subclasses) */
  virtual bool
  write(int                   ncid,
    std::shared_ptr<DataType> dt,
    std::shared_ptr<XMLNode>  dfs) = 0;

  virtual std::shared_ptr<DataType>
  read(
    const int ncid,
    const URL & loc
  ) = 0;
};

/**
 * The base class of all GDAL formatters.
 *
 * @author Robert Toomey
 */
class IOGDAL : public IODataType {
public:

  // Registering of classes ---------------------------------------------
  virtual void
  initialize() override;

  /** Use this to introduce new subclasses. */
  static void
  introduce(const std::string & dataType,
    std::shared_ptr<GDALType> new_subclass);

  static std::shared_ptr<GDALType>
  getIOGDAL(const std::string& type);

  // READING ------------------------------------------------------------
  //

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(const std::string& params) override;

  /** Do a full read from a param list */
  static std::shared_ptr<DataType>
  readGDALDataType(const URL& path);

  // WRITING ------------------------------------------------------------

  /** Encode using GDAL library */
  bool
  encodeGDAL();

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    const std::string                      & params,
    std::shared_ptr<XMLNode>               dfs,
    bool                                   directFile,
    // Output for notifiers
    std::vector<Record>                    & records
  ) override;

  virtual
  ~IOGDAL();
};
}
