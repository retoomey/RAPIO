#include "rLatLonHeightGrid.h"
#include "rProject.h"

using namespace rapio;
using namespace std;

std::string
LatLonHeightGrid::getGeneratedSubtype() const
{
  return "full3D";
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
  LogInfo("****** INIT CALLED ON LATLONHEIGHTGRID\n");
  // Lak changed dimension ordering..so we have to start with Ht first here.
  // Not sure if this will break reading in the netcdf reader.
  DataGrid::init(TypeName, Units, location, datatime, { num_layers, num_lats, num_lons }, { "Ht", "Lat", "Lon" });

  setDataType("LatLonHeightGrid");

  setSpacing(lat_spacing, lon_spacing);

  // Our current LatLonHeight grid is implemented using a 3D array
  addFloat3D(Constants::PrimaryDataName, Units, { 0, 1, 2 });

  // A Height array
  addFloat1D("Height", "Meters", { 0 });

  // Note layers random...you need to fill them all during data reading
  // FIXME: Could use an array, right?  Gotta write it anyway
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
LatLonHeightGrid::preWrite(bool sparse)
{
  // Copy height array always for writing
  auto& heights = getFloat1DRef("Height");

  for (size_t i = 0; i < myLayerNumbers.size(); ++i) {
    heights[i] = myLayerNumbers[i];
  }

  if (!sparse) {
    return;
  }

  // Check if sparse already...
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

  auto dataptr = getFloat3D(Constants::PrimaryDataName);

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
} // LatLonHeightGrid::preWrite

void
LatLonHeightGrid::postWrite(bool sparse)
{
  // Nothing to undo here
  if (!sparse) {
    return;
  }

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
