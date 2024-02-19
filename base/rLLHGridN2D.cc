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

  // Copy into the layer values.  However, these are int, so since the heights
  // are partial KMs, we need to upscale to fit it.
  for (size_t i = 0; i < h.size(); i++) {
    newonesp->setLayerValue(i, h[i] * 1000); // meters
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
  // We act like 3D...we won't store a 3D array though...
  DataGrid::init(TypeName, Units, location, datatime, { num_layers, num_lats, num_lons }, { "Ht", "Lat", "Lon" });

  // setDataType("LLHGridN2D");
  setDataType("LatLonHeightGrid"); // We're actually an implementation/view of LatLonHeightGrid

  setSpacing(lat_spacing, lon_spacing);

  // Our current LatLonHeight grid is implemented using a 3D array
  // addFloat3D(Constants::PrimaryDataName, Units, { 0, 1, 2 });
  myGrids = std::vector<std::shared_ptr<LatLonGrid> >(num_layers, nullptr);

  // Note layers random...you need to fill them all during data reading
  // FIXME: Couldn't we use a regular array here
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
    Time t = myTime;                           // Default time to our time, caller can change it later
    LLH l  = myLocation;                       // Default location and height is our layer number
    l.setHeightKM(myLayerNumbers[i] / 1000.0); // We stored meter level resolution

    // FIXME: getNumLats get Z here, because Lak changed dimension ordering.  Need work
    myGrids[i] = LatLonGrid::Create(myTypeName, myUnits, l, t, myLatSpacing, myLonSpacing,
 // getNumLats(), getNumLons());
        myDims[1].size(), myDims[2].size());
    // LogInfo("Lazy created LatLonGrid " << i+1 << " of " << myGrids.size() << " total layers.\n");
  }

  if (myGrids[i] == nullptr) {
    LogSevere("Failed to get grid " << i << " from LLHGridN2D\n");
  }
  return myGrids[i];
}

void
LLHGridN2D::fillPrimary(float value)
{
  // std::vector<int> myLayerNumbers;
  const size_t size = myGrids.size();

  for (size_t i = 0; i < size; i++) {
    auto g = myGrids[i];
    if (g == nullptr) { g = get(i); }
    auto array = g->getFloat2D();
    array->fill(value);
  }
}

void
LLHGridN2D::makeSparse()
{
  // Alpha code as I transition sparse ability out of the netcdf reader/writer..
  // though at this point reading is still in the netcdf reader module.
  // I think having sparse in the class not the netcdf writer makes more sense to keep the code
  // clean.  However, to do this we need to implement non-writable arrays (a hidden ability)
  // For alpha gonna hold onto the array ourselves...we should match makeSparse with
  // makeNonSparse calls
  LogInfo("--->Alpha Make sparse called on LLHGridN2D.\n");

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

  LogInfo(">>>>NEEDED PIXELS " << neededPixels << "\n");
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

  // Move the primary out of the way and mark it hidden to writers...
  // we don't want to delete because the caller may be using the array and has no clue
  // about the sparse stuff
  // We don't currently have a primary array...we store N LatLonGrids...
  // changeName(Constants::PrimaryDataName, "DisabledPrimary");
  // setVisible("DisabledPrimary", false); // turn off writing

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

  LogInfo("Sizes: " << size << "(" << sizes[0] << ") and " << sizes[1] << ", " << sizes[2] << "\n");
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

  LogInfo("Indirect total: " << total << " and " << total2 << "\n");
  LogInfo("Need sparse: " << neededPixels << "  final sparse: " << at << "\n");
} // LLHGridN2D::makeSparse

void
LLHGridN2D::makeNonSparse()
{
  LogInfo("------>Calling non sparse to restore LLHGridN2D\n");
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
  // We don't currently have a primary array
  //  changeName("DisabledPrimary", Constants::PrimaryDataName);
  //  setVisible(Constants::PrimaryDataName, true);
}
