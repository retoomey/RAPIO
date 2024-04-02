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
LatLonHeightGrid::postRead(std::map<std::string, std::string>& keys)
{
  // FIXME: Make it stay compressed for things like rcopy, probably
  // using a key
  // FIXME: I think we could allow unsparse into a N 2D implementation as well
  unsparse3D(myDims[1].size(), myDims[2].size(), myDims[0].size(), keys);
} // LatLonHeightGrid::postRead

void
LatLonHeightGrid::preWrite(std::map<std::string, std::string>& keys)
{
  if (sparse3D()) {
    setDataType("SparseLatLonHeightGrid");
  }
} // LatLonHeightGrid::preWrite

void
LatLonHeightGrid::postWrite(std::map<std::string, std::string>& keys)
{
  // These depend on the source array anyway..so have to be regenerated
  // on next write
  if (myDims.size() != 4) {
    return;
  }
  deleteArrayName(Constants::PrimaryDataName); // Deleting the sparse array
  deleteArrayName("pixel_z");
  deleteArrayName("pixel_y");
  deleteArrayName("pixel_x");
  deleteArrayName("pixel_count");

  // Remove the dimension we added in makeSparse
  myDims.pop_back();

  // Put back our saved primary array from the makeSparse above...
  changeArrayName("DisabledPrimary", Constants::PrimaryDataName);
  setVisible(Constants::PrimaryDataName, true);

  setDataType("LatLonHeightGrid");
}
