#include "rLLHGridN2D.h"
#include "rLLH.h"
#include "rSparseEncoder.h"

using namespace rapio;
using namespace std;

std::shared_ptr<LLHGridN2D>
LLHGridN2D::Create(
  std::string_view TypeName,
  std::string_view Units,
  const Time       & time,

  // Projection information
  const LLH   & location,
  const float lat_spacing,
  const float lon_spacing,

  // Size information
  size_t num_lats,
  size_t num_lons,
  size_t num_layers
)
{
  auto newonesp = std::make_shared<LLHGridN2D>();

  newonesp->init(TypeName, Units, location, time, lat_spacing, lon_spacing, num_lats, num_lons, num_layers);
  return newonesp;
}

std::shared_ptr<LLHGridN2D>
LLHGridN2D::Create(
  std::string_view     TypeName,
  std::string_view     Units,
  const Time           & time,
  const LLCoverageArea & g)
{
  auto newonesp = std::make_shared<LLHGridN2D>();

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

std::shared_ptr<LLHGridN2D>
LLHGridN2D::Clone() const
{
  auto nsp = std::make_shared<LLHGridN2D>();

  LLHGridN2D::deep_copy(nsp);
  return nsp;
}

void
LLHGridN2D::deep_copy(std::shared_ptr<LLHGridN2D> nsp) const
{
  LatLonHeightGrid::deep_copy(nsp);

  // Clone our grids...
  for (auto g:myGrids) {
    nsp->myGrids.push_back(g->Clone());
  }
}

bool
LLHGridN2D::init(
  std::string_view TypeName,
  std::string_view Units,

  // Projection information
  const LLH   & location,
  const Time  & datatime,
  const float lat_spacing,
  const float lon_spacing,

  // Size information
  size_t num_lats,
  size_t num_lons,
  size_t num_layers
)
{
  // We act like 3D...we won't store a 3D array though...
  DataGrid::init(TypeName, Units, location, datatime, { num_layers, num_lats, num_lons }, { "Ht", "Lat", "Lon" });

  setDataType("LatLonHeightGrid"); // We're actually an implementation/view of LatLonHeightGrid

  setSpacing(lat_spacing, lon_spacing);

  // Our current LatLonHeight grid is implemented using a 3D array
  // addFloat3D(Constants::PrimaryDataName, Units, { 0, 1, 2 });
  myGrids = std::vector<std::shared_ptr<LatLonGrid> >(num_layers, nullptr);

  // A Height array
  addFloat1D("Height", "Meters", { 0 });

  return true;
}

void
LLHGridN2D::setUnits(std::string_view units, std::string_view name)
{
  DataGrid::setUnits(units, name);

  // Echo primary to our N layers...
  if (name == Constants::PrimaryDataName) {
    for (auto& g:myGrids) {
      g->setUnits(units, name);
    }
  }
}

std::shared_ptr<LatLonGrid>
LLHGridN2D::get(size_t i)
{
  if (i >= myGrids.size()) {
    fLogSevere("Attempting to get LatLonGrid {}, but our size is {}", i, myGrids.size());
    return nullptr;
  }

  // Lazy creation or explicit creation method?
  // Create a new working space LatLonGrid for the layer
  if (myGrids[i] == nullptr) {
    Time t = myTime;                          // Default time to our time, caller can change it later
    LLH l  = myLocation;                      // Default location and height is our layer number
    l.setHeightKM(getLayerValue(i) / 1000.0); // We stored meter level resolution
    myGrids[i] = LatLonGrid::Create(myTypeName, getUnits(), l, t, getLatSpacing(), getLonSpacing(),
        getNumLats(), getNumLons());
    // fLogInfo("Lazy created LatLonGrid {} of {} total layers.", i+1, myGrids.size());
  }

  if (myGrids[i] == nullptr) {
    fLogSevere("Failed to get grid {} from LLHGridN2D", i);
  }
  return myGrids[i];
}

void
LLHGridN2D::fillPrimary(float value)
{
  const size_t size = myGrids.size();

  for (size_t i = 0; i < size; i++) {
    auto g = myGrids[i];
    if (g == nullptr) { g = get(i); }
    auto array = g->getFloat2D();
    array->fill(value);
  }
}

void
LLHGridN2D::preWrite(IOConfig& keys)
{
  std::vector<std::shared_ptr<Array<float, 2> > > arrays;

  arrays.reserve(myGrids.size());
  for (size_t i = 0; i < myGrids.size(); i++) {
    auto g = myGrids[i] ? myGrids[i] : get(i);
    arrays.push_back(g->getFloat2D());
  }
  SparseEncoder::sparse3DN(*this, keys, arrays);
}

void
LLHGridN2D::postWrite(IOConfig&)
{
  SparseEncoder::unsparseRestore(*this);
}
