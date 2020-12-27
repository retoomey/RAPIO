#include "rIONetcdf.h"

#include "rFactory.h"
#include "rIOURL.h"
#include "rOS.h"
#include "rStrings.h"
#include "rProcessTimer.h"

// Default built in DataType support
#include "rNetcdfDataGrid.h"
#include "rNetcdfDataType.h"
#include "rNetcdfRadialSet.h"
#include "rNetcdfLatLonGrid.h"
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

void
IONetcdf::initialize()
{
  // Add the default classes we handle...
  // Note: These could be replaced by algorithms with subclasses
  // for example to add or replace functionality
  // FIXME: This means some of the creation path/filename stuff
  // needs to be run through the NetcdfType class...
  NetcdfRadialSet::introduceSelf();
  NetcdfLatLonGrid::introduceSelf();
  NetcdfBinaryTable::introduceSelf();
  // Generic netcdf reader class
  NetcdfDataGrid::introduceSelf();
}

void
IONetcdf::introduce(const std::string & name,
  std::shared_ptr<NetcdfType>         new_subclass)
{
  Factory<NetcdfType>::introduce(name, new_subclass);
}

std::shared_ptr<NetcdfType>
IONetcdf::getIONetcdf(const std::string& name)
{
  return (Factory<NetcdfType>::get(name, "Netcdf builder"));
}

IONetcdf::~IONetcdf()
{ }

std::shared_ptr<DataType>
IONetcdf::readNetcdfDataType(const URL& url)
{
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
        LogSevere("The NSSL 'DataType' netcdf attribute is not in netcdf file, trying generic reader\n");
        type = "DataGrid";
      }

      std::shared_ptr<NetcdfType> fmt = IONetcdf::getIONetcdf(type);
      if (fmt == nullptr) {
        LogSevere("No netcdf reader for DataType '" << type << "', using generic reader\n");
        fmt = IONetcdf::getIONetcdf("DataGrid");
      }
      if (fmt != nullptr) {
        datatype = fmt->read(ncid, url);
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

#if 0
std::string
IONetcdf::writeNetcdfDataType(std::shared_ptr<DataType> dt,
  const std::string                                     & myDirectory,
  std::shared_ptr<XMLNode>                              dfs,
  std::vector<Record>                                   & records)
{
  // So myLookup "RadialSet" writer for example from the data type.
  //  This allows algs etc to replace our IONetcdf with a custom if needed.
  const std::string type = dt->getDataType();

  std::shared_ptr<NetcdfType> fmt = IONetcdf::getIONetcdf(type);
  if (fmt == nullptr) {
    fmt = IONetcdf::getIONetcdf("DataGrid");
  }

  if (fmt == nullptr) {
    LogSevere("Can't create IO writer for " << type << "\n");
    return ("");
  }

  // Generate the filepath/notification info for this datatype.
  // Static encoded for now.  Could make it virtual in the formatter
  std::vector<std::string> selections;
  std::vector<std::string> params;
  URL aURL = generateOutputInfo(*dt, myDirectory, dfs, "netcdf", params, selections);

  // Ensure full path to output file exists
  const std::string dir(aURL.getDirName());
  if (!OS::isDirectory(dir) && !OS::mkdirp(dir)) {
    LogSevere("Unable to create " << dir << "\n");
    return ("");
  }

  // Open netcdf file
  int ncid;

  // FIXME: should speed test this...
  try {
    NETCDF(nc_create(aURL.path.c_str(), NC_NETCDF4, &ncid));
  } catch (const NetcdfException& ex) {
    nc_close(ncid);
    LogSevere("Netcdf create error: "
      << aURL.path << " " << ex.getNetcdfStr() << "\n");
    return ("");
  }

  if (ncid == -1) { return (""); }

  bool successful = false;

  try {
    successful = fmt->write(ncid, dt, dfs);
  } catch (...) {
    successful = false;
    LogSevere("Failed to write netcdf file for DataType\n");
  }

  nc_close(ncid);

  /* If we upgrade netcdf can play with this which could allow
   * us to md5 checksum on the fly
   * NC_memio finalmem;
   * size_t initialsize = 65000;
   * try {
   *  // Interesting, might be able to pre size it to speed it up
   *  NETCDF(nc_create_mem("testing", NC_NETCDF4, initialsize, &ncid));
   * }catch(const NetcdfException& ex){
   *  nc_close_memio(ncid, &finalmem);
   *  LogSevere("Netcdf create error: " <<
   *            fileName_url.path << " " << ex.getNetcdfStr() << "\n");
   *  return "";
   * }
   * if (ncid == -1) return "";
   * bool successful = false;
   *
   * try {
   *  successful = fmt->write(ncid, dt);
   * } catch (...) {
   *  successful = false;
   *  LogSevere("Failed to write netcdf file for DataType\n");
   * }
   *
   * nc_close_memio(ncid, &finalmem);
   * std::ofstream ofp(fileName_url.path.c_str());
   * if (!ofp) {
   *  LogSevere("Unable to create " << tmpname << "\n");
   *  return;
   * }
   * ...write from mem to disk
   * ofp.close();
   */

  // -------------------------------------------------------------
  // Update records to match our written stuff...
  if (successful) {
    const rapio::Time rsTime = dt->getTime();
    Record rec(params, selections, rsTime);
    records.push_back(rec);
    return (aURL.path);
  }
  return ("");
} // IONetcdf::encodeStatic

#endif // if 0

bool
IONetcdf::encodeDataType(std::shared_ptr<DataType> dt,
  const URL                                        & aURL,
  std::shared_ptr<XMLNode>                         dfs)
{
  /** So myLookup "RadialSet" writer for example from the data type.
   * This allows algs etc to replace our IONetcdf with a custom if needed. */
  const std::string type = dt->getDataType();

  std::shared_ptr<NetcdfType> fmt = IONetcdf::getIONetcdf(type);

  // Default base class for now at least is DataGrid, so if doesn't cast
  // we have no methods to write this
  if (fmt == nullptr) {
    auto dataGrid = std::dynamic_pointer_cast<DataGrid>(dt);
    if (dataGrid != nullptr) {
      fmt = IONetcdf::getIONetcdf("DataGrid");
    }
  }

  if (fmt == nullptr) {
    LogSevere("Can't create IO writer for " << type << "\n");
    return false;
  }

  // Open netcdf file
  int ncid;

  // FIXME: should speed test this...
  try {
    NETCDF(nc_create(aURL.getPath().c_str(), NC_NETCDF4, &ncid));
  } catch (const NetcdfException& ex) {
    nc_close(ncid);
    LogSevere("Netcdf create error: "
      << aURL.getPath() << " " << ex.getNetcdfStr() << "\n");
    return false;
  }

  if (ncid == -1) { return false; }

  bool successful = false;

  try {
    successful = fmt->write(ncid, dt, dfs);
  } catch (...) {
    successful = false;
    LogSevere("Failed to write netcdf file for DataType\n");
  }

  nc_close(ncid);
  return successful;
} // IONetcdf::encodeDataType

/** Read call */
std::shared_ptr<DataType>
IONetcdf::createDataType(const URL& path)
{
  // virtual to static call..bleh
  return (IONetcdf::readNetcdfDataType(path));
}

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
}

/** Our default compression stuff for the c library.  */
int
IONetcdf::compressVar(int ncid, int datavar)
{
  int retval;
  const int shuffle = NC_CONTIGUOUS;

  // FIXME: gzcompress seems to be charle's .gz stuff..we never actually
  // used it to do netcdf compression.  roflmao...
  const int deflate = 1; // if non-zero, turn on the deflate-level
  // const int deflate_level = GZ_LEVEL;   // Set the compression level
  // Compression for variable
  //   NETCDF(nc_def_var_chunking(ncid, datavar, 0, 0)); Probably will never
  // need this
  // shuffle == control the HDF5 shuffle filter
  // default == turn on deflate for variable
  // deflate_level 0 no compression and 9 (max compression)
  const int deflate_level = 0; // GOOP

  retval = nc_def_var_deflate(ncid, datavar, shuffle, deflate, deflate_level);
  return (retval);
}

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
IONetcdf::readSparse2D(int ncid,
  int data_var,
  int num_x,
  int num_y,
  float fileMissing,
  float fileRangeFolded,
  Array<float, 2>           & array)
{
  // Background fill for value....
  // since sparse not all points touched..have to prefill.
  float backgroundValue;

  auto& data = array.ref();

  readSparseBackground(ncid, data_var, backgroundValue);
  // data.fill(backgroundValue); // fill should be in datatype or something
  // FIXME: maybe static in DataGrid?

  // Just view as 1D to fill it...
  // FIXME: FILL method in array
  array.fill(backgroundValue);

  //  boost::multi_array_ref<float, 1> a_ref(data.data(), boost::extents[data.num_elements()]);
  //  std::fill(a_ref.begin(), a_ref.end(), backgroundValue);

  // We should have a pixel dimension for sparse data...
  int pixel_dim;
  size_t num_pixels;
  NETCDF(nc_inq_dimid(ncid, "pixel", &pixel_dim));
  NETCDF(nc_inq_dimlen(ncid, pixel_dim, &num_pixels));

  // We 'can' do this, but it might be better not to bring it all into ram.
  // We have three ways: read the vector all at once, read 1 at a time, read n
  // at a time...
  // The old way read all at once..but since we have to step over azimuth,
  // etc...think it's
  // better not to do the double memory copy and step over it. (time vs memory)
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

    // Get short pixel_z(pixel) array (3d only)
    // int pixel_z_var;
    // NETCDF(nc_inq_varid(ncid, "pixel_z", &pixel_z_var));

    // Do we do Lak's max size check?  By now it really should be a valid netcdf
    // Check max size of full image. Originally, this was (1000*1000) but was
    // increased
    // to MAX_INT because some people were having issues.
    // x_max, y_max.  The max uncompressed 2d dimensions
    const short x_max(num_x); // azimuth for radialset
    const short y_max(num_y); // gates for radialset
    const int max_size = x_max * y_max;

    if (max_size > std::numeric_limits<int>::max()) {
      LogSevere(
        "Corrupt?: Max dimension is " << max_size
                                      << " which seems too large..\n");
      return (false);
    }

    // At most would expect num_pixel max_size, each with a runlength of 1
    if (num_pixels > size_t(max_size)) {
      LogSevere(
        "Corrupt?: Number of unique start pixels more than grid size, which seems strange\n");
      LogSevere(
        "Corrupt?: num_pixels is " << num_pixels << " while max_size is " << max_size
                                   << " \n");
      return (false);
    }

    if (num_pixels == 0) { return (true); }

    // Memory: Read it all in at once.
    // This way is much much faster than the var1 netcdf calls.  It eats up more
    // ram though
    // Saw a sparse radial set read at 0.003 vs .33 seconds by using single
    // netcdf var calls
    std::vector<float> data_val(num_pixels);
    NETCDF(nc_get_var_float(ncid, data_var, &data_val[0]));

    std::vector<short> pixel_x(num_pixels);
    NETCDF(nc_get_var_short(ncid, pixel_x_var, &pixel_x[0]));

    std::vector<short> pixel_y(num_pixels);
    NETCDF(nc_get_var_short(ncid, pixel_y_var, &pixel_y[0]));

    // std::vector<short> pixel_z;
    // NETCDF(nc_get_var_short(ncid, pixel_z_var, &pixel_z));
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
      // todo: NETCDF(nc_get_var1_short(ncid, pixel_z_var, &i, &z));
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
      LogSevere("Corrupt?: Skipped a total of " << pixelSkipped << " pixels\n");
    }

    if (pixelOverflow > 0) {
      LogSevere(
        "Corrupt?: Trimmed a total of " << pixelOverflow << " runlengths\n");
    }
  }
  return (true);
} // IONetcdf::readSparse2D

/*
 * bool IONetcdf::readSparse3D(int ncid, int data_var,
 *   int num_x, int num_y, int num_z, float fileMissing, float fileRangeFolded,
 *      T& data)
 * {
 * // Background fill for value....
 * // since sparse not all points touched..have to prefill.
 * float backgroundValue;
 * IONetcdf::readSparseBackground(ncid, data_var, backgroundValue);
 * data.fill(backgroundValue);  // fill should be in datatype or something
 *
 * // We should have a pixel dimension for sparse data...
 * int retval;
 * int pixel_dim;
 * size_t num_pixels;
 * NETCDF(nc_inq_dimid(ncid, "pixel", &pixel_dim));
 * NETCDF(nc_inq_dimlen(ncid, pixel_dim, &num_pixels));
 *
 * // We 'can' do this, but it might be better not to bring it all into ram.
 * // We have three ways: read the vector all at once, read 1 at a time, read n
 *    at a time...
 * // The old way read all at once..but since we have to step over azimuth,
 *    etc...think it's
 * // better not to do the double memory copy and step over it. (time vs memory)
 * if (num_pixels > 0)
 * {
 *
 *  // Get short pixel_x(pixel) array
 *  int pixel_x_var;
 *  NETCDF(nc_inq_varid(ncid, "pixel_x", &pixel_x_var));
 *
 *  // Get short pixel_y(pixel) array
 *  int pixel_y_var;
 *  NETCDF(nc_inq_varid(ncid, "pixel_y", &pixel_y_var));
 *
 *  // Get short pixel_z(pixel) array
 *  int pixel_z_var;
 *  NETCDF(nc_inq_varid(ncid, "pixel_z", &pixel_z_var));
 *
 *  // Get int pixel_count(pixel) array (optional)
 *  int pixel_count_var = -1;
 *  retval = nc_inq_varid(ncid, "pixel_count", &pixel_count_var);
 *
 *  // Get short pixel_z(pixel) array (3d only)
 *  //int pixel_z_var;
 *  //NETCDF(nc_inq_varid(ncid, "pixel_z", &pixel_z_var));
 *
 *  // Do we do Lak's max size check?  By now it really should be a valid netcdf
 *  // Check max size of full image. Originally, this was (1000*1000) but was
 *     increased
 *  // to MAX_INT because some people were having issues.
 *  // x_max, y_max.  The max uncompressed 2d dimensions
 *  const short x_max (num_x); // azimuth for radialset
 *  const short y_max (num_y); // gates for radialset
 *  const short z_max (num_z); // gates for radialset
 *  const int max_size = x_max*y_max*z_max;
 *  if (max_size > std::numeric_limits<int>::max()){
 *    LogSevere("Corrupt?: Max dimension is " << max_size << " which seems too
 *       large..\n");
 *    return false;
 *  }
 *
 *  // At most would expect num_pixel max_size, each with a runlength of 1
 *  if (num_pixels > size_t(max_size)){
 *    LogSevere(
 *     "Corrupt?: Number of unique start pixels more than grid size, which seems
 *        strange\n");
 *    LogSevere(
 *     "Corrupt?: num_pixels is " << num_pixels << " while max_size is " <<
 *        max_size << " \n");
 *    return false;
 *  }
 *
 *  // Memory: Read it all in at once.
 *  // This way is much much faster than the var1 netcdf calls.  It eats up more
 *     ram though
 *  // Saw a sparse radial set read at 0.003 vs .33 seconds by using single
 *     netcdf var calls
 *  std::vector<float> data_val (num_pixels);
 *  NETCDF(nc_get_var_float(ncid, data_var, &data_val[0]));
 *
 *  std::vector<short> pixel_x (num_pixels);
 *  NETCDF(nc_get_var_short(ncid, pixel_x_var, &pixel_x[0]));
 *
 *  std::vector<short> pixel_y (num_pixels);
 *  NETCDF(nc_get_var_short(ncid, pixel_y_var, &pixel_y[0]));
 *
 *  std::vector<short> pixel_z (num_pixels);
 *  NETCDF(nc_get_var_short(ncid, pixel_z_var, &pixel_z[0]));
 *
 *  std::vector<int> pixel_count (num_pixels, 1); // default to 1
 *  if (pixel_count_var > -1){
 *    nc_get_var_int(ncid, pixel_count_var, &pixel_count[0]);
 *  }
 *
 *  //short x, y, z;
 *  //int c;
 *  //float v;
 *  int pixelOverflow = 0;
 *  int pixelSkipped = 0;
 *  // For each runlength value...
 *  for(size_t i=0; i< num_pixels; ++i){
 *
 *    //NETCDF(nc_get_var1_short(ncid, pixel_x_var, &i, &x));
 *    //NETCDF(nc_get_var1_short(ncid, pixel_y_var, &i, &y));
 *    //NETCDF(nc_get_var1_short(ncid, pixel_z_var, &i, &z));
 *    short& x = pixel_x[i];
 *    short& y = pixel_y[i];
 *    short& z = pixel_z[i];
 *
 *    // Set the first pixel iff it's in the box
 *    // If it's NOT, then ALL the future pixels
 *    // in the runlength are pretty much out of bounds too
 *    if (0<=x && x<x_max && 0<=y && y<y_max && 0<=z && z<z_max){
 *      //NETCDF(nc_get_var1_float(ncid, data_var, &i, &v));
 *      float& v = data_val[i];
 *
 *      // Replace any missing/range using the file version to our internal.
 *      // This is so much faster here than looping with 'replace' functions
 *         later on.
 *      if (v == fileMissing){ v = Constants::MissingData; }
 *      else if (v == fileRangeFolded){ v = Constants::RangeFolded; }
 *
 *      data.set_val (z, x, y, v);
 *
 *      // Runlength of the current pixel length
 *      //if (pixel_count_var > -1){
 *      //  NETCDF(nc_get_var1_int(ncid, pixel_count_var, &i, &c));
 *      //}else{
 *      //  c = 1; // Missing pixel count, assume it's 1 per value
 *      //}
 *      int& c = pixel_count[i];
 *
 *      // A <1 pixel count is strange
 *      if (c <0){
 *        LogSevere("Corrupt?: Have a nonpositive pixel count of " << c <<
 *           "\n");
 *      }else{
 *
 *        // pixelCount can't be larger than the reamining pixels in the
 *        // grid..we could recover from this by triming pixelcount..so let's do
 *           that
 *        // x_max-x is num of full 'y' length pieces..
 * //          const int l = ((x_max-x)*y_max)-y; // -1 for original cell..
 * //          if (c > l){
 * //             pixelOverflow++;
 * //             c = l;
 * //          }
 *
 *        for (int j=1; j < c; ++j){
 *
 *          // 'Roll'columns and rows
 ++y;
 *          if ( y == y_max ){
 *             y = 0;
 ++x;
 *          }
 *          if ( x == x_max ){
 *            x = 0;
 ++z;
 *          }
 *          if (0<=x && x<x_max && 0<=y && y<y_max && 0<=z && z<z_max){
 *            data.set_val (z, x, y, v);
 *          }
 *        }
 *      }
 *    }else{
 *      pixelSkipped++;
 *    }
 *  } // numpixels
 *  if (pixelSkipped > 0){
 *    LogSevere("Corrupt?: Skipped a total of "<<pixelSkipped << " pixels\n");
 *  }
 *  if (pixelOverflow > 0){
 *    LogSevere("Corrupt?: Trimmed a total of "<<pixelOverflow << "
 *       runlengths\n");
 *  }
 * }
 * return true;
 * }
 */

// Maybe part of a NetcdfDataGrid class?
std::vector<int>
IONetcdf::declareGridVars(
  DataGrid& grid, const std::string& typeName, const std::vector<int>& ncdims, int ncid)
{
  auto list = grid.getArrays();

  // gotta be careful to write in same order as declare...
  std::vector<int> datavars;
  for (auto l:list) {
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
    if (theName == "primary") {
      theName = typeName;
    }

    // Determine netcdf data output type from data grid type
    auto theType  = l->getStorageType();
    nc_type xtype = NC_FLOAT;
    if (theType == FLOAT) {
      xtype = NC_FLOAT;
    } else if (theType == INT) {
      xtype = NC_INT;
    } else {
      LogSevere("Declaring unknown netcdf variable type for " << theName << "\n");
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
