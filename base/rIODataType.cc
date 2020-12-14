#include "rIODataType.h"

#include "rFactory.h"
#include "rDataType.h"
#include "rStrings.h"
#include "rConfigDataFormat.h"
#include "rOS.h"

using namespace rapio;

// -----------------------------------------------------------------------------------------
// Reader stuff
//

std::shared_ptr<DataType>
IODataType::readDataType(const URL& path, const std::string& factory)
{
  std::string f = factory;
  if (f == "") {
    // Snag from end of filename.
    f = OS::getFileExtension(path.toString());
    Strings::removePrefix(f, ".");
    Strings::toLower(f);
  }
  auto builder = Factory<IODataType>::get(f, "IODataType source");
  if (builder == nullptr) {
    LogSevere("No builder corresponding to '" << f << "'\n");
    return nullptr;
  } else {
    std::shared_ptr<DataType> dt = builder->createDataType(path);
    if (dt != nullptr) {
      // Old W2ALGS is XML:
      if (f == "W2ALGS") { f = "XML"; }
      dt->setReadFactory(f);
    }
    return dt;
  }
}

// -----------------------------------------------------------------------------------------
// Writer stuff
//

bool
IODataType::write(std::shared_ptr<DataType> dt, const URL& aURL,
  bool generateFileName,
  std::vector<Record>& records,
  std::vector<std::string>& files,
  const std::string& factory)
{
  URL path = aURL;

  // FIXME: Need to refactor/clean up for multi-write ability
  // We should have ability to say write a radialset to png, netcdf, etc.
  // We should have ability to turn on/off records for this too I think.

  // 1. Determine factory we use to attempt to write this data
  // LogSevere("Write called url: "+path.toString()+"\n");
  std::string f = factory; // FORCE (which can fail)
  if (f == "") {           // Guess
    // Check if data type has the read in factory info
    // Note it could be set elsewhere to force a particular output
    // type attempt.  For example, mirroring data files we'd just use
    // same type as in
    const auto rf = dt->getReadFactory();
    if (rf != "default") {
      f = rf; // Not default, use it (assumes readers can write)
    } else {
      // Doesn't work for directory right? Could skip this then
      f = OS::getFileExtension(path.toString());
      Strings::removePrefix(f, ".");
    }
  }
  Strings::toLower(f);
  std::string suffix = f; // This won't be true with extra compression, such as .xml.gz

  // The dfs settings from WDSS2 are messy, need refactor SOON.
  // WDSS2 assumes 1 type to 1 output.  I'm thinking the settings should be set per factory.
  // Read setting for datatype from dfs file
  std::shared_ptr<DataFormatSetting> dfs = ConfigDataFormat::getSetting(dt->getDataType());
  if (dfs != nullptr) {
    // FIXME: The issue is that old WDSS2 has the default to netcdf, which stuff like xml
    // won't write out to.
    // f = dfs->format;
  }

  std::shared_ptr<IODataType> encoder = Factory<IODataType>::get(f, "IODataType writer");
  if (encoder == nullptr) {
    LogSevere("No builder corresponding to '" << f << "'\n");
    return false;
  }

  // Used for record and auto generation
  const rapio::Time rsTime      = dt->getTime();
  const std::string time_string = rsTime.getFileNameString();
  const std::string dataType    = dt->getTypeName();
  std::string spec;
  dt->getSubType(spec);

  // If a directory passed in, we generate the filename based on time, etc.
  // else if actual file, use it directly
  std::string filepath;
  std::string sfilepath;
  std::string dirpath; // root dir

  if (generateFileName) {
    // LogSevere("Directory passed in, generating file name\n");
    dirpath = path.toString();

    // Filename typical example:
    // dirName/Reflectivity/00.50/TIME.netcdf
    // dirName/TIME_Reflectivity_00.50.netcdf
    std::stringstream fs;
    if (!dfs->subdirs) {
      // Without subdirs, name files with time first followed by details
      fs << time_string << "_" << dataType;
      if (!spec.empty()) { fs << "_" << spec; }
    } else {
      // With subdirs, name files datatype first in folders
      fs << dataType;
      if (!spec.empty()) { fs << "/" << spec; }
      fs << "/" << time_string;
    }
    fs << "." << suffix;
    sfilepath = fs.str();
    filepath  = dirpath + "/" + sfilepath; // full path to file
  } else {
    LogSevere("File passed in using it directly\n");
    dirpath   = path.getDirName(); // up a '/'
    filepath  = path.toString();
    sfilepath = URL(filepath).getBaseName();
    // FAMS for these might have param issues (bad WDSS2 design)
  }

  // Directory must exist. We need getDirName to get added subdirs
  const std::string dir(URL(filepath).getDirName());
  if (!OS::isDirectory(dir) && !OS::mkdirp(dir)) {
    LogSevere("Unable to create directory: " << dir << "\n");
    return false;
  }
  // LogSevere("Directory:"+dirpath+"\n");
  // LogSevere("File:"+filepath+"\n");
  // LogSevere("SFile:"+sfilepath+"\n");
  // LogSevere("Factory name is "+f+"\n");

  // FIXME: Need to write/move files
  path = URL(filepath);
  // LogSevere("URL:"+path.toString()+"\n");
  if (encoder->encodeDataType(dt, path, dfs)) {
    // Create record params
    std::vector<std::string> params;
    params.push_back(suffix);
    params.push_back(dirpath);
    std::vector<std::string> fileparams;
    Strings::splitWithoutEnds(sfilepath, '/', &fileparams);
    for (auto f:fileparams) {
      params.push_back(f);
    }

    // Create record selections
    std::vector<std::string> selections;
    selections.push_back(time_string);
    selections.push_back(dataType);
    if (!spec.empty()) {
      selections.push_back(spec);
    }

    Record rec(params, selections, rsTime);
    records.push_back(rec);
    // Final written file path.  Doing it this way
    // in case we move or compress later
    files.push_back(path.toString());
    return true;
  }

  return false;
} // IODataType::write

bool
IODataType::write(std::shared_ptr<DataType> dt, const URL& aURL, const std::string& factory)
{
  std::vector<Record> blackHole;
  std::vector<std::string> files;
  return write(dt, aURL, false, blackHole, files, factory); // Default write single file
}
