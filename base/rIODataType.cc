#include "rIODataType.h"

#include "rFactory.h"
#include "rDataType.h"
#include "rStrings.h"
#include "rConfigDataFormat.h"
#include "rIOXML.h"

using namespace rapio;

// -----------------------------------------------------------------------------------------
// Reader stuff
//
std::shared_ptr<IODataType>
IODataType::getIODataType(const std::string& sourceType)
{
  return (Factory<IODataType>::get(sourceType, "IODataType source"));
}

URL
IODataType::getFileName(const std::vector<std::string>& params)
{
  URL loc;

  const size_t tot_parts(params.size());

  if (tot_parts < 1) {
    LogSevere("Params missing filename.\n");
    return (loc);
  }

  loc = params[0];

  for (size_t j = 1; j < tot_parts; ++j) {
    loc.path += "/" + params[j];
  }

  return (loc);
}

// -----------------------------------------------------------------------------------------
// Writer stuff
//

// static
URL
IODataType::generateOutputInfo(const DataType& dt,
  const std::string                          & directory,
  std::shared_ptr<DataFormatSetting>         dfs,
  const std::string                          & suffix,
  std::vector<std::string>                   & params,
  std::vector<std::string>                   & selections)
{
  const rapio::Time rsTime      = dt.getTime();
  const std::string time_string = rsTime.getFileNameString();
  const std::string dataType    = dt.getTypeName();

  std::string spec;
  dt.getSubType(spec);

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
  const auto filepath = fs.str();

  // Create record params
  params.push_back(suffix);
  params.push_back(directory);
  std::vector<std::string> fileparams;
  Strings::splitWithoutEnds(filepath, '/', &fileparams);
  for (auto f:fileparams) {
    params.push_back(f);
  }

  // Create record selections
  selections.push_back(time_string);
  selections.push_back(dataType);
  if (!spec.empty()) {
    selections.push_back(spec);
  }

  return URL(directory + "/" + filepath);
} // IODataType::generateOutputInfo

// static
std::string
IODataType::writeData(std::shared_ptr<DataType> dt,
  const std::string                             & outputDir,
  std::vector<Record>                           & records)
{
  // Settings are per datatype, so we need the unique settings for it right now
  std::shared_ptr<DataFormatSetting> dfs = ConfigDataFormat::getSetting(dt->getDataType());
  if (dfs != nullptr) {
    std::shared_ptr<IODataType> encoder = Factory<IODataType>::get(dfs->format,
        "IODataType writer");
    if (encoder != nullptr) {
      return (encoder->encode(dt, outputDir, dfs, records));
    } else {
      LogSevere("No IODataType for '" << dfs->format << "', I can't write the data\n");
    }
  }
  return ("");
}
