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

std::shared_ptr<LatLonHeightGrid>
LatLonHeightGrid::Create(
  const std::string    & TypeName,
  const std::string    & Units,
  const Time           & time,
  const LLCoverageArea & g)
{
  auto newonesp = std::make_shared<LatLonHeightGrid>();

  auto h = g.getHeightsKM();
  const LengthKMs bottomKMs = (h.size() < 1) ? 0 : h[0];

  newonesp->init(TypeName, Units, LLH(g.getNWLat(), g.getNWLon(), bottomKMs),
    time, g.getLatSpacing(), g.getLonSpacing(), g.getNumY(), g.getNumX(), g.getNumZ());
  // Copy CoverageArea heights into our layer numbers since we passed a coverage area

  // Copy into the layer values.  However, these are int, so since the heights
  // are partial KMs, we need to upscale to fit it.
  for (size_t i = 0; i < h.size(); i++) {
    newonesp->setLayerValue(i, h[i] * 1000); // meters
  }
  return newonesp;
}

std::shared_ptr<LatLonHeightGrid>
LatLonHeightGrid::Clone()
{
  auto nsp = std::make_shared<LatLonHeightGrid>();

  LatLonHeightGrid::deep_copy(nsp);
  return nsp;
}

void
LatLonHeightGrid::deep_copy(std::shared_ptr<LatLonHeightGrid> nsp)
{
  LatLonArea::deep_copy(nsp);

  // We don't have extra fields
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
  sparse3D(); // Standard sparse of primary data (add dimension)
} // LatLonHeightGrid::preWrite

void
LatLonHeightGrid::postWrite(std::map<std::string, std::string>& keys)
{
  unsparseRestore();
}
