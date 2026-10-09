#include "rIONetcdf.h"

#include "rFactory.h"
#include "rIOURL.h"
#include "rOS.h"
#include "rStrings.h"
#include "rDataFilter.h"
#include "rArith.h"
#include "config.h"

// Default built in DataType support
#include "rNetcdfDataGrid.h"
#include "rNetcdfDataType.h"
#include "rNetcdfRadialSet.h"
#include "rNetcdfLatLonGrid.h"
#include "rNetcdfLatLonHeightGrid.h"
#include "rNetcdfTimeHeightGrid.h"
#include "rNetcdfBinaryTable.h"

#include <netcdf_mem.h>

#include <cstdio>
#include <cassert>

using namespace rapio;
using namespace std;

// Library dynamic link to create this factory
extern "C"
{
void *
createRAPIOIO(void)
{
  auto * z = new IONetcdf();

  z->initialize();
  return reinterpret_cast<void *>(z);
}
};

float IONetcdf::MISSING_DATA = Constants::MissingData;
float IONetcdf::RANGE_FOLDED = Constants::RangeFolded;
int IONetcdf::GZ_LEVEL       = 6;

std::string
IONetcdf::getHelpString(const std::string& key)
{
  std::string help;

  help += "builder that uses the netcdf C library to read DataGrids or MRMS RadialSets, etc.";
  return help;
}

void
IONetcdf::initialize()
{
  // Add the default classes we handle...
  NetcdfRadialSet::introduceSelf(this);
  NetcdfLatLonGrid::introduceSelf(this);
  NetcdfLatLonHeightGrid::introduceSelf(this);
  NetcdfTimeHeightGrid::introduceSelf(this);
  NetcdfBinaryTable::introduceSelf(this);
  // Generic netcdf reader class
  NetcdfDataGrid::introduceSelf(this);
}

IONetcdf::~IONetcdf()
{ }

std::shared_ptr<DataType>
IONetcdf::createDataType(IOConfig& config)
{
  URL url(config.getParamURL());

  fLogInfo("Netcdf reader: {}", url.toString());
  std::shared_ptr<DataType> datatype = nullptr;

  // Note, in RAPIO we can read a netcdf file remotely too
  std::vector<char> buf;

  IOURL::read(url, buf);

  if (!buf.empty()) {
    // Open netcdf directly from buffer memory
    int retval, ncid;
    // nc_open_mem looks like it actually tries to read URLS directly
    // now...lol, this caused a massively confusing bug of double http server logs
    // and the 'second' url was misencoded as well.
    // const auto name = url.toString();
    // Trying to keep names fairly unique, not sure if it matters
    static size_t counter  = 1;
    const std::string name = "netcdf-" + std::to_string(OS::getProcessID()) + std::to_string(counter) + ".nc";
    if (counter++ > 1000000000) { counter = 1; }
    retval = nc_open_mem(name.c_str(), NC_NOWRITE, buf.size(), &buf[0], &ncid);

    if (retval == NC_NOERR) {
      // This is delegation, if successful we lose our netcdf-ness and become
      // a general data object.
      std::string type;
      retval = getAtt(ncid, Constants::sDataType, type);
      if (retval != NC_NOERR) {
        // Not necessarily an error, we could have a custom format
        fLogInfo("The NSSL 'DataType' netcdf attribute is not in netcdf file, trying generic reader");
        type = "DataGrid";
      }

      std::shared_ptr<IOSpecializer> fmt = IONetcdf::getIOSpecializer(type);
      if (fmt == nullptr) {
        // Not necessarily an error, we could have a custom format
        fLogInfo("No netcdf reader for DataType '{}', using generic reader", type);
        fmt = IONetcdf::getIOSpecializer("DataGrid");
      }
      std::shared_ptr<NetcdfSpecializer> netcdfFmt = // allowed on nullptr fmt
        std::dynamic_pointer_cast<NetcdfSpecializer>(fmt);

      if (netcdfFmt != nullptr) {
        datatype = netcdfFmt->readNETCDF(ncid, config);
        if (datatype) {
          datatype->postRead(config);
        }
      }
    } else {
      fLogSevere("Error reading netcdf: {}", nc_strerror(retval));
    }
    nc_close(ncid);
  } else {
    fLogSevere("Unable to pull data from {}", url.toString());
  }
  return (datatype);
} // IONetcdf::readNetcdfDataType

bool
IONetcdf::encodeDataType(std::shared_ptr<DataType> dt,
  IOConfig                                         & keys
)
{
  // ----------------------------------------------------------
  // Get specializer for the data type
  const std::string type = dt->getDataType();

  std::shared_ptr<IOSpecializer> fmt = IONetcdf::getIOSpecializer(type);

  // Default base class for now at least is DataGrid, so if doesn't cast
  // we have no methods to write this
  if (fmt == nullptr) {
    auto dataGrid = std::dynamic_pointer_cast<DataGrid>(dt);
    if (dataGrid != nullptr) {
      fmt = IONetcdf::getIOSpecializer("DataGrid");
    }
  }

  std::shared_ptr<NetcdfSpecializer> netcdfFmt = // allowed on nullptr fmt
    std::dynamic_pointer_cast<NetcdfSpecializer>(fmt);

  if (netcdfFmt == nullptr) {
    fLogSevere("Can't create a netcdf IO writer for datatype {}", type);
    return false;
  }

  // ----------------------------------------------------------
  // Get the filename we should write to
  std::string filename;

  if (!resolveFileName(keys, "netcdf", "netcdf-", filename)) {
    return false;
  }

  // ----------------------------------------------------------
  // Write Netcdf

  bool successful = false;

  // Get general netcdf settings
  int ncflags;

  try{
    ncflags = std::stoi(keys.get("ncflags"));
  }catch (const std::exception& e) {
    ncflags = NC_NETCDF4;
  }
  try{
    IONetcdf::GZ_LEVEL = std::stoi(keys.get("deflate_level"));
  }catch (const std::exception& e) {
    IONetcdf::GZ_LEVEL = 6;
  }

  // Open netcdf file
  int ncid = -1;

  // NC_memio finalmem;
  // size_t initialsize = 65000;
  try {
    // NETCDF(nc_create_mem("testing", NC_NETCDF4, initialsize, &ncid));
    NETCDF(nc_create(filename.c_str(), ncflags, &ncid));
  } catch (const NetcdfException& ex) {
    // nc_close_memio(ncid, &finalmem);
    nc_close(ncid);
    fLogSevere("Netcdf create error: {} {}", filename, ex.getNetcdfStr());
    return false;
  }

  if (ncid == -1) {
    fLogSevere("Invalid netcdf ncid, can't write");
    return false;
  }

  // Write netcdf to a disk file here
  try {
    // keys.set("NETCDF_NCID", to_string(ncid));
    dt->preWrite(keys);
    // successful = fmt->write(dt, keys);
    successful = netcdfFmt->writeNETCDF(ncid, dt, keys);
    dt->postWrite(keys);
  } catch (...) {
    successful = false;
    fLogSevere("Failed to write netcdf file for DataType");
  }

  nc_close(ncid);

  // ----------------------------------------------------------
  // Post processing such as extra compression, ldm, etc.
  if (successful) {
    successful = postWriteProcess(filename, keys);
  }

  // Standard output
  if (successful) {
    std::stringstream s;
    s << " (cmode:" << ncflags << " deflate_level: " << IONetcdf::GZ_LEVEL << ")";
    showFileInfo("Netcdf writer: ", keys, s.str());
  }

  return successful;
} // IONetcdf::encodeDataType

/** Add multiple dimension variable and assign a units to it */
int
IONetcdf::addVar(
  int          ncid,
  const char * name,
  const char * units,
  nc_type      xtype,
  int          ndims,
  const int    dimids[],
  int *        varid)
{
  int retval;

  // Define variable dimensions...
  retval = nc_def_var(ncid, name, xtype, ndims, dimids, varid);
  if (retval != NC_NOERR) {
    // Try again on netcdf4, etc only types...
    fLogSevere("Current netcdf format not handling {}, trying again", name);
    if (xtype == NC_UBYTE) { // netcdf4 and cdf5 only
      xtype  = NC_BYTE;
      retval = nc_def_var(ncid, name, xtype, ndims, dimids, varid);
    }
  }

  if (retval == NC_NOERR) {
    // Assign units string
    if (units != 0) {
      // retval = addAtt(ncid, Constants::Units, std::string(units), *varid);
      retval = addAtt(ncid, Constants::Units, "failure", *varid);
    }

    // Compress variable...
    if (retval == NC_NOERR) {
      //  retval = compressVar(ncid, *varid);
      const int shuffle = NC_CONTIGUOUS;
      const int deflate = 1;                        // if non-zero, turn on the
                                                    // deflate-level
      const int deflate_level = IONetcdf::GZ_LEVEL; // Set the compression level
      // Compression for variable
      //   NETCDF(nc_def_var_chunking(ncid, datavar, 0, 0)); Probably will never
      // need this
      // shuffle == control the HDF5 shuffle filter
      // default == turn on deflate for variable
      // deflate_level 0 no compression and 9 (max compression)
      // Don't care if fails..maybe warn or something...
      nc_def_var_deflate(ncid, *varid, shuffle, deflate, deflate_level);
    }
  }
  return (retval);
} // IONetcdf::addVar

int
IONetcdf::addVar1D(
  int          ncid,
  const char * name,
  const char * units,
  nc_type      xtype,
  int          dim,
  int *        varid)
{
  return (addVar(ncid, name, units, xtype, 1, &dim, varid));
}

int
IONetcdf::addVar2D(
  int          ncid,
  const char * name,
  const char * units,
  nc_type      xtype,
  int          dim1,
  int          dim2,
  int *        varid)
{
  int dims[2];

  dims[0] = dim1;
  dims[1] = dim2;

  return (addVar(ncid, name, units, xtype, 2, dims, varid));
}

int
IONetcdf::addVar3D(
  int          ncid,
  const char * name,
  const char * units,
  nc_type      xtype,
  int          dim1,
  int          dim2,
  int          dim3,
  int *        varid)
{
  int dims[3];

  dims[0] = dim1;
  dims[1] = dim2;
  dims[2] = dim3;

  return (addVar(ncid, name, units, xtype, 3, dims, varid));
}

int
IONetcdf::addVar4D(
  int          ncid,
  const char * name,
  const char * units,
  nc_type      xtype,
  int          dim1,
  int          dim2,
  int          dim3,
  int          dim4,
  int *        varid)
{
  int dims[4];

  dims[0] = dim1;
  dims[1] = dim2;
  dims[2] = dim3;
  dims[3] = dim4;

  return (addVar(ncid, name, units, xtype, 4, dims, varid));
}

int
IONetcdf::getAtt(int ncid,
  const std::string  & name,
  std::string        & text,
  const int          varid)
{
  int retval;
  size_t vr_len;
  const char * cname = name.c_str();

  retval = nc_inq_attlen(ncid, varid, cname, &vr_len);

  if (retval == NC_NOERR) {
    // Remember std::string isn't necessarily a vector of char internally by c98
    // up..
    // sooo make a char vector
    std::vector<char> c;
    c.resize(vr_len);

    retval = nc_get_att_text(ncid, varid, cname, &c[0]);

    if (retval == NC_NOERR) {
      text = std::string(c.begin(), c.end());
    }
  }
  return (retval);
}

// Helper for safely reading a single-value attribute.  Netcdf will happily
// write the full attribute length into the caller's buffer, so guard against
// array attributes before reading into a scalar.
template <typename T>
static int
getAttScalar(int ncid, const std::string& name, T * v, const int varid,
  int (* getter)(int, int, const char *, T *))
{
  size_t aLen = 0;
  int retval  = nc_inq_attlen(ncid, varid, name.c_str(), &aLen);

  if (retval != NC_NOERR) {
    return retval;
  }

  if (aLen != 1) {
    fLogSevere("Attribute '{}' has length {}, expected a single value.",
      name, aLen);
    return NC_EINVAL;
  }

  return getter(ncid, varid, name.c_str(), v);
}

// Helper for reading a one-element numeric attribute of any netcdf type,
// returning the value converted to a long.  Returns a netcdf return code.
template <typename T>
static int
getAttNumber(int ncid, int varid, const std::string& name, long& out,
  int (* getter)(int, int, const char *, T *))
{
  size_t len = 0;
  int retval = nc_inq_attlen(ncid, varid, name.c_str(), &len);

  if (retval != NC_NOERR) {
    return retval;
  }

  if (len != 1) {
    fLogSevere("Attribute '{}' has length {}, expected a single value.",
      name, len);
    return NC_EINVAL;
  }

  T value;

  retval = getter(ncid, varid, name.c_str(), &value);

  if (retval == NC_NOERR) {
    out = (long) value;
  }
  return retval;
}

int
IONetcdf::getAtt(int ncid,
  const std::string  & name,
  double *           v,
  const int          varid)
{
  return getAttScalar(ncid, name, v, varid, nc_get_att_double);
}

int
IONetcdf::getAtt(int ncid, const std::string& name, float * v,
  const int varid)
{
  return getAttScalar(ncid, name, v, varid, nc_get_att_float);
}

int
IONetcdf::getAtt(int ncid, const std::string& name, long * v,
  const int varid)
{
  return getAttScalar(ncid, name, v, varid, nc_get_att_long);
}

int
IONetcdf::getAtt(int   ncid,
  const std::string    & name,
  unsigned long long * v,
  const int            varid)
{
  size_t aLen   = 0;
  nc_type aType = NC_NAT;
  int retval    = nc_inq_att(ncid, varid, name.c_str(), &aType, &aLen);

  if (retval != NC_NOERR) {
    return retval;
  }

  if (aLen != 1) {
    fLogSevere("Attribute '{}' has length {}, expected a single value.",
      name, aLen);
    return NC_EINVAL;
  }

  // Query the attribute type first and cast from the common stored types,
  // instead of only trying unsigned long long and falling back blindly.
  switch (aType) {
      case NC_UINT64:
        return nc_get_att_ulonglong(ncid, varid, name.c_str(), v);

      case NC_INT64: {
        long long tmp;
        retval = nc_get_att_longlong(ncid, varid, name.c_str(), &tmp);
        if (retval == NC_NOERR) { *v = (unsigned long long) tmp; }
        return retval;
      }
      case NC_INT: { // also covers NC_LONG
        int tmp;
        retval = nc_get_att_int(ncid, varid, name.c_str(), &tmp);
        if (retval == NC_NOERR) { *v = (unsigned long long) tmp; }
        return retval;
      }
      case NC_UINT: {
        unsigned int tmp;
        retval = nc_get_att_uint(ncid, varid, name.c_str(), &tmp);
        if (retval == NC_NOERR) { *v = (unsigned long long) tmp; }
        return retval;
      }
      case NC_DOUBLE: {
        double tmp;
        retval = nc_get_att_double(ncid, varid, name.c_str(), &tmp);
        if (retval == NC_NOERR) { *v = (unsigned long long) tmp; }
        return retval;
      }
      case NC_FLOAT: {
        float tmp;
        retval = nc_get_att_float(ncid, varid, name.c_str(), &tmp);
        if (retval == NC_NOERR) { *v = (unsigned long long) tmp; }
        return retval;
      }
      default:
        fLogSevere(
          "Unhandled netcdf type {} for attribute '{}', can't read as unsigned long long.",
          aType, name);
        return NC_EBADTYPE;
  }
} // IONetcdf::getAtt

int
IONetcdf::addAtt(int ncid,
  const std::string  & name,
  const std::string  & text,
  const int          varid)
{
  const size_t aSize = text.size();
  int retval;

  retval = nc_put_att_text(ncid, varid, name.c_str(), aSize, text.c_str());
  return (retval);
}

int
IONetcdf::addAtt(int ncid,
  const std::string  & name,
  const double       value,
  const int          varid)
{
  int retval;

  retval = nc_put_att_double(ncid, varid, name.c_str(), NC_DOUBLE, 1, &value);
  return (retval);
}

int
IONetcdf::addAtt(int ncid,
  const std::string  & name,
  const float        value,
  const int          varid)
{
  int retval;

  retval = nc_put_att_float(ncid, varid, name.c_str(), NC_FLOAT, 1, &value);
  return (retval);
}

int
IONetcdf::addAtt(int ncid,
  const std::string  & name,
  const long         value,
  const int          varid)
{
  int retval;

  retval = nc_put_att_long(ncid, varid, name.c_str(), NC_LONG, 1, &value);
  return (retval);
}

int
IONetcdf::addAtt(int       ncid,
  const std::string        & name,
  const unsigned long long value,
  const int                varid)
{
  int retval;

  // Note: Pretty much everyone uses UINT64 for the u long long...
  retval = nc_put_att_ulonglong(ncid, varid, name.c_str(), NC_UINT64, 1, &value);
  return (retval);
}

int
IONetcdf::addAtt(int ncid,
  const std::string  & name,
  const int          value,
  const int          varid)
{
  int retval;

  retval = nc_put_att_int(ncid, varid, name.c_str(), NC_INT, 1, &value);
  return (retval);
}

int
IONetcdf::addAtt(int ncid,
  const std::string  & name,
  const short        value,
  const int          varid)
{
  int retval;

  retval = nc_put_att_short(ncid, varid, name.c_str(), NC_SHORT, 1, &value);
  return (retval);
}

bool
IONetcdf::dumpVars(int ncid)
{
  int globalcount = 0;
  nc_type vr_type;
  size_t vr_len;

  // Humm do we read all globals once and snag everything.
  // Read once and snag == O(n) in theory a little faster reading..
  // Read by variable.  More control, but in theory searching the var list
  // each time...
  NETCDF(nc_inq_natts(ncid, &globalcount)); // Hey can we do this for a
                                            // non-global var???

  std::cout << "----> There are " << globalcount << " global attributes\n";
  char name_in[NC_MAX_NAME + 1];

  for (int i = 0; i < globalcount; i++) {
    // retval = nc_inq_att(ncid, NC_GLOBAL, Constants::TypeName.c_str(),
    // &vr_type, &vr_len);
    NETCDF(nc_inq_attname(ncid, NC_GLOBAL, i, name_in));
    std::string name = std::string(name_in);
    NETCDF(nc_inq_att(ncid, NC_GLOBAL, name_in, &vr_type, &vr_len));
    std::string t      = "Unknown";
    std::string output = "";

    switch (vr_type) {
        case NC_BYTE: t = "byte";
          break;

        case NC_CHAR: {
          t = "char";

          // Remember std::string isn't necessarily a vector of char internally by
          // c98 up..
          // sooo make a char vector
          std::vector<char> c;
          c.resize(vr_len);
          NETCDF(nc_get_att_text(ncid, NC_GLOBAL, name_in, &c[0]));
          std::string out(c.begin(), c.end());
          output = out;
          break;
        }

        case NC_SHORT: t = "short";
          break;

        case NC_INT: t = "int";
          break;

        case NC_FLOAT: t = "float";
          break;

        case NC_DOUBLE: t = "double";
          break;

        default: {
          stringstream ss;
          ss << vr_type;
          t = ("Unknown " + ss.str());
        }
    }
    std::cout
      << i << ": '" << name << "' (" << t << ") == '" << output << "', Length = " << vr_len
      << "\n";
  }
  return (true);
} // IONetcdf::dumpVars

void
IONetcdf::readDimensionInfo(int ncid,
  const char *                  typeName,
  int *                         data_var,
  int *                         data_num_dims,
  const char *                  dim1Name,
  int *                         dim1,
  size_t *                      dim1size,
  const char *                  dim2Name,
  int *                         dim2,
  size_t *                      dim2size,
  const char *                  dim3Name,
  int *                         dim3,
  size_t *                      dim3size)
{
  // TypeName dimension for data array such as "Velocity"
  NETCDF(nc_inq_varid(ncid, typeName, data_var));
  NETCDF(nc_inq_varndims(ncid, *data_var, data_num_dims));

  // Standard dimensions
  NETCDF(nc_inq_dimid(ncid, dim1Name, dim1));
  NETCDF(nc_inq_dimlen(ncid, *dim1, dim1size));

  if (dim2size != 0) { // as pointer not value
    NETCDF(nc_inq_dimid(ncid, dim2Name, dim2));
    NETCDF(nc_inq_dimlen(ncid, *dim2, dim2size));
  }

  if (dim3size != 0) { // as pointer not value
    NETCDF(nc_inq_dimid(ncid, dim3Name, dim3));
    NETCDF(nc_inq_dimlen(ncid, *dim3, dim3size));
  }
}

bool
IONetcdf::dataArrayTypeToNetcdf(const DataArrayType& theType, nc_type& xtype)
{
  // Determine netcdf data output type from data grid type
  switch (theType) {
      case BYTE: xtype = NC_BYTE;
        break;
      case SHORT: xtype = NC_SHORT;
        break;
      case INT: xtype = NC_INT;
        break;
      case FLOAT: xtype = NC_FLOAT;
        break;
      case DOUBLE: xtype = NC_DOUBLE;
        break;
      default:
        fLogSevere("Trying to convert an unknown DataArrayType, using NC_Float..");
        return false;

        break;
  }
  return true;
}

bool
IONetcdf::netcdfToDataArrayType(const nc_type& xtype, DataArrayType& theType)
{
  // Determine data grid output type from netcdf type
  switch (xtype) {
      case NC_BYTE: theType = BYTE;
        break;
      case NC_SHORT: theType = SHORT;
        break;
      case NC_INT: theType = INT;
        break;
      case NC_FLOAT: theType = FLOAT;
        break;
      case NC_DOUBLE: theType = DOUBLE;
        break;
      default:
        fLogSevere("Trying to convert an unhandled netcdf to DataArrayType");
        return false;

        break;
  }
  return true;
}

// Maybe part of a NetcdfDataGrid class?
std::vector<int>
IONetcdf::declareGridVars(
  DataGrid& grid, const std::vector<int>& ncdims, int ncid)
{
  auto list = grid.getVisibleArrays();

  // The file's data type name is used to rename the primary array
  const std::string typeName = grid.getTypeName();

  // gotta be careful to write in same order as declare...
  std::vector<int> datavars;

  for (auto l:list) {
    auto theName = l->getName();

    // Ensure units even if missing in the DataGrid
    auto theUnitAttr = l->getAttribute<std::string>(Constants::Units);
    std::string theUnits;
    if (theUnitAttr) {
      theUnits = *theUnitAttr;
    } else {
      theUnits = "Dimensionless";
    }

    // Primary data is the data type of the file
    if (theName == Constants::PrimaryDataName) {
      theName = typeName;
    }

    // Determine netcdf data output type from data grid type
    nc_type xtype;
    if (!dataArrayTypeToNetcdf(l->getStorageType(), xtype)) {
      fLogSevere(
        "Declaring unknown/unsupported DataGrid variable type for {}, using NC_FLOAT, field may corrupt in output.",
        theName);
    }

    // Translate the indexes into the matching netcdf dimension
    auto ddims     = l->getDimIndexes();
    const size_t s = ddims.size();
    int dims[s];
    for (size_t i = 0; i < s; ++i) {
      dims[i] = ncdims[ddims[i]];
    }

    // Add the variable
    int var = -1;
    NETCDF(addVar(ncid, theName.c_str(), theUnits.c_str(), xtype, s, dims, &var));

    datavars.push_back(var);
  }
  return datavars;
} // IONetcdf::declareGridVars

IONetcdf::DimensionInfo
IONetcdf::getDimensions(int ncid)
{
  DimensionInfo info;

  int ndimsp = -1;

  // Find the number of dimension ids
  NETCDF(nc_inq_ndims(ncid, &ndimsp));
  size_t numdims = ndimsp < 0 ? 0 : (size_t) (ndimsp);

  // Find the dimension ids
  if (numdims > 0) {
    info.dimids.resize(numdims);
    NETCDF(nc_inq_dimids(ncid, 0, &info.dimids[0], 0));
  }

  // Detect unlimited (record) dimensions.  Reading works fine since
  // nc_inq_dim returns the current length, but callers should know a
  // record dimension is present and may grow.
  int nunlim = 0;
  int unlimids[NC_MAX_DIMS];

  NETCDF(nc_inq_unlimdims(ncid, &nunlim, unlimids));
  if (nunlim > 0) {
    fLogSevere("File has {} unlimited (record) dimension(s), RAPIO reading may be incomplete for those.",
      nunlim);
  }

  // For each dimension, get the name and size
  info.dimsizes.resize(numdims);
  info.dimnames.resize(numdims);
  char name[NC_MAX_NAME + 1];

  for (size_t d = 0; d < numdims; ++d) {
    NETCDF(nc_inq_dim(ncid, info.dimids[d], name, &info.dimsizes[d]));
    info.dimnames[d] = std::string(name);
  }
  return info;
} // IONetcdf::getDimensions

size_t
IONetcdf::getAttributes(int ncid, int varid, std::shared_ptr<DataAttributeList> list)
{
  char name_in[NC_MAX_NAME + 1];

  // Number of attributes...
  // If varid == NC_GLOBAL here, seems to work also
  // so maybe we could always use varnatts...
  int nattsp = -1;

  if (varid == NC_GLOBAL) {
    NETCDF(nc_inq_natts(ncid, &nattsp));
  } else {
    NETCDF(nc_inq_varnatts(ncid, varid, &nattsp));
  }

  // For each attribute...
  size_t nattsp2 = nattsp < 0 ? 0 : nattsp;

  for (size_t v = 0; v < nattsp2; ++v) {
    NETCDF(nc_inq_attname(ncid, varid, v, name_in));
    std::string attname    = std::string(name_in);
    std::string outattname = attname;

    // Make sure old Units in old data becomes the new units
    if (attname == "Units") {
      outattname = Constants::Units;
    }

    // Can do all in one right?  Well need name from above though
    // ...and get the type and length.
    size_t lenp;
    nc_type type_in;
    NETCDF(nc_inq_att(ncid, varid, name_in, &type_in, &lenp));

    /** Handle the netcdf types, remap to ours.
     * Scalar attributes are read by type and stored as the closest RAPIO
     * type (string, long, float or double).  Array attributes aren't
     * supported yet and are skipped. */
    if (list != nullptr) {
      switch (type_in) {
          case NC_BYTE: {
            long aLong;
            if (getAttNumber(ncid, varid, attname, aLong, nc_get_att_schar) == NC_NOERR) {
              list->put<long>(outattname, aLong);
            }
          }
          break;
          case NC_UBYTE: {
            long aLong;
            if (getAttNumber(ncid, varid, attname, aLong, nc_get_att_uchar) == NC_NOERR) {
              list->put<long>(outattname, aLong);
            }
          }
          break;
          case NC_CHAR: {
            std::string aString;
            getAtt(ncid, attname, aString, varid);
            list->put<std::string>(outattname, aString);
          }
          break;
          case NC_SHORT: {
            long aLong;
            if (getAttNumber(ncid, varid, attname, aLong, nc_get_att_short) == NC_NOERR) {
              list->put<long>(outattname, aLong);
            }
          }
          break;
          case NC_USHORT: {
            long aLong;
            if (getAttNumber(ncid, varid, attname, aLong, nc_get_att_ushort) == NC_NOERR) {
              list->put<long>(outattname, aLong);
            }
          }
          break;
          case NC_INT: { // includes the deprecated NC_LONG alias
            long aLong;
            if (getAtt(ncid, attname, &aLong, varid) == NC_NOERR) {
              list->put<long>(outattname, aLong);
            }
          }
          break;
          case NC_UINT: {
            long aLong;
            if (getAttNumber(ncid, varid, attname, aLong, nc_get_att_uint) == NC_NOERR) {
              list->put<long>(outattname, aLong);
            }
          }
          break;
          case NC_INT64: {
            long aLong;
            if (getAttNumber(ncid, varid, attname, aLong, nc_get_att_longlong) == NC_NOERR) {
              list->put<long>(outattname, aLong);
            }
          }
          break;
          case NC_FLOAT: {
            float aFloat;
            if (getAtt(ncid, attname, &aFloat, varid) == NC_NOERR) {
              list->put<float>(outattname, aFloat);
            }
          }
          break;
          case NC_DOUBLE: {
            double aDouble;
            if (getAtt(ncid, attname, &aDouble) == NC_NOERR) {
              list->put<double>(outattname, aDouble);
            }
          }
          break;
          case NC_STRING: {
            char * aString = nullptr;
            if (nc_get_att_string(ncid, varid, name_in, &aString) == NC_NOERR) {
              list->put<std::string>(outattname, std::string(aString ? aString : ""));
              nc_free_string(1, &aString);
            }
          }
          break;
          default:
            fLogSevere("Unhandled NETCDF type for {}, ignoring read of it.", name_in);
            break;
      }
    }
  }

  return 0;
} // IONetcdf::getAttributes

// Write a single attribute if the stored value matches the given type.
template <typename T>
static void
writeAttributeIf(NamedAny& at, int ncid, int varid)
{
  if (at.is<T>()) {
    auto field = at.get<T>();
    if (field) {
      NETCDF(IONetcdf::addAtt(ncid, at.getName(), *field, varid));
    }
  }
}

void
IONetcdf::setAttributes(int ncid, int varid, std::shared_ptr<DataAttributeList> list)
{
  // Meta info for our writer
  if (varid == NC_GLOBAL) {
    NETCDF(IONetcdf::addAtt(ncid, "MRMSWriterInfo", "RAPIO (Build: " + std::string(BUILD_DATE) + ")", varid));
  }

  // Netcdf has separate C functions per type, so dispatch on the stored type.
  for (auto& i: *list) {
    writeAttributeIf<std::string>(i, ncid, varid);
    writeAttributeIf<long>(i, ncid, varid);
    writeAttributeIf<float>(i, ncid, varid);
    writeAttributeIf<double>(i, ncid, varid);
  }
}
