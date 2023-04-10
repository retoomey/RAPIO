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
  myDataType = "LatLonHeightGrid";
}

LatLonHeightGrid::LatLonHeightGrid(
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
) : LatLonGrid(TypeName, Units, location, time, lat_spacing, lon_spacing, num_lats, num_lons,
    { num_lats, num_lons, num_layers }, { "Lat", "Lon", "Ht" })
{
  // Our current LatLonHeight grid is implemented using a 3D array
  addFloat3D(Constants::PrimaryDataName, Units, { 0, 1, 2 });

  // Note layers random...you need to fill them all during data reading
  myLayerNumbers.resize(num_layers);
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
  return (std::make_shared<LatLonHeightGrid>(TypeName, Units, location, time, lat_spacing, lon_spacing, num_lats,
         num_lons, num_layers));
}

std::shared_ptr<DataProjection>
LatLonHeightGrid::getProjection(const std::string& layer)
{
  return std::make_shared<LatLonHeightGridProjection>(layer, this);
}
