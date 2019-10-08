#include "rIONetcdf.h"

#include "rFactory.h"
#include "rIOURL.h"
#include "rOS.h"
#include "rStrings.h"
#include "rProcessTimer.h"

// Default built in DataType support
#include "rNetcdfDataType.h"
#include "rNetcdfRadialSet.h"
#include "rNetcdfLatLonGrid.h"
#include "rNetcdfBinaryTable.h"

#include <netcdf_mem.h>

#include <cstdio>
#include <cassert>

using namespace rapio;
using namespace std;

float IONetcdf::MISSING_DATA = Constants::MissingData;
float IONetcdf::RANGE_FOLDED = Constants::RangeFolded;
int IONetcdf::GZ_LEVEL       = 6;

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
IONetcdf::readNetcdfDataType(const std::vector<std::string>& args)
{
  std::shared_ptr<DataType> datatype = nullptr;

  // Note, in RAPIO we can read a netcdf file remotely too
  const URL url = getFileName(args);
  std::vector<char> buf;
  IOURL::read(url, buf);

  if (!buf.empty()) {
    // Open netcdf directly from buffer memory
    int retval, ncid;
    const auto name = url.toString();
    retval = nc_open_mem(name.c_str(), 0, buf.size(), &buf[0], &ncid);

    if (retval == NC_NOERR) {
      // This is delegation, if successful we lose our netcdf-ness and become
      // a general data object.
      // FIXME: Maybe poll all the objects instead.  This could be slower
      // though.
      // Get the datatype from netcdf (How we determine subobject handling)
      std::string type;
      //  retval = IONetcdf::getAtt(ncid, "MAKEITBREAK", type);
      retval = getAtt(ncid, Constants::sDataType, type);
      if (retval == NC_NOERR) { // found
        std::shared_ptr<NetcdfType> fmt = IONetcdf::getIONetcdf(type);
        if (fmt != nullptr) {
          datatype = fmt->read(ncid, url, args);
        }
      } else {
        // FIXME: this might not actually be an error..
        LogSevere("The 'DataType' netcdf attribute is not in netcdf file\n");
        // Create generic netcdf object.  Up to algorithm to open/close it
        datatype = std::make_shared<NetcdfDataType>(buf);
        datatype->setTypeName("NetcdfData");
      }
    } else {
      LogSevere("Error reading netcdf: " << nc_strerror(retval) << "\n");
    }

    nc_close(ncid);
    return (datatype);
  } else {
    LogSevere("Unable to pull data from " << url << "\n");
  }
  return (datatype);
} // IONetcdf::readNetcdfDataType

std::string
IONetcdf::writeNetcdfDataType(const rapio::DataType& dt,
  const std::string                                & myDirectory,
  std::shared_ptr<DataFormatSetting>               dfs,
  std::vector<Record>                              & records)
{
  /** So myLookup "RadialSet" writer for example from the data type.
   * This allows algs etc to replace our IONetcdf with a custom if needed. */
  const std::string type = dt.getDataType();

  std::shared_ptr<NetcdfType> fmt = IONetcdf::getIONetcdf(type);

  if (fmt == nullptr) {
    LogSevere("Can't create IO writer for " << type << "\n");
    return ("");
  }

  // Generate the filepath/notification info for this datatype.
  // Static encoded for now.  Could make it virtual in the formatter
  std::vector<std::string> selections;
  std::vector<std::string> params;
  URL aURL = generateOutputInfo(dt, myDirectory, dfs, "netcdf", params, selections);

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
  } catch (NetcdfException& ex) {
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
   * }catch(NetcdfException& ex){
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
    const rapio::Time rsTime = dt.getTime();
    Record rec(params, selections, rsTime);
    records.push_back(rec);
    return (aURL.path);
  }
  return ("");
} // IONetcdf::encodeStatic

std::string
IONetcdf::encode(const rapio::DataType& dt,
  const std::string                   & directory,
  std::shared_ptr<DataFormatSetting>  dfs,
  std::vector<Record>                 & records)
{
  // FIXME: Do we need this to be static?
  return (IONetcdf::writeNetcdfDataType(dt, directory, dfs, records));
}

void
IONetcdf::introduceSelf()
{
  std::shared_ptr<IONetcdf> newOne = std::make_shared<IONetcdf>();

  // Read can read netcdf.  Can we read netcdf3?  humm
  // Factory<DataReader>::introduce("netcdf", newOne);
  Factory<IODataType>::introduce("netcdf", newOne);

  // Read can write netcdf/netcdf3
  // Factory<DataWriter>::introduce("netcdf", newOne);
  // Factory<DataWriter>::introduce("netcdf3", newOne);
  Factory<IODataType>::introduce("netcdf3", newOne);

  // Add the default classes we handle...
  // Note: These could be replaced by algorithms with subclasses
  // for example to add or replace functionality
  // FIXME: This means some of the creation path/filename stuff
  // needs to be run through the NetcdfType class...
  NetcdfRadialSet::introduceSelf();
  NetcdfLatLonGrid::introduceSelf();
  NetcdfBinaryTable::introduceSelf();
}

/** Read call */
std::shared_ptr<DataType>
IONetcdf::createObject(const std::vector<std::string>& args)
{
  // virtual to static call..bleh
  return (IONetcdf::readNetcdfDataType(args));
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
IONetcdf::addGlobalAttr(int ncid, const DataType& dt,
  const std::string& encoded_type)
{
  // Datatype and typename
  NETCDF(addAtt(ncid, Constants::TypeName, dt.getTypeName()));
  NETCDF(addAtt(ncid, Constants::sDataType, encoded_type));

  // Space-time-ref: Latitude, Longitude, Height, Time and FractionalTime
  Time aTime    = dt.getTime();
  LLH aLocation = dt.getLocation();
  NETCDF(addAtt(ncid, Constants::Latitude, aLocation.getLatitudeDeg()));
  NETCDF(addAtt(ncid, Constants::Longitude, aLocation.getLongitudeDeg()));
  NETCDF(addAtt(ncid, Constants::Height, aLocation.getHeightKM() * 1000.0));
  NETCDF(addAtt(ncid, Constants::Time, aTime.getSecondsSinceEpoch()));
  NETCDF(addAtt(ncid, Constants::FractionalTime, aTime.getFractional()));

  // We can go ahead and write the -unit -value stuff, however, for reading
  // they can't be read until the datatype is created.
  return (writeUnitValueList(ncid, dt));
}

bool
IONetcdf::getGlobalAttr(
  int ncid, std::vector<std::string>& all_attr,
  rapio::LLH * location,
  rapio::Time * time,
  rapio::SentinelDouble * FILE_MISSING_DATA,
  rapio::SentinelDouble * FILE_RANGE_FOLDED)
{
  // Datatype and typename
  std::string aDataType, aTypeName;
  NETCDF(getAtt(ncid, Constants::TypeName, aTypeName));
  NETCDF(getAtt(ncid, Constants::sDataType, aDataType));

  // FIXME: handle ConfigFile and DataIdentifier attributes.  Do we even use
  // these?
  // Maybe not even used here.  Probably Lak used it somewhere in the algs
  // Need to check but if not used wipe em and match the addGlobal above...
  std::string configFile, dataId;

  // if no config file was provided, use the data type as default.
  // getAtt(ncid, Constants::ConfigFile, configFile); // Ok to fail
  if (configFile.empty()) {
    configFile = aDataType; // getAtt(ncid,
  }

  // Constants::DataIdentifier,
  // dataId); // Ok to fail

  if (dataId.empty()) {
    dataId = "Filename"; // FIXME: netcdf file name
                         // really?.
  }

  // This seems really stupid.  Keeping the 'enum' list for moment...
  // Think a generic map might be better...
  all_attr.push_back(aTypeName);
  all_attr.push_back(aDataType);
  all_attr.push_back(configFile);
  all_attr.push_back(dataId);

  // Space-time-ref: Latitude, Longitude, Height, Time and FractionalTime
  float lat_degrees, lon_degrees, ht_meters;
  unsigned long long aTimeSecs;
  double aTimeFrac = 0;
  NETCDF(getAtt(ncid, Constants::Latitude, &lat_degrees));
  NETCDF(getAtt(ncid, Constants::Longitude, &lon_degrees));
  NETCDF(getAtt(ncid, Constants::Height, &ht_meters));
  NETCDF(getAtt(ncid, Constants::Time, &aTimeSecs));
  getAtt(ncid, Constants::FractionalTime, &aTimeFrac); // ok to fail here

  // Now create a space time ref
  location->set(lat_degrees, lon_degrees, ht_meters / 1000.0);

  time->set(aTimeSecs, aTimeFrac);

  // missing and rangefolded ...
  if (FILE_MISSING_DATA) {
    float missing;
    int retval = getAtt(ncid, "MissingData", &missing); // ok to fail here

    *FILE_MISSING_DATA =
      (retval == NC_NOERR) ? SentinelDouble((int) (missing), 0.0001) :
      Constants::MISSING_DATA;
  }

  if (FILE_RANGE_FOLDED) {
    float range;
    int retval = getAtt(ncid, "RangeFolded", &range); // ok to fail here
    *FILE_RANGE_FOLDED =
      (retval == NC_NOERR) ? SentinelDouble((int) (range), 0.0001) :
      Constants::RANGE_FOLDED;
  }

  return (true);
} // IONetcdf::getGlobalAttr

bool
IONetcdf::writeUnitValueList(int ncid, const rapio::DataType& dt)
{
  // all other attributes
  // const DataType::AttrMap attr = dt.getAttributes();
  const DataType::DataAttributes attr = dt.getAttributes2();

  std::string attrnames;

  for (auto& iter: attr) {
    attrnames = attrnames + " " + iter.first;
  }
  NETCDF(addAtt(ncid, "attributes", attrnames));

  for (auto& iter:attr) {
    string str, unit;

    if (dt.getRawDataAttribute(iter.first, str, unit)) {
      NETCDF(addAtt(ncid, iter.first + "-unit", unit));
      NETCDF(addAtt(ncid, iter.first + "-value", str));
    }
  }
  return (true);
}

bool
IONetcdf::readUnitValueList(int ncid, rapio::DataType& dt)
{
  int retval, retval2;

  // Read the general -unit -value attributes
  std::string strs;
  retval = getAtt(ncid, "attributes", strs);

  if (retval != NC_NOERR) {
    LogInfo(
      "No attributes string in datatype...skipping attribute -unit -value pairs\n");
    return (false);
  }
  std::vector<std::string> attrs;
  size_t total_attrs = rapio::Strings::split(strs, &attrs);
  std::string attr_value, attr_unit;

  for (size_t j = 0; j < total_attrs; j++) {
    const std::string value = attrs[j] + string("-value");
    const std::string unit  = attrs[j] + string("-unit");
    retval  = getAtt(ncid, value, attr_value);
    retval2 = getAtt(ncid, unit, attr_unit);

    if ((retval == NC_NOERR) && (retval2 == NC_NOERR)) {
      if ((attr_value.size() != 0) && (attr_unit.size() != 0)) {
        dt.setDataAttributeValue(attrs[j], attr_value, attr_unit);
      } else if ((attr_value.size() != 0) && (attr_unit.size() == 0)) {
        dt.setDataAttributeValue(attrs[j], attr_value);
      }
    }
  }
  return (true);
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

  LogSevere("----> There are " << globalcount << " global attributes\n");
  char name_in[NC_MAX_NAME];

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
    LogSevere(
      i << ": '" << name << "' (" << t << ") == '" << output << "', Length = " << vr_len
        << "\n");
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
  int                      data_var,
  int                      num_x,
  int                      num_y,
  float                    fileMissing,
  float                    fileRangeFolded,
  RAPIO_2DF                & data)
{
  // Background fill for value....
  // since sparse not all points touched..have to prefill.
  float backgroundValue;

  readSparseBackground(ncid, data_var, backgroundValue);
  // data.fill(backgroundValue); // fill should be in datatype or something
  // FIXME: maybe static in DataGrid?
  #ifdef BOOST_ARRAY
  // Just view as 1D to fill it...
  boost::multi_array_ref<float, 1> a_ref(data.data(), boost::extents[data.num_elements()]);
  std::fill(a_ref.begin(), a_ref.end(), backgroundValue);
  #else
  std::fill(data.begin(), data.end(), backgroundValue);
  #endif

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

        #ifdef BOOST_ARRAY
        data[x][y] = v;
        #else
        data.set(x, y, v);
        #endif

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

            #ifdef BOOST_ARRAY
            data[x][y] = v;
            #else
            data.set(x, y, v);
            #endif
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
    auto theName  = l->getName();
    auto theUnits = l->getUnits();
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
    auto ddims     = l->getDims();
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
