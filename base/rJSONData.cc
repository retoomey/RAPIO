#include "rJSONData.h"

#include "rError.h"
#include "rStrings.h"

#include <boost/property_tree/json_parser.hpp>

using namespace rapio;
using namespace std;

JSONData::JSONData()
{
  myDataType = "JSON";
}

size_t
JSONData::writeBuffer(std::vector<char>& buf)
{
  std::stringstream ss;
  boost::property_tree::json_parser::write_json(ss, myRoot.node);
  std::string out = ss.str();
  buf = std::vector<char>(out.begin(), out.end());
  return buf.size();
}

bool
JSONData::writeURL(
  const URL & path,
  bool      shouldIndent,
  bool      console)
{
  // Formatting for humans
  auto settings = shouldIndent ? true : false;

  // .json means to console (can we pass this in)
  std::string base = path.getBaseName();
  Strings::toLower(base);
  console = base == ".json";

  if (console) {
    boost::property_tree::write_json(std::cout, myRoot.node, settings);
  } else {
    if (path.isLocal()) {
      boost::property_tree::write_json(path.toString(), myRoot.node, std::locale(), settings);
    } else {
      LogSevere("Can't write to a remote URL at " << path << "\n");
      return false;
    }
  }
  return true;
}

bool
JSONData::readBuffer(std::vector<char>& buffer)
{
  try{
    buffer.push_back('\0');
    std::istringstream is(&buffer.front());
    boost::property_tree::read_json(is, myRoot.node);
    return true;
  }catch (std::exception& e) { // pt::xml_parser::xml_parser_error
    // We catch all to recover
    LogSevere("Exception reading JSON data..." << e.what() << " ignoring\n");
    return false;
  }
}

LLH
JSONData::getLocation() const
{
  return (LLH());
}

Time
JSONData::getTime() const
{
  return (Time::CurrentTime());
}
