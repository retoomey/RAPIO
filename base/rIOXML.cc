#include "rIOXML.h"

#include "rFactory.h"
#include "rStrings.h"
#include "rError.h"
#include "rIOURL.h"
#include "rOS.h"

using namespace rapio;

/** Read call */
std::shared_ptr<DataType>
IOXML::createDataType(const URL& url)
{
  std::shared_ptr<DataType> datatype = nullptr;
  std::vector<char> buf;

  if (IOURL::read(url, buf) > 0) {
    std::shared_ptr<XMLData> xml = std::make_shared<XMLData>();
    if (xml->readBuffer(buf)) {
      return xml;
    }
  }
  LogSevere("Unable to create XML from " << url << "\n");
  return nullptr;
}

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
  const URL                                     & aURL,
  std::shared_ptr<DataFormatSetting>            dfs)
{
  bool successful = false;

  try{
    std::shared_ptr<XMLData> xml = std::dynamic_pointer_cast<XMLData>(dt);
    // FIXME: shouldIndent probably added to dfs
    if (xml != nullptr) {
      writeURL(aURL, xml, true, false);
      successful = true;
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
