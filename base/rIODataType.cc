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
    return dt;
  }
}

// -----------------------------------------------------------------------------------------
// Writer stuff
//

bool
IODataType::write(std::shared_ptr<DataType> dt, const URL& aURL,
  bool generateFileName,
  std::vector<Record>& records, const std::string& factory)
{
  URL path = aURL;

  // LogSevere("Write called url: "+path.toString()+"\n");
  std::string f = factory;
  if (f == "") {
    // Doesn't work for directory right? Could skip this then
    f = OS::getFileExtension(path.toString());
    Strings::removePrefix(f, ".");
  }
  Strings::toLower(f);
  std::string suffix = f; // This might not always be true

  // Need to fix/redesign how this works.
  // We don't want 'xml' turned into 'netcdf' for example....
  // So a datatype like "RadialSet" might have special settings
  std::shared_ptr<DataFormatSetting> dfs = ConfigDataFormat::getSetting(dt->getDataType());
  if (dfs != nullptr) {
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

  path = URL(filepath);
  // LogSevere("URL:"+path.toString()+"\n");
  if (encoder->encodeDataType(dt, path, dfs)) {
    // Create record params
    std::vector<std::string> params;
    params.push_back(suffix);
    params.push_back(dirpath);
    std::vector<std::string> fileparams;

    Strings::splitWithoutEnds(sfilepath, '/', &fileparams);
    // Create record selections
    std::vector<std::string> selections;
    selections.push_back(time_string);
    selections.push_back(dataType);
    if (!spec.empty()) {
      selections.push_back(spec);
    }
    for (auto f:fileparams) {
      params.push_back(f);
    }
    Record rec(params, selections, rsTime);
    records.push_back(rec);
    return true;
  }

  return false;
} // IODataType::write

bool
IODataType::write(std::shared_ptr<DataType> dt, const URL& aURL, const std::string& factory)
{
  std::vector<Record> blackHole;
  return write(dt, aURL, false, blackHole, factory); // Default write single file
}
