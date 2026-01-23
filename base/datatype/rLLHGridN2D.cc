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
LLHGridN2D::Clone()
{
  auto nsp = std::make_shared<LLHGridN2D>();

  LLHGridN2D::deep_copy(nsp);
  return nsp;
}

void
LLHGridN2D::deep_copy(std::shared_ptr<LLHGridN2D> nsp)
{
  LatLonHeightGrid::deep_copy(nsp);

  // Clone our grids...
  for (auto g:myGrids) {
    nsp->myGrids.push_back(g->Clone());
  }
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
LLHGridN2D::setUnits(const std::string& units, const std::string& name)
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
LLHGridN2D::preWrite(std::map<std::string, std::string>& keys)
{
  // Check if sparse already...
  auto pixelptr = getFloat1D("pixel_x");

  if (pixelptr != nullptr) {
    fLogInfo("Not making sparse since pixels already exists...");
    return;
  }

  // ----------------------------------------------------------------------------
  // Calculate size of pixels required to store the data.
  // We 'could' also just do it and let vectors grow "might" be faster
  auto sizes = getSizes();

  // We don't have a 3D array here..we have a set of N 2D...soooo
  // auto& data = getFloat3DRef(Constants::PrimaryDataName);

  float backgroundValue = Constants::MissingData; // default and why not unvailable?
  size_t neededPixels   = 0;
  float lastValue       = backgroundValue;

  const size_t size = myGrids.size(); // ALPHA not tested yet

  for (size_t i = 0; i < size; i++) {
    auto g = myGrids[i];
    if (g == nullptr) { g = get(i); }
    auto& array = g->getFloat2DRef();
    for (size_t x = 0; x < sizes[1]; x++) {
      for (size_t y = 0; y < sizes[2]; y++) {
        float& v = array[x][y];
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

  // Ok we don't have a primary at least right now...so get units from the first
  // LatLonGrid we have...
  std::string dataunits = "dimensionless";

  if (myGrids.size() > 0) {
    auto g = myGrids[0];
    if (g == nullptr) { g = get(0); }
    dataunits = g->getUnits();
  }

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

  size_t total  = 0;
  size_t total2 = 0;

  for (size_t i = 0; i < size; i++) { // ALPHA: is z matching order?
    auto g = myGrids[i];
    if (g == nullptr) { g = get(i); }
    auto& data = g->getFloat2DRef();

    for (size_t x = 0; x < sizes[1]; x++) {
      for (size_t y = 0; y < sizes[2]; y++) {
        total2++;
        float& v = data[x][y];
        // Ok we have a value not background...
        if (v != backgroundValue) {
          // If it's part of the current RLE
          if (v == lastValue) {
            // Add to the previous count...
            ++counts[at - 1];
            total++;
          } else {
            // ...otherwise start a new run
            pixel_z[at] = i; // flipped or not?
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
  setDataType("SparseLatLonHeightGrid");
} // LLHGridN2D::makeSparse

void
LLHGridN2D::postWrite(std::map<std::string, std::string>& keys)
{
  unsparseRestore();
}
