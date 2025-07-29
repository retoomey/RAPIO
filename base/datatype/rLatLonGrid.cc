#include "rLatLonGrid.h"
#include "rLatLonGridProjection.h"

using namespace rapio;
using namespace std;

std::string
LatLonGrid::getGeneratedSubtype() const
{
  return (formatString(myLocation.getHeightKM(), 5, 2));
}

LatLonGrid::LatLonGrid()
{
  setDataType("LatLonGrid");
}

bool
LatLonGrid::init(
  const std::string & TypeName,
  const std::string & Units,
  const LLH         & location,
  const Time        & datatime,
  const float       lat_spacing,
  const float       lon_spacing,
  size_t            num_lats,
  size_t            num_lons
)
{
  DataGrid::init(TypeName, Units, location, datatime, { num_lats, num_lons }, { "Lat", "Lon" });

  setDataType("LatLonGrid");

  setSpacing(lat_spacing, lon_spacing);

  addFloat2D(Constants::PrimaryDataName, Units, { 0, 1 });

  return true;
}

std::shared_ptr<LatLonGrid>
LatLonGrid::Create(
  const std::string& TypeName,
  const std::string& Units,

  // Projection information (metadata of the 2D)
  const LLH   & location,
  const Time  & time,
  const float lat_spacing,
  const float lon_spacing,

  // Basically the 2D array
  size_t num_lats,
  size_t num_lons
)
{
  auto newonesp = std::make_shared<LatLonGrid>();

  newonesp->init(TypeName, Units, location, time, lat_spacing, lon_spacing, num_lats, num_lons);
  return newonesp;
}

std::shared_ptr<LatLonGrid>
LatLonGrid::Create(
  const std::string    & TypeName,
  const std::string    & Units,
  const Time           & time,
  const LLCoverageArea & g)
{
  auto newonesp = std::make_shared<LatLonGrid>();

  newonesp->init(TypeName, Units, LLH(g.getNWLat(), g.getNWLon(), 0),
    time, g.getLatSpacing(), g.getLonSpacing(), g.getNumY(), g.getNumX());
  return newonesp;
}

std::shared_ptr<LatLonGrid>
LatLonGrid::Clone()
{
  auto nsp = std::make_shared<LatLonGrid>();

  LatLonGrid::deep_copy(nsp);
  return nsp;
}

void
LatLonGrid::RemapInto(std::shared_ptr<LatLonGrid> out, std::shared_ptr<ArrayAlgorithm> remapper)
{
  if (out == nullptr) { return; }

  auto& r = *remapper;
  auto& o = *out;

  // FIXME: Can we know name/params of the particular remapper
  LogInfo("Remapping using matrix size of " << r.myWidth << " by " << r.myHeight << "\n");

  // FIXME: We're only mapping primary data array
  r.setSource(getFloat2D());
  r.setOutput(o.getFloat2D());

  // Input coordinates
  const LLH inCorner = getTopLeftLocationAt(0, 0); // not centered
  const AngleDegs inNWLatDegs = inCorner.getLatitudeDeg();
  const AngleDegs inNWLonDegs = inCorner.getLongitudeDeg();
  const auto inLatSpacingDegs = getLatSpacing();
  const auto inLonSpacingDegs = getLonSpacing();

  // Output coordinates
  const LLH outCorner = o.getTopLeftLocationAt(0, 0); // not centered
  const AngleDegs outNWLatDegs = outCorner.getLatitudeDeg();
  const AngleDegs outNWLonDegs = outCorner.getLongitudeDeg();

  const AngleDegs startLat = outNWLatDegs - (o.getLatSpacing() / 2.0); // move south (lat decreasing)
  const AngleDegs startLon = outNWLonDegs + (o.getLonSpacing() / 2.0); // move east (lon increasing)
  const size_t numY        = o.getNumLats();
  const size_t numX        = o.getNumLons();

  // Cell hits yof and xof
  // Note the cell is allowed to be fractional and out of range,
  // since we're doing a matrix 'some' cells might be in the range
  size_t counter = 0;

  AngleDegs atLat = startLat;

  auto& array = *r.myRefOut;

  for (size_t y = 0; y < numY; ++y, atLat -= o.getLatSpacing()) {
    const float yof = (inNWLatDegs - atLat) / inLatSpacingDegs;

    AngleDegs atLon = startLon;
    for (size_t x = 0; x < numX; ++x, atLon += o.getLonSpacing()) {
      const float xof = (atLon - inNWLonDegs ) / inLonSpacingDegs;
      float value;
      if (r.remap(yof, xof, value)) {
        array[y][x] = value; // y/x is boost memory order for speed
        counter++;
      }
    } // endX
  }   // endY

  if (counter > 0) {
    LogInfo("Sample hit counter is " << counter << "\n");
  }
} // LatLonGrid::RemapInto

void
LatLonGrid::deep_copy(std::shared_ptr<LatLonGrid> nsp)
{
  LatLonArea::deep_copy(nsp);

  // We don't have extra fields
}

std::shared_ptr<DataProjection>
LatLonGrid::getProjection(const std::string& layer)
{
  // FIXME: Caching now so first layer wins.  We'll need something like
  // setLayer on the projection I think 'eventually'.  We're only using
  // primary layer at moment
  if (myDataProjection == nullptr) {
    myDataProjection = std::make_shared<LatLonGridProjection>(layer, this);
  }
  return myDataProjection;
}

void
LatLonGrid::postRead(std::map<std::string, std::string>& keys)
{
  // For now, we always unsparse to full.  Though say in rcopy we
  // would want to keep it sparse.  FIXME: have a key control this
  unsparse2D(getNumLats(), getNumLons(), keys);
} // LatLonGrid::postRead

void
LatLonGrid::preWrite(std::map<std::string, std::string>& keys)
{
  sparse2D(); // Standard sparse of primary data (add dimension)
}

void
LatLonGrid::postWrite(std::map<std::string, std::string>& keys)
{
  unsparseRestore();
}
