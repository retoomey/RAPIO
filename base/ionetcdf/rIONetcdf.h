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

/** Worker classes that handle read and write of individual datatypes.
 * These can be overriden by users of the library. */
class NetcdfType : public IO {
public:

  /** Write DataType from given ncid (subclasses) */
  virtual bool
  write(int                    ncid,
    std::shared_ptr<DataType>  dt,
    std::shared_ptr<PTreeNode> dfs) = 0;

  /** Read a DataType from given ncid */
  virtual std::shared_ptr<DataType>
  read(
    const int ncid,
    const URL & loc
  ) = 0;
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

  virtual void
  initialize() override;

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

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(const std::string& params) override;

  /** Do a full read from a param list */
  static std::shared_ptr<DataType>
  readNetcdfDataType(const URL& path);

  // WRITING ------------------------------------------------------------

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    const std::string                      & params,
    std::shared_ptr<PTreeNode>             dfs,
    bool                                   directFile,
    // Output for notifiers
    std::vector<Record>                    & records
  ) override;

  /** Current compression level of netcdf files */
  static void
  setGZLevel(int level)
  {
    GZ_LEVEL = level;
  }

  static int
  getGZLevel()
  {
    return (GZ_LEVEL);
  }

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

  /** Get background attribute for sparse data sets */
  static bool
  readSparseBackground(int ncid,
    int                    data_var,
    float                  & backgroundValue);

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
    int *               dim2 = 0,
    size_t *            dim2size = 0,
    const char *        dim3Name = "",
    int *               dim3 = 0,
    size_t *            dim3size = 0);

  /** Read sparse 2D.  Template for the 'set_val(x,y)' and 'fill' methods for
   * DataType classes */
  static bool
  readSparse2D(int ncid,
    int data_var,
    int num_x,
    int num_y,
    float fileMissing,
    float fileRangeFolded,
    Array<float, 2> & dt);

  // static bool readSparse3D(int   ncid,
  //                                           int   data_var,
  //                                           int   num_x,
  //                                           int   num_y,
  //                                           int   num_z,
  //                                           float fileMissing,
  //                                           float fileRangeFolded,
  //                                           DataType   & dt);

  /** Debug functions */
  static bool
  dumpVars(int ncid);

  virtual
  ~IONetcdf();

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

  /** Convenience for gathering global attributes.*/
  // static
  // size_t
  // getGlobalAttributes(int ncid );


protected:

public:

  /** The global string attributes we expect to see in every file. */
  enum GlobalAttr { ncTypeName, ncDataType, ncConfigFile,
                    ncDataIdentifier,
                    ncNumGlobals };

  static int GZ_LEVEL;
private:
};
}

// Code readablity...
#define NETCDF(e) \
  { int ret2; if ((ret2 = e)) { throw NetcdfException(ret2, # e); } }
