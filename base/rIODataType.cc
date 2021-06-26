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
  const std::string                                    & basepattern)
{
  const rapio::Time rsTime   = dt->getTime();
  const std::string dataType = dt->getTypeName();
  const std::string subType  = dt->getSubType();

  // Get absolute path in input, make sure it's in directory format
  // This might break for non-existent paths need to check
  URL forFull = URL(outputinfo + "/up");

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

  // Example: dirName/Reflectivity/00.50/TIMESTRING.netcdf
  // How about a PATTERN instead, let's try it out
  // First replace meta datatype terms with datatype values,
  // then pass on to the pickier time filter
  // std::string p = "{base}/{datatype}{/subtype}/{time}";
  std::string p = basepattern;

  // Example: dirName/TIMESTRING_Reflectivity_00.50.netcdf (the subdirs from WDSS2)
  // std::string p = "{base}/{time}_{datatype}{_subtype}";

  Strings::replace(p, "{base}", dirbase);
  Strings::replace(p, "{datatype}", dataType);
  // Specials for subtype where we don't want blank delimiter.  Maybe there's a
  // cleaner way, can explore in the future.  We support / and _ for optional subtype
  Strings::replace(p, "{/subtype}", subType.empty() ? ("") : ('/' + subType));
  Strings::replace(p, "{_subtype}", subType.empty() ? ("") : ('_' + subType));
  Strings::replace(p, "{subtype}", subType);
  Strings::replace(p, "{time}", Record::RECORD_TIMESTAMP); // Note you could use the time fields independently
  URL path = URL(rsTime.getString(p));

  // Ensure directory tree exists for this path.  Should be done by caller I think...because this might
  // be a bucket prefix instead..muahhahhaa
  OS::ensureDirectory(path.getDirName());

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

  bool directFile = false;
  // Temp hack
  if (directFile) {
    outputParams["filepathmode"] = "direct";
  } else {
    outputParams["filepathmode"] = "datatype";
  }

  return encoder->writeout(dt, outputinfo, records, f, outputParams);
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

  // Parse the command line options, such as
  // PYTHON=folder,scriptname or
  // NETCDF=folder
  // Note: This means currently output folder/factory is always forced by command line
  handleCommandParam(outputinfo, outputParams);
  const std::string folder = outputParams["outputfolder"];

  // New idea, file path mode for generalizing.  I want to expand for 'other' paths like S3
  // The code for ensuring directory need to not use with S3 obviously
  std::string filePathMode = outputParams["filepathmode"];
  URL aURL;
  bool directFile;
  bool ensureDir = false;
  if (filePathMode == "datatype") { // default datatype tree pathing
    std::string prefix = outputParams["fileprefix"];
    if (prefix.empty()) { prefix = DataType::DATATYPE_PREFIX; }
    // Generate a path/base filename using timestamps of datatype in a subtype tree
    aURL       = generateFileName(dt, folder, prefix);
    ensureDir  = true;
    directFile = false;
  } else if (filePathMode == "direct") {
    // Write a direct file name (folder here assumed to the the actual full file path)
    aURL       = URL(folder);
    ensureDir  = true;
    directFile = true;
  } else {
    LogSevere("Unrecognized file pathing mode: '" << filePathMode << "', cannot write output\n");
    return false;
  }

  // For local files we need directory tree for writing
  if (ensureDir) {
    std::string dirpath = aURL.getDirName();
    if (!OS::ensureDirectory(dirpath)) {
      LogSevere("Cannot create output directory: '" << dirpath << "', cannot write output\n");
      return false;
    }
  }

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
  // Called to directly write single file by system to xml/json/txt, this is writing
  // without notification, etc.
  std::vector<Record> blackHole;
  std::map<std::string, std::string> outputParams;
  outputParams["filepathmode"] = "direct";
  return write(dt, outputinfo, blackHole, factory, outputParams); // Default write single file
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
