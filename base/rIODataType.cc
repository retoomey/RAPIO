#include "rIODataType.h"

#include "rConfigIODataType.h"
#include "rColorTerm.h"
#include "rFactory.h"
#include "rDataType.h"
#include "rStrings.h"
#include "rOS.h"

using namespace rapio;

std::string
IODataType::introduceHelp()
{
  std::string help;
  help +=
    "Default is netcdf output folder, or the same as netcdf=/path.  Multiple writers with multiple or same folders can be chained.  For example, image=folder1 gdal=folder2.";
  help +=
    "  The key= part is the builder, these are typically in metadata files (index, fml) as the first part of the <params> section.";
  help += "  Some builders are static, some are dynamically loaded from rapiosettings.xml on demand.\n";
  help += ColorTerm::fBlue + "Static built-in builders:" + ColorTerm::fNormal + "\n";
  auto full = Factory<IODataType>::getAll();
  for (auto a:full) {
    help += " " + ColorTerm::fRed + a.first + ColorTerm::fNormal + " : " + a.second->getHelpString(a.first) + "\n";
    // shouldn't be null, rightmyList.push_back(ele.second);
  }
  help += ColorTerm::fBlue + "Dynamic loaded builders:" + ColorTerm::fBlue + "\n";
  // We're fully loading now...but that's ok help dies afterwards
  auto fulld = Factory<IODataType>::getAllDynamic();
  for (auto ele:fulld) {
    auto& f = ele.second;
    help += ColorTerm::fRed + " ";
    for (auto& s:f.alias) {
      help += s + " ";
    }
    help += ColorTerm::fNormal + " : ";
    if (f.stored == nullptr) {
      help += " couldn't find/load this module!!!";
    } else {
      help += f.stored->getHelpString("");
    }
    help += "\n";
  }
  return help;
}

// -----------------------------------------------------------------------------------------
// Reader stuff
//

std::shared_ptr<IODataType>
IODataType::getFactory(std::string& factory, const std::string& path, std::shared_ptr<DataType> dt)
{
  // 1. Use the suffix lookup table for empty factory
  if (factory.empty()) {
    const std::string suffix = OS::getRootFileExtension(path);
    factory = ConfigIODataType::getIODataTypeFromSuffix(suffix);
  }

  // 2. If still no factory, and the datatype exists, try to use the read factory for output
  if (factory.empty()) {
    if (dt != nullptr) {
      const auto rf = dt->getReadFactory();
      if (rf != "default") {
        factory = rf; // Not default, use it (assumes readers can write)
      }
    }
  }

  // 3. If STILL empty, try the suffix as a factory...might get lucky
  if (factory.empty()) {
    factory = OS::getRootFileExtension(path);
    LogSevere("Using file suffix as builder guess, might put this in configuration file\n");
  }

  Strings::toLower(factory);
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
      if (f == "w2algs") { f = "xml"; }
      dt->setReadFactory(f);
    }
    return dt;
  } else {
    LogSevere("No builder IO module loaded for type '" << f << "'\n");
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
      if (f == "w2algs") { f = "xml"; }
      dt->setReadFactory(f);
    }
    return dt;
  } else {
    LogSevere("No builder IO module loaded for type '" << f << "'\n");
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

    // FIXME:  My temp hack for multi-radar output to avoid data overwrite stomping
    // We should generalize file output ability with a smart configuration
    // instead of hardcoding it here
    // This is bug in mrms imo, so may have to change it there as well
    std::string radar = "";
    dt->getString("radarName-value", radar);

    if (!radar.empty()) {
      dirbase = dirbase + '/' + radar;
    }

    if (useSubDirs) {
      // Example: dirName/Reflectivity/00.50/TIMESTRING.netcdf
      const std::string extra = subType.empty() ? ("") : ('/' + subType);
      filepath = dirbase + '/' + dataType + extra + '/' + time_string; //  + '.' + suffix;
    } else {
      // Example: dirName/TIMESTRING_Reflectivity_00.50.netcdf
      const std::string extra = subType.empty() ? ('.' + suffix) : ('_' + subType); //  + '.' + suffix);
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
  std::string radar;
  if (dt->getString("radarName-value", radar)) {
    rec.setSourceName(radar);
  }
  records.push_back(rec);
} // IODataType::generateRecord

// -----------------------------------------------------------------------------------------
// Writer stuff
//

bool
IODataType::write(std::shared_ptr<DataType> dt,
  const std::string & outputinfo,
  bool directFile,
  std::vector<Record> & records,
  const std::string & factory,
  std::map<std::string, std::string>  & outputParams)
{
  // The static method grabs the first factory and sends to it to handle
  // This is assuming outputinfo is a file name?
  // 1. Get the factory for this output
  std::string f = factory; // either passed in, or blank or suffix guess stuff.
  auto encoder  = getFactory(f, outputinfo, dt);
  if (encoder == nullptr) {
    LogSevere("Unable to write using unknown factory '" << f << "'\n");
    return false;
  }
  return encoder->writeout(dt, outputinfo, directFile, records, f, outputParams);
} // IODataType::write

void
IODataType::handleCommandParam(const std::string& command,
  std::map<std::string, std::string> &outputParams)
{
  // The default is factory=outputfolder.  Python for example splits
  // the command param into script,outputfolder
  outputParams["outputfolder"] = command;
}

bool
IODataType::writeout(std::shared_ptr<DataType> dt,
  const std::string & outputinfo,
  bool directFile,
  std::vector<Record> & records,
  const std::string & knownfactory,
  std::map<std::string, std::string>  & outputParams)
{
  // Add any output fields not explicitly set by caller
  // Basically outputParams overrides settings from rapiosettings.xml
  // I think this design is simplier in long run.
  std::shared_ptr<PTreeNode> dfs = ConfigIODataType::getSettings(knownfactory);
  if (dfs != nullptr) {
    try{
      auto output = dfs->getChild("output");
      auto map    = output.getAttrMap();
      for (auto& x:map) {
        if (outputParams.count(x.first) == 0) {
          outputParams[x.first] = x.second;
        }
      }
    }catch (const std::exception& e) {
      // it's ok, use the passed params
    }
  }

  // We'll handle generating filename and paths required here.
  // Why does this require suffix?
  bool useSubDirs = true; // Use subdirs (should be a flag on us right?)
  // I don't think we 'need' the suffix here...
  // Writer should modify the filename....
  std::string suffix = "";

  // Parse/get output folder for writing directory
  handleCommandParam(outputinfo, outputParams);
  const std::string folder = outputParams["outputfolder"];
  URL aURL = generateFileName(dt, folder, suffix, directFile, useSubDirs);

  // Here we go right?  Chicken egg issue with suffix I think...
  outputParams["filename"] = aURL.toString(); // base filename.  Writer determines ending unless directFile right?

  // FIXME: direct files vs generated..bleh. One has suffix, one doesn't
  // I still need to 'fix' this I think
  outputParams["directfile"] = directFile ? "true" : "false"; // don't like this right now it's a suffix flag

  // Pass map to children.  Note: children can use the map to reply back to caller as well
  bool success = encodeDataType(dt, outputParams);

  // Generate a notification record on successful write of output file
  if (success) {
    const std::string finalFile = outputParams["filename"];
    if (!finalFile.empty()) {
      // FIXME: ok so we could not create record in first place if not wanted, right?
      IODataType::generateRecord(dt, finalFile, knownfactory, records);
    }
  }

  return success;
} // IODataType::writeout

bool
IODataType::write(std::shared_ptr<DataType> dt, const std::string& outputinfo, const std::string& factory)
{
  std::vector<Record> blackHole;
  std::map<std::string, std::string> empty;
  return write(dt, outputinfo, true, blackHole, factory, empty); // Default write single file
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
