#include "rIOXML.h"

#include "rFactory.h"
#include "rStrings.h"
#include "rError.h"
#include "rIOURL.h"
#include "rOS.h"

using namespace rapio;

/** Read call */
std::shared_ptr<DataType>
IOXML::createDataType(const std::string& params)
{
  // We only read from file/url
  const URL url(params);

  std::shared_ptr<DataType> datatype = nullptr;
  std::vector<char> buf;

  if (IOURL::read(url, buf) > 0) {
    std::shared_ptr<XMLData> xml = std::make_shared<XMLData>();
    if (xml->readBuffer(buf)) {
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

bool
IOXML::writeURL(
  const URL                & path,
  std::shared_ptr<XMLData> tree,
  bool                     shouldIndent,
  bool                     console)
{
  // Delegate to XML since it knows the internals
  return (tree->writeURL(path, shouldIndent, console));
}

bool
IOXML::encodeDataType(std::shared_ptr<DataType> dt,
  const std::string                             & params,
  std::shared_ptr<XMLNode>                      dfs,
  bool                                          directFile,
  // Output for notifiers
  std::vector<Record>                           & records,
  std::vector<std::string>                      & files
)
{
  bool successful = false;

  bool useSubDirs = true; // Use subdirs
  URL aURL        = IODataType::generateFileName(dt, params, "xml", directFile, useSubDirs);

  try{
    std::shared_ptr<XMLData> xml = std::dynamic_pointer_cast<XMLData>(dt);
    // FIXME: shouldIndent probably added to dfs
    if (xml != nullptr) {
      writeURL(aURL, xml, true, false);
      successful = true;
      IODataType::generateRecord(dt, aURL, "xml", records, files);
    }
  }catch (std::exception& e) {
    LogSevere("XML create error: "
      << aURL.getPath() << " " << e.what() << "\n");
  }
  return successful;
}

bool
IOXML::splitSignedXMLFile(const std::string& signedxml,
  std::string                              & outmsg,
  std::string                              & outkey)
{
  // Get full .sxml file
  std::ifstream xmlfile;
  xmlfile.open(signedxml.c_str());
  std::ostringstream str;
  str << xmlfile.rdbuf();
  std::string msg = str.str();
  xmlfile.close();

  // The last three lines should be of form:
  // <signed>
  // key
  // </signed>
  // where key may not be xml compliant.  We remove it before
  // passing to xml parsing.
  std::size_t found = msg.rfind("<signed>");
  if (found == std::string::npos) {
    LogSevere("Missing signed tag end of file\n");
    return false;
  }

  // Split on the msg/signature
  std::string onlymsg    = msg.substr(0, found);
  std::string keywrapped = msg.substr(found);

  // Remove the signed tags from the key
  found = keywrapped.find("<signed>\n");
  if (found == std::string::npos) {
    LogSevere("Missing start <signed> tag end of file\n");
    return false;
  }
  keywrapped.replace(found, 9, "");

  found = keywrapped.find("</signed>\n");
  if (found == std::string::npos) {
    LogSevere("Missing end </signed> tag end of file\n");
    return false;
  }
  keywrapped.replace(found, 10, "");

  outmsg = onlymsg;
  outkey = keywrapped;
  return true;
} // IOXML::splitSignedXMLFile
