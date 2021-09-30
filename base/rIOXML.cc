#include "rIOXML.h"

#include "rFactory.h"
#include "rStrings.h"
#include "rError.h"
#include "rIOURL.h"
#include "rOS.h"

// Default built in DataType support
#include "rDataTable.h"

#include <boost/property_tree/xml_parser.hpp>

using namespace rapio;

std::string
IOXML::getHelpString(const std::string & key)
{
  std::string help;
  if (key == "W2ALGS") {
    help = "alias of XML builder for MRMS algorithms and records.\n";
  } else {
    help = "builder for reading XML formatted data.\n";
  }
  return help;
}

void
IOXML::initialize()
{
  // PTreeDataTable::introduceSelf(this);
  std::shared_ptr<IOSpecializer> d = std::make_shared<PTreeDataTable>();
  this->introduce("datatable", d);
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
  } else {
    LogSevere("NOT A PTREE, FAILED TO WRITE\n");
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

  std::vector<char> buf;

  if (IOURL::read(url, buf) > 0) {
    std::shared_ptr<PTreeData> xml = readPTreeDataBuffer(buf);
    if (xml) {
      // Specialize by passing original PTreeData datatype to the specializer
      // that will create the datatype such a DataTable or ColorMap, etc.
      try{
        auto firsttag = xml->getTree()->getFirstChildName();
        auto fmt      = getIOSpecializer(firsttag);
        if (fmt != nullptr) {
          std::map<std::string, std::string> keys;
          keys["XML_URL"] = url.toString(); // Maybe some classes find useful?
          return (fmt->read(keys, xml));
        }
      }catch (const std::exception& e)
      {
        LogSevere("Failure specializing PTreeData at " << url << ".  Class is generalized to PTreeData.\n");
      }

      return xml; // unspecialized but usuable
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
  std::map<std::string, std::string>     & keys
)
{
  // Get settings
  const bool indent = (keys["indent"] == "true");

  LogInfo("XML settings: indent: " << indent << "\n");
  std::string filename = keys["filename"];
  if (keys["directfile"] == "false") {
    filename         = filename + ".xml";
    keys["filename"] = filename;
  }

  bool successful = false;
  try{
    std::shared_ptr<PTreeData> xml = std::dynamic_pointer_cast<PTreeData>(dt);
    if (xml != nullptr) {
      writeURL(filename, xml, true, false);
      successful = true;
    }
  }catch (std::exception& e) {
    LogSevere("XML create error: "
      << filename << " " << e.what() << "\n");
  }
  return successful;
}
