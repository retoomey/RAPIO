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
  auto llgridsp = std::make_shared<LatLonHeightGrid>();

  if (llgridsp == nullptr) {
    LogSevere("Couldn't create LatLonHeightGrid\n");
  } else {
    // We post constructor fill in details because many of the factories like netcdf 'chain' layers and settings
    llgridsp->init(TypeName, Units, location, time, lat_spacing, lon_spacing, num_lats, num_lons, num_layers);
  }

  return llgridsp;
}

void
LatLonHeightGrid::init(
  const std::string& TypeName,
  const std::string& Units,
  const LLH        & location,
  const Time       & time,
  const float      lat_spacing,
  const float      lon_spacing,
  size_t           num_lats,
  size_t           num_lons,
  size_t           num_layers
)
{
  setTypeName(TypeName);
  setDataAttributeValue("Unit", "dimensionless", Units);

  myLocation   = location;
  myTime       = time;
  myLatSpacing = lat_spacing;
  myLonSpacing = lon_spacing;

  bool okLat = (myLatSpacing > 0);
  bool okLon = (myLonSpacing > 0);

  if (!(okLat && okLon)) {
    LogSevere("*** WARNING *** non-positive element in grid spacing\n"
      << "(" << lat_spacing << "," << lon_spacing << ")\n");
  }

  // Our current LatLonHeight grid is implemented using a 3D array
  declareDims({ num_lats, num_lons, num_layers }, { "Lat", "Lon", "Ht" });
  addFloat3D(Constants::PrimaryDataName, Units, { 0, 1, 2 });

  // Note layers random...you need to fill them all during data reading
  myLayerNumbers.resize(num_layers);
} // LatLonHeightGrid::init

std::shared_ptr<DataProjection>
LatLonHeightGrid::getProjection(const std::string& layer)
{
  return std::make_shared<LatLonHeightGridProjection>(layer, this);
}
