#include "rDataGrid.h"
#include "rTime.h"
#include "rLLH.h"
#include "rOS.h"
#include "rStrings.h"

#include "rError.h"

using namespace rapio;
using namespace std;

DataGrid::DataGrid()
{
  setDataType("DataGrid");
  // Current the default write for all grids is netcdf which makes sense
  setReadFactory("netcdf");
}

std::shared_ptr<DataGrid>
DataGrid::Create(const std::string& aTypeName,
  const std::string               & Units,
  const LLH                       & location,
  const Time                      & datatime,
  const std::vector<size_t>       & dimsizes,
  const std::vector<std::string>  & dimnames)
{
  auto newonesp = std::make_shared<DataGrid>();

  newonesp->setDataType("DataGrid");
  newonesp->setReadFactory("netcdf");

  newonesp->init(aTypeName, Units, location, datatime, dimsizes, dimnames);
  return newonesp;
}

void
DataGrid::deep_copy(const std::shared_ptr<DataGrid>& nsp) const
{
  DataType::deep_copy(nsp);

  // Copy our stuff
  auto & n = *nsp;

  n.myDims = myDims;
  for (auto sp: myNodes) {
    n.myNodes.push_back(sp->Clone()); // Deep copy each array
  }
}

std::shared_ptr<DataGrid>
DataGrid::Clone() const
{
  auto nsp = std::make_shared<DataGrid>();

  DataGrid::deep_copy(nsp);
  return nsp;
}

bool
DataGrid::init(const std::string & aTypeName,
  const std::string              & Units,
  const LLH                      & location,
  const Time                     & datatime,
  const std::vector<size_t>      & dimsizes,
  const std::vector<std::string> & dimnames)
{
  //  setDataType("DataGrid");
  // Current the default write for all grids is netcdf which makes sense
  //  setReadFactory("netcdf");

  setTypeName(aTypeName);
  setDataAttributeValue("Unit", Units, "dimensionless"); // Maybe setUnits here?
  myLocation = location;
  myTime     = datatime;
  setDims(dimsizes, dimnames);
  return true;
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

const std::vector<size_t>
DataGrid::getSizes() const
{
  std::vector<size_t> sizes;

  for (auto& d:myDims) {
    sizes.push_back(d.size());
  }
  return sizes;
}

std::shared_ptr<DataArray>
DataGrid::getNode(const std::string& name)
{
  size_t count = 0;

  for (auto i:myNodes) {
    if (i->getName() == name) {
      return i;
    }
    count++;
  }
  return nullptr;
}

int
DataGrid::getNodeIndex(const std::string& name)
{
  int count = 0;

  for (auto i:myNodes) {
    if (i->getName() == name) {
      return count;
    }
    count++;
  }
  return -1;
}

void
DataGrid::setDims(const std::vector<size_t>& dimsizes,
  const std::vector<std::string>           & dimnames)
{
  // Store updated dimension info
  myDims.clear();
  for (size_t zz = 0; zz < dimsizes.size(); ++zz) {
    myDims.push_back(DataGridDimension(dimnames[zz], dimsizes[zz]));
  }

  resize(dimsizes);
} // DataGrid::setDims

void
DataGrid::resize(const std::vector<size_t>& dimsizes)
{
  if (myDims.size() != dimsizes.size()) {
    fLogSevere("Trying to resize {} dimensions but using {} values.", myDims.size(), dimsizes.size());
    return;
  }

  // Change the dimension size first.
  for (size_t zz = 0; zz < dimsizes.size(); ++zz) {
    myDims[zz].setSize(dimsizes[zz]);
  }

  // Resize each arrays to new size.
  for (auto l:myNodes) {
    auto i    = l->getDimIndexes();
    auto name = l->getName();
    // Unused here
    // auto type = l->getStorageType();
    auto size = i.size();

    // From each index into dimension, get actual sizes
    // i ==> 0, 1 or say 1, 0 if flipped
    // to --> 50, 100 size for example 0--> 50, 1 --> 100
    std::vector<size_t> sizes(size);
    for (size_t d = 0; d < size; ++d) {
      sizes[d] = dimsizes[i[d]];
    }
    auto array = l->getArray();
    array->resize(sizes);
  }
}

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
        case BYTE:
        case SHORT:
        case DOUBLE:
        default:
          fLogSevere("This type of data not supported, though should be easy to add.");
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

  DeclareArrayFactoryMethods(int8_t, BYTE)

  DeclareArrayFactoryMethods(short, SHORT)

  DeclareArrayFactoryMethods(int, INT)

  DeclareArrayFactoryMethods(float, FLOAT)

  DeclareArrayFactoryMethods(double, DOUBLE)

  return nullptr;
}

std::string
DataGrid::getUnits(const std::string& name)
{
  // Default to the global units
  std::string units = DataType::getUnits(name);

  // Update node if any
  auto n = getNode(name);

  if (n != nullptr) {
    n->getString(Constants::Units, units);
  }
  return units;
}

void
DataGrid::setUnits(const std::string& units, const std::string& name)
{
  // Only update the global units if this is primary data.
  if (name == Constants::PrimaryDataName) {
    DataType::setUnits(units, name);
  }

  // Get from node if any
  auto n = getNode(name);

  if (n != nullptr) {
    n->setString(Constants::Units, units);
  }
}
