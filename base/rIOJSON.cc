#include "rIOJSON.h"
#include "rJSONData.h"

#include "rFactory.h"
#include "rStrings.h"
#include "rError.h"
#include "rIOURL.h"
#include "rOS.h"

using namespace rapio;

/** Read call */
std::shared_ptr<DataType>
IOJSON::createDataType(const URL& url)
{
  std::shared_ptr<DataType> datatype = nullptr;
  std::vector<char> buf;

  if (IOURL::read(url, buf) > 0) {
    std::shared_ptr<JSONData> json = std::make_shared<JSONData>();
    if (json->readBuffer(buf)) {
      return json;
    }
  }
  LogSevere("Unable to create JSON from " << url << "\n");
  return nullptr;
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

bool
IOJSON::encodeDataType(std::shared_ptr<DataType> dt,
  const URL                                      & aURL,
  std::shared_ptr<DataFormatSetting>             dfs)
{
  bool successful = false;

  try{
    std::shared_ptr<JSONData> json = std::dynamic_pointer_cast<JSONData>(dt);
    // FIXME: shouldIndent probably added to dfs
    if (json != nullptr) {
      writeURL(aURL, json, true, false);
      successful = true;
    }
  }catch (const std::exception& e) {
    LogSevere("JSON create error: "
      << aURL.getPath() << " " << e.what() << "\n");
  }
  return successful;
}

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
