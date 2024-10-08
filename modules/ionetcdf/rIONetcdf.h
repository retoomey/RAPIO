#pragma once

#include <rIO.h>
#include <rDataType.h>
#include <rConstants.h> // for SentinelDouble
#include <rIODataType.h>
#include <rDataGrid.h>

#include <vector>
#include <string>
#include <exception>
#include <memory>
#include "rURL.h"

#include "netcdf.h"

namespace rapio {
/** Our simple exception class for NETCDF fail wrapped.
 * This avoids the c++ NETCDF API that is incomplete and still is changing.
 * -- We gain the latest and greatest C API at will
 * -- We can optimize more if needed.
 *
 * @author Robert Toomey
 */
class NetcdfException : public IO, public std::exception {
public:

  NetcdfException(int err, const std::string& c) : std::exception(), retval(err),
    command(c)
  { }

  virtual ~NetcdfException() throw() { }

  int
  getNetcdfVal() const
  {
    return (retval);
  }

  std::string
  getNetcdfCommand() const
  {
    return (command);
  }

  std::string
  getNetcdfStr() const
  {
    return (std::string(nc_strerror(retval)) + " " + command);
  }

private:

  /** The netcdf return value from the command */
  int retval;

  /** The netcdf command we wrapped */
  std::string command;
};

/**
 * The base class of all Netcdf formatters.
 *
 */

class IONetcdf : public IODataType {
public:

  static float MISSING_DATA;
  static float RANGE_FOLDED;

  /** Help for netcdf module */
  virtual std::string
  getHelpString(const std::string& key) override;

  // Registering of classes ---------------------------------------------

  virtual void
  initialize() override;

  // READING ------------------------------------------------------------

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(const std::string& params) override;

  // WRITING ------------------------------------------------------------

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    std::map<std::string, std::string>     & params
  ) override;

  // --------------------------------------------------------
  // ADD utilities
  //

  /** General variable add.  Use a convenience function below if you can */
  static int
  addVar(int     ncid,
    const char * name,
    const char * units,
    nc_type      xtype,
    int          ndims,
    const int    dimids[],
    int *        varid);

  /** Add a single variable with given units */
  static int
  addVar1D(int   ncid,
    const char * name,
    const char * units,
    nc_type      xtype,
    int          dim,
    int *        varid);

  /** Add a 2D variable with given units */
  static int
  addVar2D(int   ncid,
    const char * name,
    const char * units,
    nc_type      xtype,
    int          dim1,
    int          dim2,
    int *        varid);

  /** Add a 3D variable with given units */
  static int
  addVar3D(int   ncid,
    const char * name,
    const char * units,
    nc_type      xtype,
    int          dim1,
    int          dim2,
    int          dim3,
    int *        varid);

  /** Add a 4D variable with given units */
  static int
  addVar4D(int   ncid,
    const char * name,
    const char * units,
    nc_type      xtype,
    int          dim1,
    int          dim2,
    int          dim3,
    int          dim4,
    int *        varid);

  /** Add a string attribute */
  static int
  addAtt(int         ncid,
    const std::string& name,
    const std::string& text,
    int              nvar = NC_GLOBAL);

  /** Add a double attribute */
  static int
  addAtt(int         ncid,
    const std::string& name,
    const double     value,
    int              nvar = NC_GLOBAL);

  /** Add a float attribute */
  static int
  addAtt(int         ncid,
    const std::string& name,
    const float      value,
    int              nvar = NC_GLOBAL);

  /** Add a long attribute */
  static int
  addAtt(int         ncid,
    const std::string& name,
    const long       value,
    int              nvar = NC_GLOBAL);

  /** Add a unsigned long long attribute */
  static int
  addAtt(int                 ncid,
    const std::string        & name,
    const unsigned long long value,
    int                      nvar = NC_GLOBAL);

  static int
  addAtt(int         ncid,
    const std::string& name,
    const int        value,
    int              nvar = NC_GLOBAL);

  static int
  addAtt(int         ncid,
    const std::string& name,
    const short      value,
    int              nvar = NC_GLOBAL);

  // --------------------------------------------------------
  // GET utilities
  //

  /** Get a string attribute */
  static int
  getAtt(int         ncid,
    const std::string& name,
    std::string      & text,
    const int        varid = NC_GLOBAL);

  /** Get a double attribute */
  static int
  getAtt(int         ncid,
    const std::string& name,
    double *         value,
    const int        varid = NC_GLOBAL);

  /** Get a float attribute */
  static int
  getAtt(int         ncid,
    const std::string& name,
    float *          value,
    const int        varid = NC_GLOBAL);

  /** Get a long attribute */
  static int
  getAtt(int         ncid,
    const std::string& name,
    long *           value,
    const int        varid = NC_GLOBAL);

  /** Get a unsigned long long attribute */
  static int
  getAtt(int             ncid,
    const std::string    & name,
    unsigned long long * value,
    const int            varid = NC_GLOBAL);

  /** Convenience for reading the initial typename and dimensional information
   * for 2 or 3 defined dimensions */
  static void
  readDimensionInfo(int ncid,
    const char *        typeName,
    int *               datavar,
    int *               data_num_dims,
    const char *        dim1Name,
    int *               dim1,
    size_t *            dim1size,
    const char *        dim2Name = "",
    int *               dim2     = 0,
    size_t *            dim2size = 0,
    const char *        dim3Name = "",
    int *               dim3     = 0,
    size_t *            dim3size = 0);

public:
  /** Debug functions */
  static bool
  dumpVars(int ncid);

  virtual
  ~IONetcdf();

  /** Convert from a stored DataGrid array type to netcdf data type.
   * This is the type to use to write the pointer. */
  static bool
  dataArrayTypeToNetcdf(const DataArrayType& theType, nc_type& out);

  /** Convert from a stored netcdf type to DataGrid array type.
   * This is the type to use to read the pointer. */
  static bool
  netcdfToDataArrayType(const nc_type& theType, DataArrayType& out);

  /** Convenience for declaring DataGrid variables */
  static
  std::vector<int>
  declareGridVars(DataGrid& grid, const std::string& typeName,
    const std::vector<int>& ncdims, int ncid);

  /** Convenience for gathering dimension information.
   * FIXME: Make object for dimension information? */
  static
  size_t
  getDimensions(int ncid, std::vector<int>& dimids,
    std::vector<std::string>& dimnames, std::vector<size_t>& dimsizes);

  /** Get a data attribute list from a netcdf attribute list */
  static
  size_t
  getAttributes(int ncid, int varid, std::shared_ptr<DataAttributeList> list);

  /** Set netcdf attributes from a data attribute list */
  static
  void
  setAttributes(int ncid, int varid, std::shared_ptr<DataAttributeList> list);

  /** The global string attributes we expect to see in every file. */
  enum GlobalAttr { ncTypeName, ncDataType, ncConfigFile,
                    ncDataIdentifier,
                    ncNumGlobals };

  static int GZ_LEVEL;
};
}

// Code readablity...
#define NETCDF(e) \
  { int ret2; if ((ret2 = e)) { throw NetcdfException(ret2, # e); } }
