#include "rIOXML.h"

#include "rFactory.h"
#include "rStrings.h"
#include "rError.h"
#include "rIOURL.h"
#include "rOS.h"

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

  auto out = readURL(url);
  if (out != nullptr) {
    return out;
  } else {
    LogSevere("Unable to create XML from " << url << "\n");
  }
  return nullptr;
}

/** Read call */
std::shared_ptr<DataType>
IOXML::createObject(const std::vector<std::string>& args)
{
  return (IOXML::readXMLDataType(args));
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

std::shared_ptr<XMLData>
IOXML::readURL(const URL& url)
{
  std::vector<char> buf;

  if (IOURL::read(url, buf) > 0) {
    std::shared_ptr<XMLData> xml = std::make_shared<XMLData>();
    if (xml->readBuffer(buf)) {
      return xml;
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
  // Specialization if any, for json we don't for now
  const std::string type = dt->getDataType();

  // FIXME: Overlaps with IONetcdf writing stuff, so we could
  // generalize more here

  // start duplication ----
  // Generate the filepath/notification info for this datatype.
  // Static encoded for now.  Could make it virtual in the formatter
  std::vector<std::string> selections;
  std::vector<std::string> params;
  URL aURL = generateOutputInfo(*dt, directory, dfs, "xml", params, selections);

  // Ensure full path to output file exists
  const std::string dir(aURL.getDirName());
  if (!OS::isDirectory(dir) && !OS::mkdirp(dir)) {
    LogSevere("Unable to create " << dir << "\n");
    return ("");
  }
  // end duplication ----

  bool successful = false;
  try{
    std::shared_ptr<XMLData> xml = std::dynamic_pointer_cast<XMLData>(dt);
    // FIXME: shouldIndent probably added to dfs
    if (xml != nullptr) {
      writeURL(aURL, xml, true, false);
    }
  }catch (std::exception& e) {
    LogSevere("XML create error: "
      << aURL.path << " " << e.what() << "\n");
    return ("");
  }

  // -------------------------------------------------------------
  // Update records to match our written stuff...
  if (successful) {
    const rapio::Time aTime = dt->getTime();
    Record rec(params, selections, aTime);
    records.push_back(rec);
    return (aURL.path);
  }
  return ("");
} // IOXML::encode
