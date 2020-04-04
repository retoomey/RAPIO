#include "rIOJSON.h"
#include "rJSONData.h"

#include "rFactory.h"
#include "rStrings.h"
#include "rError.h"
#include "rIOURL.h"
#include "rOS.h"

using namespace rapio;

void
IOJSON::introduceSelf()
{
  std::shared_ptr<IOJSON> newOne = std::make_shared<IOJSON>();
  Factory<IODataType>::introduce("json", newOne);

  // Add the default classes we handle...
}

/** Read call */
std::shared_ptr<DataType>
IOJSON::readJSONDataType(const std::vector<std::string>& args)
{
  std::shared_ptr<DataType> datatype = nullptr;

  // Note, in RAPIO we can read a json file remotely too
  const URL url = getFileName(args);

  auto out = readURL(url);
  if (out != nullptr) {
    return out;
  } else {
    LogSevere("Unable to create JSON from " << url << "\n");
  }
  return nullptr;
}

/** Read call */
std::shared_ptr<DataType>
IOJSON::createObject(const std::vector<std::string>& args)
{
  return (IOJSON::readJSONDataType(args));
}

bool
IOJSON::writeURL(
  const URL                 & path,
  std::shared_ptr<JSONData> tree,
  bool                      shouldIndent,
  bool                      console)
{
  // Delegate to JSON since it knows the internals
  return (tree->writeURL(path, shouldIndent, console));
}

std::shared_ptr<JSONData>
IOJSON::readURL(const URL& url)
{
  std::vector<char> buf;

  if (IOURL::read(url, buf) > 0) {
    std::shared_ptr<JSONData> json = std::make_shared<JSONData>();
    if (json->readBuffer(buf)) {
      return json;
    }
  }
  return nullptr;
}

std::string
IOJSON::encode(std::shared_ptr<DataType> dt,
  const std::string                      & directory,
  std::shared_ptr<DataFormatSetting>     dfs,
  std::vector<Record>                    & records)
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
  URL aURL = generateOutputInfo(*dt, directory, dfs, "json", params, selections);

  // Ensure full path to output file exists
  const std::string dir(aURL.getDirName());
  if (!OS::isDirectory(dir) && !OS::mkdirp(dir)) {
    LogSevere("Unable to create " << dir << "\n");
    return ("");
  }
  // end duplication ----

  bool successful = false;
  try{
    std::shared_ptr<JSONData> json = std::dynamic_pointer_cast<JSONData>(dt);
    // FIXME: shouldIndent probably added to dfs
    if (json != nullptr) {
      writeURL(aURL, json, true, false);
    }
  }catch (std::exception& e) {
    LogSevere("JSON create error: "
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
} // IOJSON::encode

void
IOJSON::setAttributes(std::shared_ptr<JSONData> json, std::shared_ptr<DataAttributeList> attribs)
{
  auto tree = json->getTree();

  for (auto& i:*attribs) {
    auto name = i.getName().c_str();
    std::ostringstream out;
    std::string value = "UNKNOWN";
    if (i.is<std::string>()) {
      auto field = *(i.get<std::string>());
      out << field;
    } else if (i.is<long>()) {
      auto field = *(i.get<long>());
      out << field;
    } else if (i.is<float>()) {
      auto field = *(i.get<float>());
      out << field;
    } else if (i.is<double>()) {
      auto field = *(i.get<double>());
      out << field;
    }
    tree->put(name, out.str());
  }
}

std::shared_ptr<JSONData>
IOJSON::createJSON(std::shared_ptr<DataGrid> datagrid)
{
  // Create a JSON tree from datagrid.  Passed to python
  // for the python experiment

  // --------------------------------------------------------
  // DataGrid to JSON
  // FIXME: How about general DataType?
  //
  std::shared_ptr<JSONData> theJson = std::make_shared<JSONData>();
  auto tree = theJson->getTree();

  // Store the data type
  tree->put("DataType", datagrid->getDataType());

  // General Attributes to JSON
  IOJSON::setAttributes(theJson, datagrid->getGlobalAttributes());

  // -----------------------------------
  // Dimensions (only for DataGrids for moment)
  auto theDims = datagrid->getDims();
  // auto dimArrays = theJson->getNode();
  JSONNode dimArrays;
  for (auto& d:theDims) {
    JSONNode aDimArray;
    // Order matters here...
    // auto aDimArray = theJson->getNode();

    aDimArray.put("name", d.name());
    aDimArray.put("size", d.size());
    dimArrays.addArrayNode(aDimArray);
  }
  tree->addNode("Dimensions", dimArrays);

  // -----------------------------------
  // Arrays
  auto arrays = datagrid->getArrays();
  JSONNode theArrays;
  auto pid  = OS::getProcessID();
  int count = 1;
  for (auto& ar:arrays) {
    // Individual array
    JSONNode anArray;

    auto name    = ar->getName();
    auto indexes = ar->getDimIndexes();
    anArray.put("name", name);

    // Create a unique array key for shared memory
    // FIXME: Create shared_memory unique name
    anArray.put("shm", "/dev/shm/" + std::to_string(pid) + "-array" + std::to_string(count));
    count++;

    // Dimension Index Arrays
    JSONNode aDimArrays;
    for (auto& index:indexes) {
      JSONNode aDimArray;
      aDimArray.put("", index);
      aDimArrays.addArrayNode(aDimArray);
    }
    anArray.addNode("Dimensions", aDimArrays);
    theArrays.addArrayNode(anArray);
  }
  tree->addNode("Arrays", theArrays);

  // End arrays
  // -----------------------------------
  return theJson;
} // IOJSON::createJSON
