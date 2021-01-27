#include "rIODataType.h"

#include "rConfigIODataType.h"
#include "rFactory.h"
#include "rDataType.h"
#include "rStrings.h"
#include "rOS.h"

using namespace rapio;

// -----------------------------------------------------------------------------------------
// Reader stuff
//

std::shared_ptr<IODataType>
IODataType::getFactory(std::string& factory, const std::string& path, std::shared_ptr<DataType> dt)
{
  if (factory.empty()) {
    // 1. If the datatype exists, try to get from the known read factory
    if (dt != nullptr) {
      const auto rf = dt->getReadFactory();
      if (rf != "default") {
        factory = rf; // Not default, use it (assumes readers can write)
      }
    }

    // 2. Try the suffix of file we might get lucky

    // For the moment hard use the suffix.  This will work for
    // maybe xml, netcdf..not for more advanced classes.
    // xml --> xml
    // netcdf --> netcdf
    // png --> image
    // png.tar.gz --> image
    // jpg --> image
    // tif --> image or gdal for example
    // FIXME: maybe a canHandle method for all registered encoders...
    // FIXME: or a table in settings.  A mapping could work
    if (factory.empty()) {
      factory = OS::getFileExtension(path);
      Strings::removePrefix(factory, ".");
    }
    Strings::toLower(factory);
  }

  return Factory<IODataType>::get(factory, "IODataType:" + factory);
}

std::shared_ptr<DataType>
IODataType::readBufferImp(std::vector<char>& buffer, const std::string& factory)
{
  std::string f = factory;
  auto builder  = getFactory(f, "", nullptr);
  if (builder != nullptr) {
    // Create DataType and remember factory
    std::shared_ptr<DataType> dt = builder->createDataTypeFromBuffer(buffer);
    if (dt != nullptr) {
      // Old W2ALGS is XML:
      if (f == "W2ALGS") { f = "xml"; }
      dt->setReadFactory(f);
    }
    return dt;
  }
  return nullptr;
}

std::shared_ptr<DataType>
IODataType::readDataType(const std::string& factoryparams, const std::string& factory)
{
  std::string f = factory;
  auto builder  = getFactory(f, factoryparams, nullptr);

  if (builder != nullptr) {
    // Create DataType and remember factory
    std::shared_ptr<DataType> dt = builder->createDataType(factoryparams);
    if (dt != nullptr) {
      // Old W2ALGS is XML:
      if (f == "W2ALGS") { f = "xml"; }
      dt->setReadFactory(f);
    }
    return dt;
  }
  return nullptr;
}

URL
IODataType::generateFileName(std::shared_ptr<DataType> dt,
  const std::string                                    & outputinfo,
  const std::string                                    & suffix,
  bool                                                 directFile,
  bool                                                 useSubDirs)
{
  URL path;

  std::string filepath;  // Full file path  // Why not just generate this?
  std::string sfilepath; // short file path (aren't these pullable from full?, yes)
  std::string dirpath;   // directory path

  if (!directFile) {
    const rapio::Time rsTime      = dt->getTime();
    const std::string time_string = rsTime.getString(Record::RECORD_TIMESTAMP);
    const std::string dataType    = dt->getTypeName();
    const std::string subType     = dt->getSubType();

    // Get absolute path in input, make sure it's a directory
    URL forFull         = URL(outputinfo + "/up");
    std::string dirbase = forFull.getDirName();
    if (useSubDirs) {
      // Example: dirName/Reflectivity/00.50/TIMESTRING.netcdf
      const std::string extra = subType.empty() ? ("") : ('/' + subType);
      filepath = dirbase + '/' + dataType + extra + '/' + time_string + '.' + suffix;
    } else {
      // Example: dirName/TIMESTRING_Reflectivity_00.50.netcdf
      const std::string extra = subType.empty() ? ('.' + suffix) : ('_' + subType + '.' + suffix);
      filepath = dirbase + '/' + time_string + '_' + dataType + extra;
    }
    path = URL(filepath);
  } else {
    path = URL(outputinfo);
  }

  // Final output?
  dirpath   = path.getDirName();
  filepath  = path.toString();
  sfilepath = path.getBaseName();

  // Directory must exist. We need getDirName to get added subdirs
  // const std::string dir(URL(filepath).getDirName()); // isn't this dirpath??
  if (!OS::isDirectory(dirpath) && !OS::mkdirp(dirpath)) {
    LogSevere("Unable to create directory: " << dirpath << "\n");
  }
  return path;
} // IODataType::generateFileName

void
IODataType::generateRecord(std::shared_ptr<DataType> dt,
  const URL                                          & pathin,
  const std::string                                  & factory,
  std::vector<Record>                                & records
)
{
  std::string dirpath   = pathin.getDirName();
  std::string filepath  = pathin.toString();
  std::string sfilepath = pathin.getBaseName();

  const rapio::Time rsTime      = dt->getTime();
  const std::string time_string = rsTime.getString(Record::RECORD_TIMESTAMP);
  const std::string dataType    = dt->getTypeName();
  const std::string spec        = dt->getSubType();

  // Create record params
  std::vector<std::string> params;

  // Always add the builder/factory
  params.push_back(factory);

  // MRMS breaks it up with '/' but I don't think we actually need to do that
  // I think I'll modify MRMS to handle our records as one line if needed,
  // this simplifies the design and records for future non-file destinations.
  // FIXME: We need absolute path here, or the indexlocation thing.
  params.push_back(filepath);

  /*
   * params.push_back(dirpath);
   * std::vector<std::string> fileparams;
   * Strings::splitWithoutEnds(sfilepath, '/', &fileparams);
   * for (auto f:fileparams) {
   * params.push_back(f);
   * }
   */

  // Create record selections
  std::vector<std::string> selections;

  selections.push_back(time_string);
  selections.push_back(dataType);
  if (!spec.empty()) {
    selections.push_back(spec);
  }
  Record rec(params, selections, rsTime);
  records.push_back(rec);
} // IODataType::generateRecord

// -----------------------------------------------------------------------------------------
// Writer stuff
//

bool
IODataType::write(std::shared_ptr<DataType> dt,
  const std::string                         & outputinfo,
  bool                                      directFile,
  std::vector<Record>                       & records,
  const std::string                         & factory)
{
  // 1. Get the factory for this output
  std::string f = factory;
  auto encoder  = getFactory(f, outputinfo, dt);
  if (encoder == nullptr) { return false; }

  // 2. Try to get the rapiosettings info for the factory
  std::string suffix = f; // This won't be true with extra compression, such as .xml.gz
  std::shared_ptr<PTreeNode> dfs = ConfigIODataType::getSettings(f);

  // 4. Output file and generate records
  return (encoder->encodeDataType(dt, outputinfo, dfs, directFile, records));
} // IODataType::write

bool
IODataType::write(std::shared_ptr<DataType> dt, const std::string& outputinfo, const std::string& factory)
{
  std::vector<Record> blackHole;
  return write(dt, outputinfo, true, blackHole, factory); // Default write single file
}

size_t
IODataType::writeBuffer(std::shared_ptr<DataType> dt,
  std::vector<char>& buffer, const std::string& factory)
{
  // 1. Get the factory for this output
  std::string f = factory;
  auto encoder  = getFactory(f, "", dt);
  // FIXME: Pass settings?  For now just XML/JSON use I'll skip it
  if (encoder == nullptr) { return 0; }
  // 2. Output file and generate records
  return (encoder->encodeDataTypeBuffer(dt, buffer));
}
