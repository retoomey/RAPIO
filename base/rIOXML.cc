#include "rIOXML.h"

#include "rFactory.h"
#include "rStrings.h"
#include "rError.h"
#include "rIOURL.h"

#include <boost/property_tree/xml_parser.hpp>

using namespace rapio;

void
IOXML::introduceSelf()
{
  std::shared_ptr<IOXML> newOne = std::make_shared<IOXML>();

  // Read can write netcdf/netcdf3
  Factory<IODataType>::introduce("xml", newOne);
  Factory<IODataType>::introduce("W2ALGS", newOne); // Old school

  // Add the default classes we handle...
}

std::shared_ptr<DataType>
IOXML::readXMLDataType(const std::vector<std::string>& args)
{
  std::shared_ptr<DataType> datatype = nullptr;

  // Note, in RAPIO we can read a xml file remotely too
  const URL url = getFileName(args);
  std::vector<char> buf;
  IOURL::read(url, buf);

  if (!buf.empty()) {
    LogSevere("XML UNIMPLEMENTED READ OBJECT CALLED!!!\n");
  } else {
    LogSevere("Unable to pull data from " << url << "\n");
  }
  return (datatype);
}

/** Read call */
std::shared_ptr<DataType>
IOXML::createObject(const std::vector<std::string>& args)
{
  return (IOXML::readXMLDataType(args));
}

bool
IOXML::writeURL(
  const URL                  & path,
  boost::property_tree::ptree& tree,
  bool                       shouldIndent)
{
  // Formatting for humans
  auto settings = boost::property_tree::xml_writer_make_settings<std::string>(' ', shouldIndent ? 1 : 0);

  // .xml means to console
  std::string base = path.getBaseName();
  Strings::toLower(base);
  bool console = base == ".xml";
  if (console) {
    boost::property_tree::write_xml(std::cout, tree, settings);
  } else {
    if (path.isLocal()) {
      boost::property_tree::write_xml(path.toString(), tree, std::locale(), settings);
    } else {
      LogSevere("Can't write to a remote URL at " << path << "\n");
      return false;
    }
  }
  return true;
}

std::shared_ptr<boost::property_tree::ptree>
IOXML::readURL(const URL& url)
{
  std::vector<char> buf;

  if (IOURL::read(url, buf) > 0) {
    buf.push_back('\0');
    std::istringstream is(&buf.front());

    std::shared_ptr<boost::property_tree::ptree> pt = std::make_shared<boost::property_tree::ptree>();
    try{
      boost::property_tree::read_xml(is, *pt);
      return pt;
    }catch (std::exception& e) { // pt::xml_parser::xml_parser_error
      // We catch all to recover
      LogSevere("Exception reading XML data..." << e.what() << " ignoring\n");
      return nullptr;
    }
  }
  return nullptr;
}

std::string
IOXML::encode(std::shared_ptr<DataType> dt,
  const std::string                     & directory,
  std::shared_ptr<DataFormatSetting>    dfs,
  std::vector<Record>                   & records)
{
  // FIXME: Do we need this to be static?
  // return (IONetcdf::writeNetcdfDataType(dt, directory, dfs, records));
  LogSevere("XML UNIMPLEMENTED ENCODE OBJECT CALLED!!! YAY\n");
  return nullptr;
}
