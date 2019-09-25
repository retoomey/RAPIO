#include "rIOXML.h"

#include "rStrings.h"
#include "rError.h"
#include "rIOURL.h"

#include <boost/property_tree/xml_parser.hpp>

using namespace rapio;

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
    }catch (std::exception e) { // pt::xml_parser::xml_parser_error
      // We catch all to recover
      LogSevere("Exception reading XML data..." << e.what() << " ignoring\n");
      return nullptr;
    }
  }
  return nullptr;
}
