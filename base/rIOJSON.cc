#include "rIOJSON.h"

#include "rFactory.h"
#include "rStrings.h"
#include "rError.h"
#include "rIOURL.h"

#include <boost/property_tree/json_parser.hpp>

using namespace rapio;

void
IOJSON::introduceSelf()
{
  std::shared_ptr<IOJSON> newOne = std::make_shared<IOJSON>();
  Factory<IODataType>::introduce("json", newOne);

  // Add the default classes we handle...
}

/** Read call */
std::shared_ptr<DataType>
IOJSON::readJSONDataType(const std::vector<std::string>& args)
{
  std::shared_ptr<DataType> datatype = nullptr;

  // Note, in RAPIO we can read a json file remotely too
  const URL url = getFileName(args);
  std::vector<char> buf;
  IOURL::read(url, buf);

  if (!buf.empty()) {
    LogSevere("JSON UNIMPLEMENTED READ CALLED!\n");
  } else {
    LogSevere("Unable to pull data from " << url << "\n");
  }
  return (datatype);
}

/** Read call */
std::shared_ptr<DataType>
IOJSON::createObject(const std::vector<std::string>& args)
{
  return (IOJSON::readJSONDataType(args));
}

bool
IOJSON::writeURL(
  const URL                  & path,
  boost::property_tree::ptree& tree,
  bool                       shouldIndent)
{
  auto settings = shouldIndent ? true : false;

  // .xml means to console
  std::string base = path.getBaseName();
  Strings::toLower(base);
  bool console = base == ".json";
  if (console) {
    boost::property_tree::write_json(std::cout, tree, settings);
  } else {
    if (path.isLocal()) {
      boost::property_tree::write_json(path.toString(), tree, std::locale(), settings);
    } else {
      LogSevere("Can't write to a remote URL at " << path << "\n");
      return false;
    }
  }
  return true;
}

std::shared_ptr<boost::property_tree::ptree>
IOJSON::readURL(const URL& url)
{
  std::vector<char> buf;

  if (IOURL::read(url, buf) > 0) {
    buf.push_back('\0');
    std::istringstream is(&buf.front());

    std::shared_ptr<boost::property_tree::ptree> pt = std::make_shared<boost::property_tree::ptree>();
    try{
      boost::property_tree::read_json(is, *pt);
      return pt;
    }catch (std::exception& e) { // pt::json_parser::json_parser_error
      // We catch all to recover
      LogSevere("Exception reading JSON data..." << e.what() << " ignoring\n");
      return nullptr;
    }
  }
  return nullptr;
}

std::string
IOJSON::encode(std::shared_ptr<DataType> dt,
  const std::string                      & directory,
  std::shared_ptr<DataFormatSetting>     dfs,
  std::vector<Record>                    & records)
{
  // FIXME: Do we need this to be static?
  // return (IONetcdf::writeNetcdfDataType(dt, directory, dfs, records));
  LogSevere("JSON ENCODE OBJECT CALLED!!! YAY\n");
  return nullptr;
}
