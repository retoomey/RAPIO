#include "rIONetcdf.h"

#include "rFactory.h"
#include "rIOURL.h"
#include "rOS.h"
#include "rStrings.h"
#include "rDataFilter.h"
#include "rArith.h"

// Default built in DataType support
#include "rNetcdfDataGrid.h"
#include "rNetcdfDataType.h"
#include "rNetcdfRadialSet.h"
#include "rNetcdfLatLonGrid.h"
#include "rNetcdfLatLonHeightGrid.h"
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
  NetcdfBinaryTable::introduceSelf(this);
  // Generic netcdf reader class
  NetcdfDataGrid::introduceSelf(this);
}

IONetcdf::~IONetcdf()
{ }

std::shared_ptr<DataType>
IONetcdf::createDataType(const std::string& params)
{
  URL url(params);

  LogInfo("Netcdf reader: " << url << "\n");
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
    retval = nc_open_mem(name.c_str(), 0, buf.size(), &buf[0], &ncid);

    if (retval == NC_NOERR) {
      // This is delegation, if successful we lose our netcdf-ness and become
      // a general data object.
      std::string type;
      retval = getAtt(ncid, Constants::sDataType, type);
      if (retval != NC_NOERR) {
        // Not necessarily an error, we could have a custom format
        LogInfo("The NSSL 'DataType' netcdf attribute is not in netcdf file, trying generic reader\n");
        type = "DataGrid";
      }

      std::shared_ptr<IOSpecializer> fmt = IONetcdf::getIOSpecializer(type);
      if (fmt == nullptr) {
        // Not necessarily an error, we could have a custom format
        LogInfo("No netcdf reader for DataType '" << type << "', using generic reader\n");
        fmt = IONetcdf::getIOSpecializer("DataGrid");
      }
      if (fmt != nullptr) {
        std::map<std::string, std::string> keys;
        keys["NETCDF_NCID"] = to_string(ncid);
        keys["NETCDF_URL"]  = url.toString();
        datatype = fmt->read(keys, nullptr);
      }
    } else {
      LogSevere("Error reading netcdf: " << nc_strerror(retval) << "\n");
    }
    nc_close(ncid);
  } else {
    LogSevere("Unable to pull data from " << url << "\n");
  }
  return (datatype);
} // IONetcdf::readNetcdfDataType

bool
IONetcdf::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>               & keys
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

  if (fmt == nullptr) {
    LogSevere("Can't create a netcdf IO writer for datatype " << type << "\n");
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
    ncflags = std::stoi(keys["ncflags"]);
  }catch (const std::exception& e) {
    ncflags = NC_NETCDF4;
  }
  try{
    IONetcdf::GZ_LEVEL = std::stoi(keys["deflate_level"]);
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
    LogSevere("Netcdf create error: "
      << filename << " " << ex.getNetcdfStr() << "\n");
    return false;
  }

  if (ncid == -1) {
    LogSevere("Invalid netcdf ncid, can't write\n");
    return false;
  }

  // Write netcdf to a disk file here
  try {
    keys["NETCDF_NCID"] = to_string(ncid);
    successful = fmt->write(dt, keys);
  } catch (...) {
    successful = false;
    LogSevere("Failed to write netcdf file for DataType\n");
  }

  nc_close(ncid);

  // ----------------------------------------------------------
  // Post processing such as extra compression, ldm, etc.
  if (successful) {
    successful = postWriteProcess(filename, keys);
  }

  LogInfo(
    "Netcdf writer: " << keys["filename"] << " (cmode:" << ncflags << " deflate_level: " << IONetcdf::GZ_LEVEL <<
      ")\n");

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
    LogSevere("Current netcdf format not handling " << name << ", trying again\n");
    if (xtype == NC_UBYTE) { // netcdf4 and cdf5 only
      xtype  = NC_BYTE;
      retval = nc_def_var(ncid, name, xtype, ndims, dimids, varid);
    }
  }

  if (retval == NC_NOERR) {
    // Assign units string
    if (units != 0) {
      retval = addAtt(ncid, Constants::Units, std::string(units), *varid);
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

int
IONetcdf::getAtt(int ncid,
  const std::string  & name,
  double *           v,
  const int          varid)
{
  // FIXME: Could crash if it's a array and not a single...check the SIZE
  int retval;

  retval = nc_get_att_double(ncid, varid, name.c_str(), v);
  return (retval);
}

int
IONetcdf::getAtt(int ncid, const std::string& name, float * v,
  const int varid)
{
  // FIXME: Could crash if it's a array and not a single...check the SIZE
  int retval;

  retval = nc_get_att_float(ncid, varid, name.c_str(), v);
  return (retval);
}

int
IONetcdf::getAtt(int ncid, const std::string& name, long * v,
  const int varid)
{
  // FIXME: Could crash if it's a array and not a single...check the SIZE
  int retval;

  retval = nc_get_att_long(ncid, varid, name.c_str(), v);
  return (retval);
}

int
IONetcdf::getAtt(int   ncid,
  const std::string    & name,
  unsigned long long * v,
  const int            varid)
{
  // FIXME: Could crash if it's a array and not a single...check the SIZE
  int retval;

  retval = nc_get_att_ulonglong(ncid, varid, name.c_str(), v);

  // Try to downsample to int if it's not there (Hack for time sec out which is
  // written out
  // as an int..which is actually a bug)
  // FIXME: Better way might be to query type in first place and cast for any
  // getAtt and maybe
  // warn.
  if (retval != NC_NOERR) {
    int down1;
    int retval2 = nc_get_att_int(ncid, varid, name.c_str(), &down1);

    if (retval2 == NC_NOERR) {
      *v     = (unsigned long long) (down1);
      retval = NC_NOERR;
    }
  }
  return (retval);
}

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

bool
IONetcdf::readSparseBackground(int ncid,
  int                              data_var,
  float                            & backgroundValue)
{
  // Background value for sparse data
  backgroundValue = Constants::MissingData;
  int retval = getAtt(ncid, "BackgroundValue", &backgroundValue, data_var);

  if (retval != NC_NOERR) {
    LogSevere(
      "Missing BackgroundValue attribute. Assuming Constants::MissingData.\n");
    return (false);
  } else {
    backgroundValue = Arith::roundOff(backgroundValue);
    return (true);
  }
}

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
IONetcdf::isMRMSSparse(
  std::vector<int>        & dimids,
  std::vector<std::string>& dimnames,
  std::vector<size_t>     & dimsizes)
{
  // Look for pixel dimension and assume this is a WDSS2 sparse netcdf
  bool sparse = false;
  auto s      = dimids.size();

  for (size_t i = 0; i < s; i++) {
    if (dimnames[i] == "pixel") {
      sparse = true;
      // Remove "pixel" dimension from result, since we'll expand it
      // into a new data array
      dimids.erase(dimids.begin() + i);
      dimnames.erase(dimnames.begin() + i);
      dimsizes.erase(dimsizes.begin() + i);
      break;
    }
  }
  return sparse;
}

bool
IONetcdf::isMRMSSparseField(const std::string& name)
{
  return ((name == "pixel_x") || (name == "pixel_y") || (name == "pixel_z") ||
         (name == "pixel_count"));
}

bool
IONetcdf::readSparse(int ncid,
  int                    varid,
  const std::string      & arrayName,
  const std::string      & units,
  std::vector<size_t>    & dimsizes,
  DataGrid               & dataGrid)
{
  // Look for short pixel_z(pixel)
  int pixel_z_var = -1;

  nc_inq_varid(ncid, "pixel_z", &pixel_z_var);
  const bool is3D = (pixel_z_var > -1);

  // FIXME: We can probably combine the 2D and 3D methods in a refactor,
  // but I know the 2D works right now and don't know the 3D does perfectly yet
  // so dispatch to two separate methods for now
  if (is3D) {
    auto data3DF = dataGrid.addFloat3D(arrayName, units, { 1, 2, 0 });
    if (!IONetcdf::readSparse3D(ncid, varid, dimsizes[1], dimsizes[2], dimsizes[0],
      Constants::MissingData, Constants::RangeFolded, *data3DF))
    {
      return false;
    }
  } else {
    auto data2DF = dataGrid.addFloat2D(arrayName, units, { 0, 1 });
    if (!IONetcdf::readSparse2D(ncid, varid, dimsizes[0], dimsizes[1],
      Constants::MissingData, Constants::RangeFolded, *data2DF))
    {
      return false;
    }
  }
  return true;
}

bool
IONetcdf::readSparse2D(int ncid,
  int                      data_var,
  int                      num_x,
  int                      num_y,
  float                    fileMissing,
  float                    fileRangeFolded,
  Array<float, 2>          & array)
{
  auto& data = array.ref();

  // Prefill background, since sparse not all points touched
  float backgroundValue;

  readSparseBackground(ncid, data_var, backgroundValue);
  array.fill(backgroundValue);

  // We should have a pixel dimension for sparse data...
  int pixel_dim;
  size_t num_pixels;

  NETCDF(nc_inq_dimid(ncid, "pixel", &pixel_dim));
  NETCDF(nc_inq_dimlen(ncid, pixel_dim, &num_pixels));

  // 2D
  if (num_pixels > 0) {
    // Get short pixel_x(pixel) array
    int pixel_x_var;
    NETCDF(nc_inq_varid(ncid, "pixel_x", &pixel_x_var));

    // Get short pixel_y(pixel) array
    int pixel_y_var;
    NETCDF(nc_inq_varid(ncid, "pixel_y", &pixel_y_var));

    // Get int pixel_count(pixel) array (optional)
    int pixel_count_var = -1;
    nc_inq_varid(ncid, "pixel_count", &pixel_count_var);

    // Max size check
    const short x_max(num_x); // azimuth for radialset
    const short y_max(num_y); // gates for radialset
    const int max_size = x_max * y_max;

    if (max_size > std::numeric_limits<int>::max()) {
      LogSevere("Corrupt?: Max dimension is " << max_size << " which seems too large..\n");
      return false;
    }

    // At most would expect num_pixel max_size, each with a runlength of 1
    if (num_pixels > size_t(max_size)) {
      LogSevere("Corrupt?: Number of unique start pixels more than grid size, which seems strange\n");
      LogSevere("Corrupt?: num_pixels is " << num_pixels << " while max_size is " << max_size << " \n");
      return false;
    }

    // Memory: Read it all in at once.
    // This way is much much faster than the var1 netcdf calls.  It eats up more ram though
    // Saw a sparse radial set read at 0.003 vs .33 seconds by using single netcdf var calls
    std::vector<float> data_val(num_pixels);
    NETCDF(nc_get_var_float(ncid, data_var, &data_val[0]));

    std::vector<short> pixel_x(num_pixels);
    NETCDF(nc_get_var_short(ncid, pixel_x_var, &pixel_x[0]));

    std::vector<short> pixel_y(num_pixels);
    NETCDF(nc_get_var_short(ncid, pixel_y_var, &pixel_y[0]));

    std::vector<int> pixel_count(num_pixels, 1); // default to 1
    if (pixel_count_var > -1) {
      nc_get_var_int(ncid, pixel_count_var, &pixel_count[0]);
    }

    int pixelOverflow = 0;
    int pixelSkipped  = 0;

    // For each runlength value...
    for (size_t i = 0; i < num_pixels; ++i) {
      // Non memory way:
      // NETCDF(nc_get_var1_short(ncid, pixel_x_var, &i, &x));
      // NETCDF(nc_get_var1_short(ncid, pixel_y_var, &i, &y));
      short& x = pixel_x[i];
      short& y = pixel_y[i];

      // Set the first pixel iff it's in the box
      // If it's NOT, then ALL the future pixels
      // in the runlength are pretty much out of bounds too
      if ((0 <= x) && (x < x_max) && (0 <= y) && (y < y_max)) {
        // NETCDF(nc_get_var1_float(ncid, data_var, &i, &v));
        float& v = data_val[i];

        // Replace any missing/range using the file version to our internal.
        // This is so much faster here than looping with 'replace' functions
        // later on.
        if (v == fileMissing) {
          v = Constants::MissingData;
        } else if (v == fileRangeFolded) {
          v = Constants::RangeFolded;
        }

        data[x][y] = v;

        // Non memory way: Runlength of the current pixel length
        // if (pixel_count_var > -1){
        //   NETCDF(nc_get_var1_int(ncid, pixel_count_var, &i, &c));
        // }else{
        //   c = 1; // Missing pixel count, assume it's 1 per value
        // }
        int& c = pixel_count[i];

        // A <1 pixel count is strange
        if (c < 0) {
          LogSevere("Corrupt?: Have a nonpositive pixel count of " << c << "\n");
        } else {
          // pixelCount can't be larger than the reamining pixels in the
          // grid..we could recover from this by triming pixelcount..so let's do
          // that

          // x_max-x is num of full 'y' length pieces..
          const int l = ((x_max - x) * y_max) - y; // -1 for original cell..

          if (c > l) {
            pixelOverflow++;
            c = l;
          }

          for (int j = 1; j < c; ++j) {
            // 'Roll'columns and rows
            ++y;

            if (y == y_max) {
              y = 0;
              ++x;
            }

            data[x][y] = v;
          }
        }
      } else {
        pixelSkipped++;
      }
    } // numpixels

    if (pixelSkipped > 0) {
      LogSevere("Corrupt sparse?: Skipped a total of " << pixelSkipped << " pixels\n");
    }

    if (pixelOverflow > 0) {
      LogSevere("Corrupt sparse?: Trimmed a total of " << pixelOverflow << " runlengths\n");
    }
  } else {
    // LogSevere("Corrupt sparse? Zero pixels.\n");
  }
  return true;
} // IONetcdf::readSparse2D

bool
IONetcdf::readSparse3D(int ncid, int data_var,
  int num_x, int num_y, int num_z, float fileMissing, float fileRangeFolded,
  Array<float, 3>& array)
{
  auto& data = array.ref();

  // Prefill background, since sparse not all points touched
  float backgroundValue;

  readSparseBackground(ncid, data_var, backgroundValue);
  array.fill(backgroundValue);

  // We should have a pixel dimension for sparse data...
  int pixel_dim;
  size_t num_pixels;

  NETCDF(nc_inq_dimid(ncid, "pixel", &pixel_dim));
  NETCDF(nc_inq_dimlen(ncid, pixel_dim, &num_pixels));

  // 3D
  if (num_pixels > 0) {
    // Get short pixel_x(pixel) array
    int pixel_x_var;
    NETCDF(nc_inq_varid(ncid, "pixel_x", &pixel_x_var));

    // Get short pixel_y(pixel) array
    int pixel_y_var;
    NETCDF(nc_inq_varid(ncid, "pixel_y", &pixel_y_var));

    // -------------------------------------------------------------
    // Get short pixel_z(pixel) array (DIFF)
    int pixel_z_var;
    NETCDF(nc_inq_varid(ncid, "pixel_z", &pixel_z_var));
    // -------------------------------------------------------------

    // Get int pixel_count(pixel) array (optional)
    int pixel_count_var = -1;
    nc_inq_varid(ncid, "pixel_count", &pixel_count_var);

    // Max size check
    const short x_max(num_x);
    const short y_max(num_y);
    const short z_max(num_z); // -------------------------------
    const int max_size = x_max * y_max * z_max;
    if (max_size > std::numeric_limits<int>::max()) {
      LogSevere("Corrupt?: Max dimension is " << max_size << " which seems too large..\n");
      return false;
    }

    // At most would expect num_pixel max_size, each with a runlength of 1
    if (num_pixels > size_t(max_size)) {
      LogSevere("Corrupt?: Number of unique start pixels more than grid size, which seems strange\n");
      LogSevere("Corrupt?: num_pixels is " << num_pixels << " while max_size is " << max_size << " \n");
      return false;
    }
    LogInfo("3D Dimensions: " << num_x << " * " << num_y << " * " << num_z << "\n");

    // Memory: Read it all in at once.
    // This way is much much faster than the var1 netcdf calls.  It eats up more ram though
    // Saw a sparse radial set read at 0.003 vs .33 seconds by using single netcdf var calls
    std::vector<float> data_val(num_pixels);
    NETCDF(nc_get_var_float(ncid, data_var, &data_val[0]));

    std::vector<short> pixel_x(num_pixels);
    NETCDF(nc_get_var_short(ncid, pixel_x_var, &pixel_x[0]));

    std::vector<short> pixel_y(num_pixels);
    NETCDF(nc_get_var_short(ncid, pixel_y_var, &pixel_y[0]));

    std::vector<short> pixel_z(num_pixels); // (DIFF)
    NETCDF(nc_get_var_short(ncid, pixel_z_var, &pixel_z[0]));

    std::vector<int> pixel_count(num_pixels, 1); // default to 1
    if (pixel_count_var > -1) {
      nc_get_var_int(ncid, pixel_count_var, &pixel_count[0]);
    }

    int pixelOverflow = 0;
    int pixelSkipped  = 0;

    // short x, y, z;
    // int c;
    // float v;
    // For each runlength value...
    size_t overx, overy, overz;
    for (size_t i = 0; i < num_pixels; ++i) {
      // NETCDF(nc_get_var1_short(ncid, pixel_x_var, &i, &x));
      // NETCDF(nc_get_var1_short(ncid, pixel_y_var, &i, &y));
      // NETCDF(nc_get_var1_short(ncid, pixel_z_var, &i, &z));
      short& x = pixel_x[i];
      short& y = pixel_y[i];
      short& z = pixel_z[i];

      // Set the first pixel iff it's in the box
      // If it's NOT, then ALL the future pixels
      // in the runlength are pretty much out of bounds too
      if ((0 <= x) && (x < x_max) && (0 <= y) && (y < y_max) && (0 <= z) && (z < z_max) ) {
        // NETCDF(nc_get_var1_float(ncid, data_var, &i, &v));
        float& v = data_val[i];

        // Replace any missing/range using the file version to our internal.
        // This is so much faster here than looping with 'replace' functions
        // later on.
        if (v == fileMissing) {
          v = Constants::MissingData;
        } else if (v == fileRangeFolded) {
          v = Constants::RangeFolded;
        }

        data[x][y][z] = v;

        // Runlength of the current pixel length
        // if (pixel_count_var > -1){
        //  NETCDF(nc_get_var1_int(ncid, pixel_count_var, &i, &c));
        // }else{
        //  c = 1; // Missing pixel count, assume it's 1 per value
        // }
        int& c = pixel_count[i];

        // A <1 pixel count is strange
        if (c < 0) {
          LogSevere("Corrupt sparse?: Have a nonpositive pixel count of " << c << "\n");
        } else {
          // pixelCount can't be larger than the reamining pixels in the
          // grid..we could recover from this by triming pixelcount..so let's do
          // that
          // x_max-x is num of full 'y' length pieces..
          //          const int l = ((x_max-x)*y_max)-y; // -1 for original cell..
          //          if (c > l){
          //             pixelOverflow++;
          //             c = l;
          //          }

          for (int j = 1; j < c; ++j) {
            // 'Roll'columns and rows
            ++y;
            if (y == y_max) {
              y = 0;
              ++x;
            }
            if (x == x_max) {
              x = 0;
              ++z;
            }
            if ((0 <= x) && (x < x_max) && (0 <= y) && (y < y_max) && (0 <= z) && (z < z_max) ) {
              data[x][y][z] = v;
            }
          }
        }
      } else {
        // Check integrity of the data.  We'll be adding a writer in a bit
        if (!((0 <= x) && (x < x_max))) { overx++; }
        if (!((0 <= y) && (y < y_max))) { overy++; }
        if (!((0 <= z) && (z < z_max))) { overz++; }
        pixelSkipped++;
      }
    }
    if (pixelSkipped > 0) {
      LogSevere("Corrupt sparse?: Skipped a total of " << pixelSkipped << " pixels\n");
      LogSevere("Overflow: " << overx << ", " << overy << ", " << overz << "\n");
    }

    if (pixelOverflow > 0) {
      LogSevere("Corrupt sparse?: Trimmed a total of " << pixelOverflow << " runlengths\n");
    }
  } else {
    // LogSevere("Corrupt sparse? Zero pixels.\n");
  }

  // Back verify the num_pixels required...
  size_t neededPixels = 0;
  float lastValue     = backgroundValue;

  for (size_t z = 0; z < num_z; z++) {
    for (size_t x = 0; x < num_x; x++) {
      for (size_t y = 0; y < num_y; y++) {
        float& v = data[x][y][z];
        if ((v != backgroundValue) && (v != lastValue) ) {
          ++neededPixels;
        }
        lastValue = v;
      }
    }
  }
  if (neededPixels != num_pixels) {
    LogSevere("Sparse CALCULATED " << neededPixels << " vs " << num_pixels << "\n");
  }

  return true;
} // IONetcdf::readSparse3D

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
        LogSevere("Trying to convert an unknown DataArrayType, using NC_Float..\n");
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
        LogSevere("Trying to convert an unhandled netcdf to DataArrayType\n");
        return false;

        break;
  }
  return true;
}

// Maybe part of a NetcdfDataGrid class?
std::vector<int>
IONetcdf::declareGridVars(
  DataGrid& grid, const std::string& typeName, const std::vector<int>& ncdims, int ncid)
{
  auto list = grid.getArrays();

  // gotta be careful to write in same order as declare...
  std::vector<int> datavars;

  for (auto l:list) {
    // Skip hidden (FIXME: maybe getVisibleArrays to hide all this)
    auto hidden = l->getAttribute<std::string>("RAPIO_HIDDEN");
    if (hidden) { continue; }

    auto theName = l->getName();
    // auto theUnits = l->getUnits();
    auto theUnitAttr = l->getAttribute<std::string>("Units");
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
      LogSevere(
        "Declaring unknown/unsupported DataGrid variable type for " << theName <<
          ", using NC_FLOAT, field may corrupt in output.\n");
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

size_t
IONetcdf::getDimensions(int ncid,
  std::vector<int>          & dimids,
  std::vector<std::string>  & dimnames,
  std::vector<size_t>       & dimsizes)
{
  int ndimsp = -1;

  // Find the number of dimension ids
  NETCDF(nc_inq_ndims(ncid, &ndimsp));
  size_t numdims = ndimsp < 0 ? 0 : (size_t) (ndimsp);

  // LogSevere("Number of dimensions: " << ndimsp << "\n");

  // Find the dimension ids
  // std::vector<int> dimids;
  dimids.resize(numdims);
  NETCDF(nc_inq_dimids(ncid, 0, &dimids[0], 0));

  // Find the number of unlimited dimensions.  FIXME: Can't handle this yet
  // FIXME: Hey if we use a class hint hint we can store more stuff right?
  // NETCDF(nc_inq_unlimdims(ncid, &nunlim, NULL));
  // std::vector<int> unlimids; unlimids.resize(numlim);
  // Netcdf(nc_inq_unlimdims(ncid, &numlim, &unlimids[0]));
  // if (nunlim > 0){ LogSevere("Can't handle unlimited netcdf data fields at moment...\n");}

  // For each dimension, get the name and size
  dimsizes.resize(numdims);
  dimnames.resize(numdims);
  char name[NC_MAX_NAME + 1];

  for (size_t d = 0; d < numdims; ++d) {
    // LogSevere("For dim " << d << " value is " << dimids[d] << "\n");
    // NETCDF(nc_inq_dim(ncid, dimids[d], name, &length));      // id from name
    NETCDF(nc_inq_dim(ncid, dimids[d], name, &dimsizes[d])); // id from name
    dimnames[d] = std::string(name);
    // LogSevere("--> '"<<dimnames[d]<<"' " << dimsizes[d] << "\n");
  }
  return numdims;
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
    if (attname == "units") { // Sparse used lowercase, should be capital
      outattname = "Units";
    }

    // Can do all in one right?  Well need name from above though
    // ...and get the type and length.
    size_t lenp;
    nc_type type_in;
    NETCDF(nc_inq_att(ncid, varid, name_in, &type_in, &lenp));

    /** Handle some/all the netcdf types, remap to ours.
     * FIXME: add more support */
    if (list != nullptr) {
      switch (type_in) {
          case NC_BYTE:
            LogSevere("Unhandled NETCDF type NC_BYTE for " << name_in << ", ignoring read of it.\n");
            break;
          case NC_UBYTE:
            LogSevere("Unhandled NETCDF type NC_UBYTE for " << name_in << ", ignoring read of it.\n");
            break;
          case NC_CHAR: {
            std::string aString;
            getAtt(ncid, attname, aString, varid);
            list->put<std::string>(outattname, aString);
          }
          break;
          case NC_SHORT:
            LogSevere("Unhandled NETCDF type NC_SHORT for " << name_in << ", ignoring read of it.\n");
            break;
          case NC_USHORT:
            LogSevere("Unhandled NETCDF type NC_USHORT for " << name_in << ", ignoring read of it.\n");
            break;
          case NC_LONG:
            long aLong;
            getAtt(ncid, attname, &aLong, varid);
            list->put<long>(outattname, aLong);
            break; // or NC_INT
          case NC_UINT:
            LogSevere("Unhandled NETCDF type NC_UINT for " << name_in << ", ignoring read of it.\n");
            break;
          case NC_FLOAT: {
            float aFloat;
            getAtt(ncid, attname, &aFloat, varid);
            list->put<float>(outattname, aFloat);
          }
          break;
          case NC_DOUBLE: {
            double aDouble;
            getAtt(ncid, attname, &aDouble);
            list->put<double>(outattname, aDouble);
          }
          break;
          case NC_STRING:
            LogSevere("Unhandled NETCDF type NC_STRING for " << name_in << ", ignoring read of it.\n");
            break;
          default:
            break;
      }
    }
  }

  return 0;
} // IONetcdf::getAttributes

void
IONetcdf::setAttributes(int ncid, int varid, std::shared_ptr<DataAttributeList> list)
{
  // For each type, write out attr...
  // Netcdf has c functions each type so we check types...
  // FIXME: Anyway to reduce the code in a smart way?
  for (auto& i: *list) {
    auto name = i.getName().c_str();
    // NC_CHAR
    if (i.is<std::string>()) {
      auto field = i.get<std::string>();
      NETCDF(IONetcdf::addAtt(ncid, name, *field, varid));
      // NC_LONG
    } else if (i.is<long>()) {
      auto field = i.get<long>();
      NETCDF(IONetcdf::addAtt(ncid, name, *field, varid));
      // NC_FLOAT
    } else if (i.is<float>()) {
      auto field = i.get<float>();
      NETCDF(IONetcdf::addAtt(ncid, name, *field, varid));
      // NC_DOUBLE
    } else if (i.is<double>()) {
      auto field = i.get<double>();
      NETCDF(IONetcdf::addAtt(ncid, name, *field, varid));
    }
  }
}
