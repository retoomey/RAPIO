#include "rIONetcdf.h"

#include "rNetcdfUtil.h"
#include "rFactory.h"
#include "rBuffer.h"
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
  Buffer buf;
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
      //  retval = NetcdfUtil::getAtt(ncid, "MAKEITBREAK", type);
      retval = NetcdfUtil::getAtt(ncid, Constants::sDataType, type);
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

/*
 * URL
 * generateOutputInfo(const DataType& dt,
 * const std::string                         & directory,
 * std::shared_ptr<DataFormatSetting> dfs,
 * const std::string                         & suffix,
 * std::vector<std::string>& params,
 * std::vector<std::string>& selections)
 * {
 * const rapio::Time rsTime = dt.getTime();
 * const std::string time_string = rsTime.getFileNameString();
 * const std::string dataType    = dt.getTypeName();
 * std::string spec;
 * dt.getSubType(spec);
 *
 * // Filename typical example:
 * // dirName/Reflectivity/00.50/TIME.netcdf
 * // dirName/TIME_Reflectivity_00.50.netcdf
 * std::stringstream fs;
 * if (!dfs->subdirs){
 *  // Without subdirs, name files with time first followed by details
 *  fs << time_string << "_" << dataType;
 *  if (!spec.empty()){ fs  << "_" << spec; }
 * }else{
 *  // With subdirs, name files datatype first in folders
 *  fs << dataType;
 *  if (!spec.empty()){ fs  << "/" << spec; }
 *  fs << "/" << time_string;
 * }
 * fs << "." << suffix;
 * const auto filepath = fs.str();
 *
 * // Create record params
 * params.push_back(suffix);
 * params.push_back(directory);
 * Strings::splitWithoutEnds(filepath, '/', &params);
 *
 * // Create record selections
 * selections.push_back(time_string);
 * selections.push_back(dataType);
 * if (!spec.empty()) {
 *  selections.push_back(spec);
 * }
 *
 * return URL(directory+"/"+filepath);
 * }
 */

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
  if (!OS::testFile(dir, OS::FILE_IS_DIRECTORY) && !OS::mkdirp(dir)) {
    LogSevere("Unable to create " << dir << "\n");
    return ("");
  }

  // Open netcdf file
  int ncid;
  int retval;

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
