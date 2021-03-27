#include "rIOXML.h"

#include "rFactory.h"
#include "rStrings.h"
#include "rError.h"
#include "rIOURL.h"
#include "rDataGrid.h"
#include "rOS.h"

#include <boost/property_tree/xml_parser.hpp>

using namespace rapio;

std::string
IOXML::getHelpString(const std::string & key)
{
  std::string help;
  if (key == "W2ALGS") {
    help = "alias of XML builder for MRMS algorithms and records.\n";
  } else {
    help = "builder for reading xml formatted data.\n";
  }
  return help;
}

std::shared_ptr<DataType>
IOXML::createDataTypeFromBuffer(std::vector<char>& buffer)
{
  return (readPTreeDataBuffer(buffer));
}

size_t
IOXML::encodeDataTypeBuffer(std::shared_ptr<DataType> dt, std::vector<char>& buffer)
{
  std::shared_ptr<PTreeData> ptree = std::dynamic_pointer_cast<PTreeData>(dt);
  if (ptree) {
    return (writePTreeDataBuffer(ptree, buffer));
  }
  return 0;
}

std::shared_ptr<PTreeData>
IOXML::readPTreeDataBuffer(std::vector<char>& buffer)
{
  try{
    std::shared_ptr<PTreeData> d = std::make_shared<PTreeData>();
    auto& n = d->getTree()->node;
    // To keep sizes correct, only add a 0 iff there isn't one there already
    if ((buffer.size() > 0) && (buffer[buffer.size() - 1] != 0)) {
      buffer.push_back('\0');
    }

    std::istringstream is(&buffer.front());
    // Need to trim white space, or when we write we gets tons of blank lines.
    // Bug in boost or rapidxml I think.
    boost::property_tree::read_xml(is, n, boost::property_tree::xml_parser::trim_whitespace);
    return d;
  }catch (const std::exception& e) { // pt::xml_parser::xml_parser_error
    // We catch all to recover
    LogSevere("Exception reading XML data..." << e.what() << " ignoring\n");
  }
  return nullptr;
}

/** Read call */
std::shared_ptr<DataType>
IOXML::createDataType(const std::string& params)
{
  // We only read from file/url
  const URL url(params);

  std::shared_ptr<DataType> datatype = nullptr;
  std::vector<char> buf;

  if (IOURL::read(url, buf) > 0) {
    std::shared_ptr<PTreeData> xml = readPTreeDataBuffer(buf);
    if (xml) {
      // Most likely we'll introduce subtypes like in the other readers
      // but how to do this generically..
      // TEMP hack for WDSS2 DataTable class.  Look for <datatable>
      // and the <stref> tags for time/location and set in the datatype
      auto datatable = xml->getTree()->getChildOptional("datatable");
      if (datatable != nullptr) {
        auto datatype = datatable->getChildOptional("datatype");
        if (datatype != nullptr) {
          try{
            // The type (for folder writing)
            const auto theDataType = datatype->getAttr("name", std::string(""));
            xml->setTypeName(theDataType);

            auto time = datatype->getChild("stref.time");
            // Units, bleh.  Assume for now to epoch.  FIXME: Use the units convert if needed
            // const auto units = time.getAttr("units", std::string(""));
            const auto value = time.getAttr("value", (time_t) (0));
            xml->setTime(Time::SecondsSinceEpoch(value));

            // FIXME: Need location too...
            // LogSevere("GOT: " << value << "\n");
          }catch (const std::exception& e) {
            LogSevere("Tried to read stref tag in datatable and failed: " << e.what() << "\n");
          }
        }
      }

      return xml;
    }
  }
  LogSevere("Unable to create XML from " << url << "\n");
  return nullptr;
} // IOXML::createDataType

size_t
IOXML::writePTreeDataBuffer(std::shared_ptr<PTreeData> d, std::vector<char>& buf)
{
  // We know that PTree hides a boost node...
  const auto& n = d->getTree()->node;

  std::stringstream ss;
  boost::property_tree::xml_parser::write_xml(ss, n);
  std::string out = ss.str();
  buf = std::vector<char>(out.begin(), out.end());
  // To keep sizes correct, only add a 0 iff there isn't one there already
  if ((buf.size() > 0) && (buf[buf.size() - 1] != 0)) {
    buf.push_back('\0');
  }
  return buf.size();
}

bool
IOXML::writeURL(
  const URL                  & path,
  std::shared_ptr<PTreeData> tree,
  bool                       shouldIndent,
  bool                       console)
{
  // We know that PTree hides a boost node...
  const auto& n = tree->getTree()->node;

  // Formatting for humans
  auto settings = boost::property_tree::xml_writer_make_settings<std::string>(' ', shouldIndent ? 1 : 0);

  // .xml means to console (can we pass this in)
  std::string base = path.getBaseName();
  Strings::toLower(base);
  console = base == ".xml";

  if (console) {
    boost::property_tree::write_xml(std::cout, n, settings);
  } else {
    if (path.isLocal()) {
      boost::property_tree::write_xml(path.toString(), n, std::locale(), settings);
    } else {
      LogSevere("Can't write to a remote URL at " << path << "\n");
      return false;
    }
  }
  return true;
}

bool
IOXML::encodeDataType(std::shared_ptr<DataType> dt,
  const std::string                             & params,
  std::shared_ptr<PTreeNode>                    dfs,
  bool                                          directFile,
  // Output for notifiers
  std::vector<Record>                           & records
)
{
  // Get settings
  bool indent     = true;
  bool useSubDirs = true; // Use subdirs

  if (dfs != nullptr) {
    try{
      auto output = dfs->getChild("output");
      indent = output.getAttr("indent", indent);
    }catch (const std::exception& e) {
      LogSevere("Unrecognized settings, using defaults\n");
    }
  }
  LogInfo("XML settings: indent: " << indent << " useSubDirs: " << useSubDirs << "\n");

  URL aURL = IODataType::generateFileName(dt, params, "xml", directFile, useSubDirs);

  bool successful = false;
  try{
    std::shared_ptr<PTreeData> xml = std::dynamic_pointer_cast<PTreeData>(dt);
    if (xml != nullptr) {
      writeURL(aURL, xml, true, false);
      successful = true;
      IODataType::generateRecord(dt, aURL, "xml", records);
    }
  }catch (std::exception& e) {
    LogSevere("XML create error: "
      << aURL.getPath() << " " << e.what() << "\n");
  }
  return successful;
}
