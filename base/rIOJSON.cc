#include "rIOJSON.h"
#include "rPTreeData.h"

#include "rFactory.h"
#include "rStrings.h"
#include "rError.h"
#include "rIOURL.h"
#include "rOS.h"

#include <boost/property_tree/json_parser.hpp>

using namespace rapio;

std::string
IOJSON::getHelpString(const std::string & key)
{
  std::string help;

  help = "builder for reading JSON formatted data.\n";
  return help;
}

std::shared_ptr<DataType>
IOJSON::createDataTypeFromBuffer(std::vector<char>& buffer)
{
  return (readPTreeDataBuffer(buffer));
}

size_t
IOJSON::encodeDataTypeBuffer(std::shared_ptr<DataType> dt, std::vector<char>& buffer)
{
  std::shared_ptr<PTreeData> ptree = std::dynamic_pointer_cast<PTreeData>(dt);

  if (ptree) {
    return (writePTreeDataBuffer(ptree, buffer));
  }
  return 0;
}

std::shared_ptr<PTreeData>
IOJSON::readPTreeDataBuffer(std::vector<char>& buffer)
{
  try{
    std::shared_ptr<PTreeData> d = std::make_shared<PTreeData>();
    auto& n = d->getTree()->node;
    // To keep sizes correct, only add a 0 iff there isn't one there already
    if ((buffer.size() > 0) && (buffer[buffer.size() - 1] != 0)) {
      buffer.push_back('\0');
    }
    std::istringstream is(&buffer.front());
    boost::property_tree::read_json(is, n);
    return d;
  }catch (const std::exception& e) { // pt::xml_parser::xml_parser_error
    // We catch all to recover
    LogSevere("Exception reading JSON data..." << e.what() << " ignoring\n");
  }
  return nullptr;
}

/** Read call */
std::shared_ptr<DataType>
IOJSON::createDataType(const std::string& params)
{
  // We only read file/url
  const URL url(params);

  std::shared_ptr<DataType> datatype = nullptr;
  std::vector<char> buf;

  if (IOURL::read(url, buf) > 0) {
    std::shared_ptr<PTreeData> json = readPTreeDataBuffer(buf);
    if (json) {
      return json;
    }
  }
  LogSevere("Unable to create JSON from " << url << "\n");
  return nullptr;
}

size_t
IOJSON::writePTreeDataBuffer(std::shared_ptr<PTreeData> d, std::vector<char>& buf)
{
  // We know that PTree hides a boost node...
  const auto& n = d->getTree()->node;

  std::stringstream ss;

  boost::property_tree::json_parser::write_json(ss, n);
  std::string out = ss.str();

  buf = std::vector<char>(out.begin(), out.end());
  // To keep sizes correct, only add a 0 iff there isn't one there already
  if ((buf.size() > 0) && (buf[buf.size() - 1] != 0)) {
    buf.push_back('\0');
  }
  return buf.size();
}

bool
IOJSON::writeURL(
  const URL                  & path,
  std::shared_ptr<PTreeData> tree,
  bool                       shouldIndent,
  bool                       console)
{
  // We know that PTree hides a boost node...
  const auto& n = tree->getTree()->node;

  // Formatting for humans
  auto settings = shouldIndent ? true : false;

  // .json means to console (can we pass this in)
  std::string base = path.getBaseName();

  Strings::toLower(base);
  console = base == ".json";

  if (console) {
    boost::property_tree::write_json(std::cout, n, settings);
  } else {
    if (path.isLocal()) {
      boost::property_tree::write_json(path.toString(), n, std::locale(), settings);
    } else {
      LogSevere("Can't write to a remote URL at " << path << "\n");
      return false;
    }
  }
  return true;
}

bool
IOJSON::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>             & keys
)
{
  // Get settings
  const bool indent = (keys["indent"] == "true");

  LogInfo("JSON settings: indent: " << indent << "\n");
  std::string filename = keys["filename"];

  if (keys["directfile"] == "false") {
    filename         = filename + ".xml";
    keys["filename"] = filename;
  }

  bool successful = false;

  try{
    std::shared_ptr<PTreeData> json = std::dynamic_pointer_cast<PTreeData>(dt);
    if (json != nullptr) {
      writeURL(filename, json, true, false);
      successful = true;
    }
  }catch (const std::exception& e) {
    LogSevere("JSON create error: "
      << filename << " " << e.what() << "\n");
  }
  return successful;
}
