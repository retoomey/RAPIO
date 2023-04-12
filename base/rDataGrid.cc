#include "rDataGrid.h"
#include "rTime.h"
#include "rLLH.h"
#include "rOS.h"

#include "rError.h"

using namespace rapio;
using namespace std;

DataGrid::DataGrid()
{
  setDataType("DataGrid");
}

DataGrid::DataGrid(const std::string& aTypeName,
  const std::string                 & Units,
  const LLH                         & center,
  const Time                        & datatime,
  const std::vector<size_t>         & dimsizes,
  const std::vector<std::string>    & dimnames)
{
  setDataType("DataGrid");
  setTypeName(aTypeName);
  setDataAttributeValue("Unit", "dimensionless", Units);
  myLocation = center;
  myTime     = datatime;
  declareDims(dimsizes, dimnames);
}

std::shared_ptr<DataGrid>
DataGrid::Create(const std::string& aTypeName,
  const std::string               & Units,
  const LLH                       & center,
  const Time                      & datatime,
  const std::vector<size_t>       & dimsizes,
  const std::vector<std::string>  & dimnames)
{
  return (std::make_shared<DataGrid>(aTypeName, Units, center, datatime, dimsizes, dimnames));
}

std::shared_ptr<DataAttributeList>
DataArray::getAttributes()
{
  return myAttributes;
}

std::shared_ptr<DataAttributeList>
DataGrid::getAttributes(
  const std::string& name)
{
  auto node = getNode(name);

  if (node != nullptr) {
    return node->getAttributes();
  }
  return nullptr;
}

std::shared_ptr<DataArray>
DataGrid::getDataArray(const std::string& name)
{
  return getNode(name);
}

void
DataGrid::declareDims(const std::vector<size_t>& dimsizes,
  const std::vector<std::string>               & dimnames)
{
  // Store updated dimension info
  myDims.clear();
  for (size_t zz = 0; zz < dimsizes.size(); ++zz) {
    myDims.push_back(DataGridDimension(dimnames[zz], dimsizes[zz]));
  }

  // Resize all arrays to given dimensions...this only gets
  // calls if someone is trying to 'expand' or reduce the size of
  // our stuff.  I've tested some but not 100% hammered it yet
  // I probably need to design a DataGrid test in tests for this
  // purpose.
  for (auto l:myNodes) {
    auto i    = l->getDimIndexes();
    auto name = l->getName();
    auto type = l->getStorageType();
    auto size = i.size();

    // From each index into dimension, get actual sizes
    // i ==> 0, 1 or say 1, 0 if flipped
    // to --> 50, 100 size for example 0--> 50, 1 --> 100
    std::vector<size_t> sizes(size);
    for (size_t d = 0; d < size; ++d) {
      sizes[d] = dimsizes[i[d]];
    }
    auto array = l->getNewArray();
    array->resize(sizes);
  }
} // DataGrid::declareDims

namespace {
void
setAttributes(std::shared_ptr<PTreeData> json, std::shared_ptr<DataAttributeList> attribs)
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
}

std::shared_ptr<PTreeData>
DataGrid::createMetadata()
{
  // Create a JSON tree from datagrid.  Passed to python
  // for the python experiment
  std::shared_ptr<PTreeData> theJson = std::make_shared<PTreeData>();
  auto tree = theJson->getTree();

  // Store the data type
  tree->put("DataType", getDataType());

  // General Attributes to JSON
  setAttributes(theJson, getGlobalAttributes());

  // -----------------------------------
  // Dimensions (only for DataGrids for moment)
  auto theDims = getDims();
  // auto dimArrays = theJson->getNode();
  PTreeNode dimArrays;

  for (auto& d:theDims) {
    PTreeNode aDimArray;
    // Order matters here...
    // auto aDimArray = theJson->getNode();

    aDimArray.put("name", d.name());
    aDimArray.put("size", d.size());
    dimArrays.addArrayNode(aDimArray);
  }
  tree->addNode("Dimensions", dimArrays);

  // -----------------------------------
  // Arrays
  auto arrays = getArrays();
  PTreeNode theArrays;
  auto pid  = OS::getProcessID();
  int count = 1;

  for (auto& ar:arrays) {
    // Individual array
    PTreeNode anArray;

    auto name = ar->getName();
    anArray.put("name", name);

    auto type = ar->getStorageType();
    std::string typeStr = "Unknown";
    switch (type) {
        case FLOAT:
          typeStr = "float32";
          break;
        case INT:
          typeStr = "int32";
          break;
    }
    anArray.put("type", typeStr);

    // Create a unique array key for shared memory
    // FIXME: Create shared_memory unique name
    anArray.put("shm", "/dev/shm/" + std::to_string(pid) + "-array" + std::to_string(count));
    count++;

    // Dimension Index Arrays
    PTreeNode aDimArrays;
    auto indexes = ar->getDimIndexes();
    for (auto& index:indexes) {
      PTreeNode aDimArray;
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
} // DataGrid::createMetadata

bool
DataGrid::initFromGlobalAttributes()
{
  bool success = true;

  DataType::initFromGlobalAttributes();
  return success;
}

void
DataGrid::updateGlobalAttributes(const std::string& encoded_type)
{
  // Note: Datatype updates the attributes -unit -value specials,
  // so don't add any after this
  DataType::updateGlobalAttributes(encoded_type);
}

// Define a case for creating a particular type/dimension.
#define DeclareArrayFactoryMethodsForD(TYPE, ARRAYTYPE, DIMENSION) \
  if ((dimCount == DIMENSION) && (type == ARRAYTYPE)) { \
    auto d = add<TYPE, DIMENSION>(name, units, ARRAYTYPE, dimindexes); \
    return d->getRawDataPointer(); \
  }

// Define dimensions we support
// Make sure to sync these calls with the DeclareArrayMethods in the .h
#define DeclareArrayFactoryMethods(TYPE, ARRAYTYPE) \
  DeclareArrayFactoryMethodsForD(TYPE, ARRAYTYPE, 1) \
  DeclareArrayFactoryMethodsForD(TYPE, ARRAYTYPE, 2) \
  DeclareArrayFactoryMethodsForD(TYPE, ARRAYTYPE, 3)

void *
DataGrid::factoryGetRawDataPointer(const std::string& name, const std::string& units, const DataArrayType& type,
  const std::vector<size_t>& dimindexes)
{
  // A typeless factory for creating/adding and returning a working pointer to array.
  // Note: This stay in scope only if the DataGrid does, so this is typically used for generic
  // array creation by readers
  // Don't think there's a way to pass dynamic parameters to the templates, so we have this
  // messy thing..unless BOOST lets you do it at lower level somewhere.
  //
  // Make sure these calls match up with the DeclareArrayMethods call in the header...

  const size_t dimCount = dimindexes.size();

  DeclareArrayFactoryMethods(char, BYTE)

  DeclareArrayFactoryMethods(short, SHORT)

  DeclareArrayFactoryMethods(int, INT)

  DeclareArrayFactoryMethods(float, FLOAT)

  DeclareArrayFactoryMethods(double, DOUBLE)

  return nullptr;
}
