#include "rIONetcdf.h"

#include "rNetcdfUtil.h"
#include "rFactory.h"
#include "rBuffer.h"
#include "rIOURL.h"
#include "rOS.h"
#include "rStrings.h"
#include "rProcessTimer.h"

// Default built in DataType support
#include "rNetcdfRadialSet.h"
#include "rNetcdfLatLonGrid.h"
#include "rNetcdfBinaryTable.h"

#include <netcdf_mem.h>

#include <cstdio>
#include <cassert>

using namespace rapio;
using namespace std;

float IONetcdf::MISSING_DATA  = Constants::MissingData;
float IONetcdf::RANGE_FOLDED  = Constants::RangeFolded;
bool IONetcdf::CDM_COMPLIANCE = false;
bool IONetcdf::FAA_COMPLIANCE = false;

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

URL
IONetcdf::getFileName(const std::vector<std::string>& params, size_t start)
{
  URL loc;

  const size_t tot_parts(params.size());

  if (tot_parts == start) {
    LogSevere("IONetcdf: Params missing filename.\n");
    return (loc);
  }

  loc = params[start];

  for (size_t j = 1 + start; j < tot_parts; ++j) {
    loc.path += "/" + params[j];
  }

  return (loc);
}

std::shared_ptr<DataType>
IONetcdf::readNetcdfDataType(const std::vector<std::string>& args)
{
  // check the argument count
  if (args.size() < 1) {
    LogSevere("Not enough arguments to create the object!\n");
    return (0);
  }

  // Note, in RAPIO we can read a netcdf file remotely too
  const URL url(getFileName(args, 0));

  Buffer buf;
  IOURL::read(url, buf);

  if (!buf.empty()) {
    // Open netcdf directly from buffer memory
    int retval, ncid;
    retval = nc_open_mem("test", 0, buf.size(), &buf[0], &ncid);

    if (retval != NC_NOERR) {
      LogSevere("Error reading netcdf " << nc_strerror(retval) << "\n");
      nc_close(ncid);
      return (0);
    }

    // Get the datatype from netcdf
    std::string type;
    NETCDF(NetcdfUtil::getAtt(ncid, Constants::sDataType, type));

    if (type.empty()) {
      LogSevere("The 'DataType' netcdf attribute is not in netcdf file.\n");
      return (0);
    }

    // Get object creator from the datatype
    std::shared_ptr<NetcdfType> fmt = IONetcdf::getIONetcdf(type);

    if (fmt == 0) {
      LogSevere("Can't create IO reader for " << type << "\n");
      return (0);
    }

    // std::shared_ptr<DataType> dt = fmt->createObject(ncid, url, args);
    std::shared_ptr<DataType> dt = fmt->read(ncid, url, args);
    nc_close(ncid);

    return (dt);
  }
  return (0);
} // IONetcdf::readNetcdfDataType

std::string
IONetcdf::encodeStatic(const rapio::DataType& dt,
  const std::string                         & subtype,
  const std::string                         & myDirectory,
  bool                                      myUseSubDirs,
  std::vector<Record>                       & records)
{
  /** So myLookup "RadialSet" writer for example from the data type.
   * This allows algs etc to replace our IONetcdf with a custom if needed. */
  const std::string type = dt.getDataType();

  // Get object creator from the datatype
  std::shared_ptr<NetcdfType> fmt = IONetcdf::getIONetcdf(type);

  if (fmt == 0) {
    LogSevere("Can't create IO reader for " << type << "\n");
    return ("");
  }

  // std::stringstream ss;
  // ss << "Writing new " << type << " netcdf took...\n";
  // ProcessTimerInfo test(ss.str());
  rapio::Time rsTime = dt.getTime();

  // check if need to use FilenameDateTime.
  // This seems to be legacy, but maybe some ancient datasets have this?
  // rapio::Date filenameDateTime = dt.getFilenameDateTime();
  // Time filenameDateTime = dt.getFilenameDateTime();

  // if (filenameDateTime.isValid()) {
  // rsTime = filenameDateTime.getTime();
  // rsTime = filenameDateTime;
  // }

  // Filename, example:  dirName/Reflectivity/data_90820212023.netcdf
  // This means that dirName should encode the location somehow, example
  // via radar name
  // std::string time_string = Date(rsTime).get String(false);
  std::string time_string = rsTime.getFileNameString();
  std::string dataType    = dt.getTypeName();
  string fileName         = myDirectory + "/" + dataType;
  std::string spec        = subtype;

  // LogSevere("FILENAME DATETIME " << filenameDateTime << "\n");
  // LogSevere("FILENAME RSTIME " << rsTime << "\n");
  // LogSevere("FILENAME TIMESTRING " << time_string << "\n");

  if (spec == "-1") { spec = dt.getGeneratedSubtype(); }

  if (!spec.empty()) {
    // fileName += getSeparatorBetweenDataTypeAndSubtype() + spec;
    if (myUseSubDirs) { fileName += "/"; } else {
      fileName += "_";
    } fileName += spec;
  }

  // fileName += getSeparatorToTimeString() + time_string;
  if (myUseSubDirs) { fileName += "/"; } else { fileName += "_"; }
  fileName += time_string;

  // fileName += add;
  fileName += ".netcdf";

  // set up the selections (for record generation)
  std::vector<std::string> selections;
  selections.clear();
  selections.push_back(time_string);
  selections.push_back(dataType);

  if (spec.size() > 0) {
    selections.push_back(spec);
  }

  // ensure the directory exists
  const URL fileName_url(fileName);
  const std::string dir(fileName_url.getDirName());

  if (!OS::testFile(dir, OS::FILE_IS_DIRECTORY) && !OS::mkdirp(dir)) {
    LogSevere("Unable to create " << dir << "\n");
    return (0);
  }

  // Open netcdf file
  int ncid;
  int retval;

  // FIXME: should speed test this...
  try {
    NETCDF(nc_create(fileName_url.path.c_str(), NC_NETCDF4, &ncid));
  } catch (NetcdfException& ex) {
    nc_close(ncid);
    LogSevere("Netcdf create error: "
      << fileName_url.path << " " << ex.getNetcdfStr() << "\n");
    return ("");
  }

  if (ncid == -1) { return (""); }

  bool successful = false;

  try {
    successful = fmt->write(ncid, dt);
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
    Record rec;
    std::vector<std::string> params;

    // params.push_back(getFormatName());
    params.push_back("netcdf");
    params.push_back(myDirectory);
    std::string filename(fileName);
    assert(filename.substr(0, myDirectory.size()) == myDirectory);
    filename = filename.substr(myDirectory.size(),
        filename.size() - myDirectory.size());

    // split without ends because empty strings are pointless
    Strings::splitWithoutEnds(filename, '/', &params); // will add to list
    rec = Record(params, selections, rsTime);
    records.push_back(rec);

    // -------------------------------------------------------------
    return (fileName);
  }
  return ("");
} // IONetcdf::encodeStatic

std::string
IONetcdf::encode(const rapio::DataType& dt,
  const std::string                   & directory,
  bool                                useSubDirs,
  std::vector<Record>                 & records)
{
  return (encode(dt, "-1", directory, useSubDirs, records));
}

std::string
IONetcdf::encode(const rapio::DataType& dt,
  const std::string                   & subtype,
  const std::string                   & directory,
  bool                                useSubDirs,
  std::vector<Record>                 & records)
{
  // static.. ahhahhrghahg
  return (IONetcdf::encodeStatic(dt, subtype, directory, useSubDirs, records));
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
