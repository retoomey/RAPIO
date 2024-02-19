#include "rLatLonHeightGrid.h"
#include "rProject.h"

using namespace rapio;
using namespace std;

std::string
LatLonHeightGrid::getGeneratedSubtype() const
{
  return (formatString(myLocation.getHeightKM(), 5, 2));
}

LatLonHeightGrid::LatLonHeightGrid()
{
  setDataType("LatLonHeightGrid");
}

std::shared_ptr<LatLonHeightGrid>
LatLonHeightGrid::Create(
  const std::string& TypeName,
  const std::string& Units,

  // Projection information (metadata of the each 2D)
  const LLH   & location,
  const Time  & time,
  const float lat_spacing,
  const float lon_spacing,

  // Basically the 3D array
  size_t num_lats,
  size_t num_lons,
  size_t num_layers
)
{
  auto newonesp = std::make_shared<LatLonHeightGrid>();

  newonesp->init(TypeName, Units, location, time, lat_spacing, lon_spacing, num_lats, num_lons, num_layers);
  return newonesp;
}

bool
LatLonHeightGrid::init(
  const std::string& TypeName,
  const std::string& Units,

  // Projection information (metadata of the each 2D)
  const LLH   & location,
  const Time  & datatime,
  const float lat_spacing,
  const float lon_spacing,

  // Basically the 3D array
  size_t num_lats,
  size_t num_lons,
  size_t num_layers
)
{
  DataGrid::init(TypeName, Units, location, datatime, { num_lats, num_lons, num_layers }, { "Lat", "Lon", "Ht" });

  setDataType("LatLonHeightGrid");

  setSpacing(lat_spacing, lon_spacing);

  // Our current LatLonHeight grid is implemented using a 3D array
  addFloat3D(Constants::PrimaryDataName, Units, { 0, 1, 2 });

  // Note layers random...you need to fill them all during data reading
  myLayerNumbers.resize(num_layers);
  return true;
}

std::shared_ptr<DataProjection>
LatLonHeightGrid::getProjection(const std::string& layer)
{
  if (myDataProjection == nullptr) {
    myDataProjection = std::make_shared<LatLonHeightGridProjection>(layer, this);
  }
  return myDataProjection;
}

void
LatLonHeightGrid::makeSparse()
{
  // Alpha code as I transition sparse ability out of the netcdf reader/writer..
  // though at this point reading is still in the netcdf reader module.
  // I think having sparse in the class not the netcdf writer makes more sense to keep the code
  // clean.  However, to do this we need to implement non-writable arrays (a hidden ability)
  // For alpha gonna hold onto the array ourselves...we should match makeSparse with
  // makeNonSparse calls
  LogInfo("--->Alpha Make sparse called on LLHGrid.\n");

  // Humm do we want the ability to redo the sparse array?  Probably, since the data
  // will change.
  auto pixelptr = getFloat1D("pixel_x");

  if (pixelptr != nullptr) {
    LogInfo("Not making sparse since pixels already exists...\n");
    return;
  }

  // ----------------------------------------------------------------------------
  // Calculate size of pixels required to store the data.
  // We 'could' also just do it and let vectors grow "might" be faster
  auto sizes = getSizes();
  auto& data = getFloat3DRef(Constants::PrimaryDataName);
  float backgroundValue = Constants::MissingData; // default and why not unvailable?
  size_t neededPixels   = 0;
  float lastValue       = backgroundValue;

  for (size_t z = 0; z < sizes[0]; z++) {
    for (size_t x = 0; x < sizes[1]; x++) {
      for (size_t y = 0; y < sizes[2]; y++) {
        float& v = data[x][y][z];
        if ((v != backgroundValue) && (v != lastValue) ) {
          ++neededPixels;
        }
        lastValue = v;
      }
    }
  }

  // ----------------------------------------------------------------------------
  // Modify ourselves to be parse.  But we need to keep our actual data (probably)
  // Add a 'pixel' dimension...we can add (ONCE) without messing with current arrays
  // since it's the last dimension. FIXME: more api probably
  myDims.push_back(DataGridDimension("pixel", neededPixels));

  // Move the primary out of the way and mark it hidden to writers...
  // we don't want to delete because the caller may be using the array and has no clue
  // about the sparse stuff
  std::string dataunits = getUnits(); // Get units first since it comes from primary

  changeName(Constants::PrimaryDataName, "DisabledPrimary");
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
        float& v = data[x][y][z];
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

  LogInfo("Need sparse: " << neededPixels << "  final sparse: " << at << "\n");
} // LatLonHeightGrid::makeSparse

void
LatLonHeightGrid::makeNonSparse()
{
  LogInfo("------>Calling non sparse to restore LLHGrid RAM array.\n");
  if (myDims.size() != 4) {
    return;
  }
  // These depend on the source array anyway..so have to be regenerated
  // on next write
  deleteName(Constants::PrimaryDataName); // Deleting the sparse array
  deleteName("pixel_z");
  deleteName("pixel_y");
  deleteName("pixel_x");
  deleteName("pixel_count");

  // Remove the dimension we added in makeSparse
  myDims.pop_back();

  // Put back our saved primary array from the makeSparse above...
  changeName("DisabledPrimary", Constants::PrimaryDataName);
  setVisible(Constants::PrimaryDataName, true);
}
