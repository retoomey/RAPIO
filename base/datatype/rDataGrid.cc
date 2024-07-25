#include "rDataGrid.h"
#include "rTime.h"
#include "rLLH.h"
#include "rOS.h"
#include "rStrings.h"

#include "rError.h"

using namespace rapio;
using namespace std;

// Percentage of 'size' of sparse before we no longer use it.
// For example, if 0.75 than if the sparse size is guessed to be
// within 75% of the unsparse than we will use sparse.
double DataGrid::SparseThreshold = 0.75;

DataGrid::DataGrid()
{
  setDataType("DataGrid");
  // Current the default write for all grids is netcdf which makes sense
  setReadFactory("netcdf");
}

std::shared_ptr<DataGrid>
DataGrid::Create(const std::string& aTypeName,
  const std::string               & Units,
  const LLH                       & center,
  const Time                      & datatime,
  const std::vector<size_t>       & dimsizes,
  const std::vector<std::string>  & dimnames)
{
  auto newonesp = std::make_shared<DataGrid>();

  newonesp->setDataType("DataGrid");
  newonesp->setReadFactory("netcdf");

  newonesp->init(aTypeName, Units, center, datatime, dimsizes, dimnames);
  return newonesp;
}

void
DataGrid::deep_copy(std::shared_ptr<DataGrid> nsp)
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
DataGrid::Clone()
{
  auto nsp = std::make_shared<DataGrid>();

  DataGrid::deep_copy(nsp);
  return nsp;
}

bool
DataGrid::init(const std::string & aTypeName,
  const std::string              & Units,
  const LLH                      & center,
  const Time                     & datatime,
  const std::vector<size_t>      & dimsizes,
  const std::vector<std::string> & dimnames)
{
  //  setDataType("DataGrid");
  // Current the default write for all grids is netcdf which makes sense
  //  setReadFactory("netcdf");

  setTypeName(aTypeName);
  setDataAttributeValue("Unit", Units, "dimensionless"); // Maybe setUnits here?
  myLocation = center;
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

std::vector<size_t>
DataGrid::getSizes()
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
    auto array = l->getArray();
    array->resize(sizes);
  }
} // DataGrid::setDims

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
          LogSevere("This type of data not supported, though should be easy to add.\n");
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

void
DataGrid::unsparse2D(
  size_t                            num_x,
  size_t                            num_y,
  std::map<std::string, std::string>& keys,
  const std::string                 & pixelX,
  const std::string                 & pixelY,
  const std::string                 & pixelCount)
{
  // ------------------------------------------
  // Check if sparse array exist
  auto pixelXptr = getShort1D(pixelX);

  if (!pixelXptr) {
    // No sparse array so give up
    LogInfo("Data isn't sparse so we're done reading.\n");
    return;
  }

  try{
    auto& pixel_x = getShort1DRef(pixelX);
    auto& pixel_y = getShort1DRef(pixelY);
  }catch (...) {
    LogSevere("Excepted pixel_x and pixel_y arrays, can't find to unsparse.\n");
    return;
  }
  auto& pixel_x = getShort1DRef(pixelX);
  auto& pixel_y = getShort1DRef(pixelY);

  // We have to rename the 'pixel' primary array, since it uses the same name as the
  // primary full 3D array. FIXME: Or maybe just pull it to shared_ptr
  const std::string Units = getUnits();

  changeArrayName(Constants::PrimaryDataName, "SparseData");
  addFloat2D(Constants::PrimaryDataName, Units, { 0, 1 });

  auto& data_val = getFloat1DRef("SparseData");

  auto n = getDataArray("SparseData"); // get actual DataArray class

  // Gather sizes
  const size_t max_size   = num_x * num_y;
  const size_t num_pixels = pixel_x.size();

  // At most would expect num_pixel max_size, each with a runlength of 1
  if (num_pixels > size_t(max_size)) {
    LogSevere("Corrupt?: Number of unique start pixels more than grid size, which seems strange\n");
    LogSevere("Corrupt?: num_pixels is " << num_pixels << " while max_size is " << max_size << " \n");
    return;
  }

  LogInfo("2D Sparse Dimensions: " << num_pixels << " for (" << num_x << " * " << num_y << ")\n");

  auto dataptr = getFloat2D();
  auto& data   = dataptr->ref();

  // Get the BackgroundValue
  float backgroundValue = Constants::MissingData; // MRMS using this

  n->getFloat("BackgroundValue", backgroundValue);
  dataptr->fill(backgroundValue);

  // Lak's allows missing default of 1 pixel but never seen it
  auto& counts = getInt1DRef(pixelCount);

  for (size_t i = 0; i < num_pixels; ++i) {
    short x = pixel_x[i];
    short y = pixel_y[i];
    float v = data_val[i];
    int c   = counts[i]; // check negative count?

    // FIXME: Not sure this is changed anywhere in MRMS. But Lak
    // obviously intended to have this ability.
    // Replace any missing/range using the file version to our internal.
    // This is so much faster here than looping with 'replace' functions
    // later on.
    //    if (v == fileMissing) {
    //      v = Constants::MissingData;
    ///    } else if (v == fileRangeFolded) {
    //     v = Constants::RangeFolded;
    //   }
    for (int j = 0; j < c; ++j) {
      data[x][y] = v;

      // 'Roll'columns and rows
      ++y;

      if (y == num_y) {
        y = 0;
        ++x;
      }
      // Branchless here is slower for radial sets.
      // const bool border = (y == num_y);
      // y = y - (border *y);
      // x = x + border;
    }
  }

  // "SparseRadialSet" --> "RadialSet"
  std::string datatype = getDataType();

  Strings::removePrefix(datatype, "Sparse");
  setDataType(datatype);

  // Remove the sparse array elements
  deleteArrayName("SparseData");
  deleteArrayName(pixelX);
  deleteArrayName(pixelY);
  deleteArrayName(pixelCount);
  // For now..pop dimension since we know x,y,pixels.
  // FIXME: Won't work generically for other sparse arrays.
  // We would need to hunt/handle dimension movement
  myDims.pop_back();
} // DataGrid::unsparse2D

void
DataGrid::unsparse3D(
  size_t                            num_x,
  size_t                            num_y,
  size_t                            num_z,
  std::map<std::string, std::string>& keys,
  const std::string                 & pixelX,
  const std::string                 & pixelY,
  const std::string                 & pixelZ,
  const std::string                 & pixelCount)
{
  // ------------------------------------------
  // Check if sparse array exist
  auto pixelXptr = getShort1D(pixelX);

  if (!pixelXptr) {
    // No sparse array so give up
    LogInfo("Data isn't sparse so we're done reading.\n");
    return;
  }

  try{
    auto& pixel_x = getShort1DRef(pixelX);
    auto& pixel_y = getShort1DRef(pixelY);
    // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    auto& pixel_z = getShort1DRef(pixelZ);
    // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  }catch (...) {
    LogSevere("Excepted pixel_x, pixel_y, pixel_z arrays, can't find to unsparse.\n");
    return;
  }

  auto& pixel_x = getShort1DRef(pixelX);
  auto& pixel_y = getShort1DRef(pixelY);
  auto& pixel_z = getShort1DRef(pixelZ);

  // We have to rename the 'pixel' primary array, since it uses the same name as the
  // primary full 3D array.
  // FIXME: If we could lazy add an array..that would be good.  We could keep the state
  // then without destroying in the error case.
  const std::string Units = getUnits();

  changeArrayName(Constants::PrimaryDataName, "SparseData");
  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  addFloat3D(Constants::PrimaryDataName, Units, { 0, 1, 2 });
  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  auto& data_val = getFloat1DRef("SparseData");

  auto n = getDataArray("SparseData"); // get actual DataArray class

  // Gather sizes
  const size_t max_size   = num_x * num_y * num_z;
  const size_t num_pixels = pixel_x.size();

  // At most would expect num_pixel max_size, each with a runlength of 1
  if (num_pixels > size_t(max_size)) {
    LogSevere("Corrupt?: Number of unique start pixels more than grid size, which seems strange\n");
    LogSevere("Corrupt?: num_pixels is " << num_pixels << " while max_size is " << max_size << " \n");
    return;
  }

  LogInfo("3D Sparse Dimensions: " << num_pixels << " for (" << num_x << " * " << num_y << " * " << num_z << ")\n");

  auto dataptr = getFloat3D();
  auto& data   = dataptr->ref();

  // Get the BackgroundValue
  float backgroundValue = Constants::MissingData; // MRMS using this

  n->getFloat("BackgroundValue", backgroundValue);
  dataptr->fill(backgroundValue);

  // Lak's allows missing default of 1 pixel but never seen it
  auto& counts = getInt1DRef("pixel_count");

  // Wonder if we could 'scan' the pixels and check bounds 'first' and avoid it during the set
  // loop.  And/or we can use a flag to turn it off for speed
  for (size_t i = 0; i < num_pixels; ++i) {
    short x = pixel_x[i];
    short y = pixel_y[i];
    short z = pixel_z[i];
    float v = data_val[i];
    int c   = counts[i]; // check negative count?

    for (int j = 0; j < c; ++j) {
      data[z][x][y] = v;

      // 'Roll'columns and rows
      ++y;
      if (y == num_y) {
        y = 0;
        ++x;
      }
      if (x == num_x) {
        x = 0;
        ++z;
      }
    }
  }

  // Remove the sparse data arrays and extra dimension and ensure proper DataType
  // "SparseRadialSet" --> "RadialSet"
  std::string datatype = getDataType();

  Strings::removePrefix(datatype, "Sparse");
  setDataType(datatype);

  deleteArrayName("SparseData");
  deleteArrayName(pixelZ);
  deleteArrayName(pixelX);
  deleteArrayName(pixelY);
  deleteArrayName(pixelCount);
  myDims.pop_back();
} // DataGrid::unsparse3D

bool
DataGrid::sparse3D()
{
  // Check if sparse already...
  auto pixelptr = getShort1D("pixel_x");

  if (pixelptr != nullptr) {
    LogInfo("Not making sparse since pixels already exists...\n");
    return false;
  }

  // Have to have the 3D array to turn to sparse.  If this is just
  // loaded as sparse it may not have this
  auto dataptr = getFloat3D(Constants::PrimaryDataName);

  if (dataptr == nullptr) {
    return false;
  }

  // ----------------------------------------------------------------------------
  // Calculate size of pixels required to store the data.
  // We 'could' also just do it and let vectors grow "might" be faster
  auto sizes = getSizes(); // FIXME: Maybe we pass in
  auto& data = getFloat3DRef(Constants::PrimaryDataName);
  float backgroundValue = Constants::MissingData; // default and why not unvailable?
  size_t neededPixels   = 0;
  float lastValue       = backgroundValue;

  for (size_t z = 0; z < sizes[0]; z++) {
    for (size_t x = 0; x < sizes[1]; x++) {
      for (size_t y = 0; y < sizes[2]; y++) {
        float& v = data[z][x][y];
        if ((v != backgroundValue) && (v != lastValue) ) {
          ++neededPixels;
        }
        lastValue = v;
      }
    }
  }

  // 3 shorts + 1 float are thrice the size of just a float
  float compr_ratio = float(4 * neededPixels) / (sizes[0] * sizes[1] * sizes[2]);

  LogInfo("---> 3D compression of: " << int(0.5 + 100 * compr_ratio) << "% of original.\n");

  // ----------------------------------------------------------------------------
  // Modify ourselves to be parse.  But we need to keep our actual data (probably)
  // Add a 'pixel' dimension...we can add (ONCE) without messing with current arrays
  // since it's the last dimension. FIXME: more api probably
  myDims.push_back(DataGridDimension("pixel", neededPixels));

  // Move the primary out of the way and mark it hidden to writers...
  // we don't want to delete because the caller may be using the array and has no clue
  // about the sparse stuff
  std::string dataunits = getUnits(); // Get units first since it comes from primary

  changeArrayName(Constants::PrimaryDataName, "DisabledPrimary");
  setVisible("DisabledPrimary", false); // turn off writing

  // New primary array is a sparse one.
  auto& pixels = addFloat1DRef(Constants::PrimaryDataName, dataunits, { 3 });
  const std::string Units = "dimensionless";
  auto& pixel_z = addShort1DRef("pixel_z", Units, { 3 });
  auto& pixel_y = addShort1DRef("pixel_y", Units, { 3 });
  auto& pixel_x = addShort1DRef("pixel_x", Units, { 3 });
  auto& counts  = addInt1DRef("pixel_count", Units, { 3 });

  // Now fill the pixel array...
  lastValue = backgroundValue;
  size_t at = 0;

  for (size_t z = 0; z < sizes[0]; z++) {
    for (size_t x = 0; x < sizes[1]; x++) {
      for (size_t y = 0; y < sizes[2]; y++) {
        float& v = data[z][x][y];
        // Ok we have a value not background...
        if (v != backgroundValue) {
          // If it's part of the current RLE
          if (v == lastValue) {
            // Add to the previous count...
            ++counts[at - 1];
          } else {
            // ...otherwise start a new run
            pixel_z[at] = z;
            pixel_x[at] = x;
            pixel_y[at] = y;
            pixels[at]  = v;
            counts[at]  = 1;
            ++at; // move forward
          }
        }
        lastValue = v;
      }
    }
  }
  // Extra Meta for array
  auto pixelsDataArray = getNode(Constants::PrimaryDataName);

  if (pixelsDataArray) {
    pixelsDataArray->setLong("missing_value", Constants::MissingData);
    pixelsDataArray->setLong("BackgroundValue", backgroundValue);
    pixelsDataArray->setFloat("SparseGridCompression", compr_ratio);
    pixelsDataArray->setLong("NumValidRuns", pixels.size());
  }

  // Change DataType to have "Sparse" in front of it...
  std::string datatype = getDataType();

  setDataType("Sparse" + datatype);

  return true;
} // DataGrid::sparse3D

bool
DataGrid::sparse2D()
{
  // Check if sparse already...
  auto pixelptr = getShort1D("pixel_x");

  if (pixelptr != nullptr) {
    LogInfo("Not making sparse since pixels already exists...\n");
    return false;
  }

  // Have to have the 2D array to turn to sparse.  If this is just
  // loaded as sparse it may not have this
  auto dataptr = getFloat2D(Constants::PrimaryDataName);

  if (dataptr == nullptr) {
    return false;
  }
  // ----------------------------------------------------------------------------
  // Calculate size of pixels required to store the data.
  // We 'could' also just do it and let vectors grow "might" be faster
  auto sizes = getSizes(); // FIXME: Maybe we pass in
  auto& data = getFloat2DRef(Constants::PrimaryDataName);
  float backgroundValue = Constants::MissingData; // default and why not unvailable?
  size_t neededPixels   = 0;
  float lastValue       = backgroundValue;

  for (size_t x = 0; x < sizes[0]; x++) {
    for (size_t y = 0; y < sizes[1]; y++) {
      float& v = data[x][y];
      if ((v != backgroundValue) && (v != lastValue) ) {
        ++neededPixels;
      }
      lastValue = v;
    }
  }

  // Lak's stuff to determine if we should optionally sparse or not?
  // 2 shorts + 1 float are thrice the size of just a float
  float compr_ratio = float(3 * neededPixels) / (sizes[0] * sizes[1]);

  LogInfo("---> 2D compression of: " << int(0.5 + 100 * compr_ratio) << "% of original.\n");

  // ----------------------------------------------------------------------------
  // Modify ourselves to be parse.  But we need to keep our actual data (probably)
  // Add a 'pixel' dimension...we can add (ONCE) without messing with current arrays
  // since it's the last dimension. FIXME: more api probably
  myDims.push_back(DataGridDimension("pixel", neededPixels));

  // Move the primary out of the way and mark it hidden to writers...
  // we don't want to delete because the caller may be using the array and has no clue
  // about the sparse stuff
  std::string dataunits = getUnits(); // Get units first since it comes from primary

  changeArrayName(Constants::PrimaryDataName, "DisabledPrimary");
  setVisible("DisabledPrimary", false); // turn off writing

  // New primary array is a sparse one.
  auto& pixels = addFloat1DRef(Constants::PrimaryDataName, dataunits, { 2 });
  const std::string Units = "dimensionless";
  auto& pixel_y = addShort1DRef("pixel_y", Units, { 2 });
  auto& pixel_x = addShort1DRef("pixel_x", Units, { 2 });
  auto& counts  = addInt1DRef("pixel_count", Units, { 2 });

  // Now fill the pixel array...
  lastValue = backgroundValue;
  size_t at = 0;

  for (size_t x = 0; x < sizes[0]; x++) {
    for (size_t y = 0; y < sizes[1]; y++) {
      float& v = data[x][y];
      // Ok we have a value not background...
      if (v != backgroundValue) {
        // If it's part of the current RLE
        if (v == lastValue) {
          // Add to the previous count...
          ++counts[at - 1];
        } else {
          // ...otherwise start a new run
          pixel_x[at] = x;
          pixel_y[at] = y;
          pixels[at]  = v;
          counts[at]  = 1;
          ++at; // move forward
        }
      }
      lastValue = v;
    }
  }

  // Extra Meta for array
  auto pixelsDataArray = getNode(Constants::PrimaryDataName);

  if (pixelsDataArray) {
    pixelsDataArray->setLong("missing_value", Constants::MissingData);
    pixelsDataArray->setLong("BackgroundValue", backgroundValue);
    pixelsDataArray->setFloat("SparseGridCompression", compr_ratio);
    pixelsDataArray->setLong("NumValidRuns", pixels.size());
  }

  // Change DataType to have "Sparse" in front of it...
  std::string datatype = getDataType();

  setDataType("Sparse" + datatype);

  return true;
} // DataGrid::sparse2D

void
DataGrid::unsparseRestore()
{
  std::string datatype = getDataType();

  if (!Strings::beginsWith(datatype, "Sparse")) { return; }

  // LatLonGrid...RadialSet
  deleteArrayName(Constants::PrimaryDataName); // Deleting the sparse array
  deleteArrayName("pixel_z");                  // Might be there for 3D..this is ok if it's not
  deleteArrayName("pixel_y");
  deleteArrayName("pixel_x");
  deleteArrayName("pixel_count");

  // Remove the dimension we added in sparse2D/sparse3D
  myDims.pop_back();

  // Put back our saved primary array from the sparse2D/sparse3D above...
  if (haveArrayName("DisabledPrimary")) {
    changeArrayName("DisabledPrimary", Constants::PrimaryDataName);
    setVisible(Constants::PrimaryDataName, true);
  }

  Strings::removePrefix(datatype, "Sparse");
  setDataType(datatype);
}

std::string
DataGrid::getUnits(const std::string& name)
{
  // Default to the global units
  std::string units = DataType::getUnits(name);

  // Update node if any
  auto n = getNode(name);

  if (n != nullptr) {
    n->getString("Units", units);
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
    n->setString("Units", units);
  }
}
