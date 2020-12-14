#include "rXMLData.h"

#include "rError.h"
#include "rStrings.h"

#include <boost/property_tree/xml_parser.hpp>

using namespace rapio;
using namespace std;

XMLData::XMLData()
{
  myDataType = "XML";
  myTypeName = "XML"; // Affects output writing
}

bool
XMLData::writeURL(
  const URL & path,
  bool      shouldIndent,
  bool      console)
{
  // Formatting for humans
  auto settings = boost::property_tree::xml_writer_make_settings<std::string>(' ', shouldIndent ? 1 : 0);

  // .xml means to console (can we pass this in)
  std::string base = path.getBaseName();
  Strings::toLower(base);
  console = base == ".xml";

  if (console) {
    boost::property_tree::write_xml(std::cout, myRoot.node, settings);
  } else {
    if (path.isLocal()) {
      boost::property_tree::write_xml(path.toString(), myRoot.node, std::locale(), settings);
    } else {
      LogSevere("Can't write to a remote URL at " << path << "\n");
      return false;
    }
  }
  return true;
}

bool
XMLData::readBuffer(std::vector<char>& buffer)
{
  try{
    buffer.push_back('\0');
    std::istringstream is(&buffer.front());
    // Need to trim white space, or when we write we gets tons of blank lines.
    // Bug in boost or rapidxml I think.
    boost::property_tree::read_xml(is, myRoot.node, boost::property_tree::xml_parser::trim_whitespace);
    return true;
  }catch (const std::exception& e) { // pt::xml_parser::xml_parser_error
    // We catch all to recover
    LogSevere("Exception reading XML data..." << e.what() << " ignoring\n");
    return false;
  }
}

size_t
XMLData::writeBuffer(std::vector<char>& buf)
{
  std::stringstream ss;
  boost::property_tree::xml_parser::write_xml(ss, myRoot.node);
  std::string out = ss.str();
  buf = std::vector<char>(out.begin(), out.end());
  return buf.size();
}
