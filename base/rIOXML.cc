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
      << aURL.path << " " << e.what() << "\n");
  }
  return successful;
}
