#include "rLLHGridN2D.h"
#include "rLLH.h"

using namespace rapio;
using namespace std;

std::shared_ptr<LLHGridN2D>
LLHGridN2D::Create(
  const std::string& TypeName,
  const std::string& Units,
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
  const std::string    & TypeName,
  const std::string    & Units,
  const Time           & time,
  const LLCoverageArea & g)
{
  auto newonesp = std::make_shared<LLHGridN2D>();

  newonesp->init(TypeName, Units, LLH(g.getNWLat(), g.getNWLon(), 0),
    time, g.getLatSpacing(), g.getLonSpacing(), g.getNumY(), g.getNumX(), g.getNumZ());
  // Copy CoverageArea heights into our layer numbers since we passed a coverage area
  auto h = g.getHeightsKM();

  for (size_t i = 0; i < h.size(); i++) {
    newonesp->setLayerValue(i, h[i]);
  }
  return newonesp;
}

bool
LLHGridN2D::init(
  const std::string& TypeName,
  const std::string& Units,

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
  // FIXME: go ahead and make a 2D in case we want to store each lat lon grid as a 2D array in
  // output later. Note we're not adding any arrays but in theory each layer could be just
  // an additional 2D array here instead of a full LatLonGrid, but eh.
  DataGrid::init(TypeName, Units, location, datatime, { num_lats, num_lons }, { "Lat", "Lon" });

  setDataType("LLHGridN2D");

  setSpacing(lat_spacing, lon_spacing);

  // Our current LatLonHeight grid is implemented using a 3D array
  // addFloat3D(Constants::PrimaryDataName, Units, { 0, 1, 2 });
  myGrids = std::vector<std::shared_ptr<LatLonGrid> >(num_layers, nullptr);

  // Note layers random...you need to fill them all during data reading
  myLayerNumbers.resize(num_layers);
  return true;
}

std::shared_ptr<LatLonGrid>
LLHGridN2D::get(size_t i)
{
  if (i >= myGrids.size()) {
    LogSevere("Attempting to get LatLonGrid " << i << ", but our size is " << myGrids.size() << "\n");
    return nullptr;
  }

  // Lazy creation or explicit creation method?
  // Create a new working space LatLonGrid for the layer
  if (myGrids[i] == nullptr) {
    Time t = myTime;     // Default time to our time, caller can change it later
    LLH l  = myLocation; // Default location and height is our layer number
    l.setHeightKM(myLayerNumbers[i]);
    myGrids[i] = LatLonGrid::Create(myTypeName, myUnits, l, t, myLatSpacing, myLonSpacing,
        getNumLats(), getNumLons());
    // LogInfo("Lazy created LatLonGrid " << i+1 << " of " << myGrids.size() << " total layers.\n");
  }

  return myGrids[i];
}

void
LLHGridN2D::fillPrimary(float value)
{
  std::vector<int> myLayerNumbers;
  const size_t size = myGrids.size();

  for (size_t i = 0; i < size; i++) {
    auto g = myGrids[i];
    if (g == nullptr) { g = get(i); }
    auto array = g->getFloat2D();
    array->fill(value);
  }
}
