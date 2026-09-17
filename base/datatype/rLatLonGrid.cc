#include "rLatLonGrid.h"
#include "rLatLonGridProjection.h"
#include "rArith.h"

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
LatLonGrid::Clone() const
{
  auto nsp = std::make_shared<LatLonGrid>();

  LatLonGrid::deep_copy(nsp);
  return nsp;
}

void
LatLonGrid::RemapInto(std::shared_ptr<LatLonGrid> out, std::shared_ptr<ArrayPipeline> pipeline)
{
  if (!out || !pipeline) { return; }

  // Our array has special geometry in the array pipeline
  // (for Cressman distances, etc.)
  LatLonGridMapper geoMapper(*this, *out);

  pipeline->remap(this->getFloat2D(), out->getFloat2D(), geoMapper);
}

bool
LatLonGrid::OverlayAligned(std::shared_ptr<LatLonGrid> dest)
{
  if (!dest) { return false; }

  // 1. Validate Resolution matches
  if (!Arith::feq(getLatSpacing(), dest->getLatSpacing()) ||
    !Arith::feq(getLonSpacing(), dest->getLonSpacing()))
  {
    fLogSevere("BlockCopyInto failed: Resolution mismatch.");
    return false;
  }

  auto& srcData = getFloat2DRef();
  auto& dstData = dest->getFloat2DRef();

  int dstRows = dest->getNumLats();
  int dstCols = dest->getNumLons();
  int srcRows = getNumLats();
  int srcCols = getNumLons();

  // 2. Calculate offsets relative to the destination grid
  int startDestY =
    std::round((dest->getLocation().getLatitudeDeg() - getLocation().getLatitudeDeg()) / getLatSpacing());
  int startDestX = std::round(
    (getLocation().getLongitudeDeg() - dest->getLocation().getLongitudeDeg()) / getLonSpacing());

  // 3. Calculate the exact intersection loop bounds relative to the SOURCE grid
  // This automatically handles clipping if the source is larger!
  int startSrcY = std::max(0, -startDestY);
  int startSrcX = std::max(0, -startDestX);
  int endSrcY   = std::min(srcRows, dstRows - startDestY);
  int endSrcX   = std::min(srcCols, dstCols - startDestX);

  // 4. Check if grids actually overlap at all
  if ((startSrcY >= endSrcY) || (startSrcX >= endSrcX)) {
    // fLogInfo("BlockCopyInto: Grids do not overlap. Skipping.");
    return true; // Completely disjoint but not a failure
  }

  // This should be pretty fast
  //
  // 5. Tightly loop ONLY over the overlapping intersection
  for (int y = startSrcY; y < endSrcY; ++y) {
    int destY = startDestY + y;

    for (int x = startSrcX; x < endSrcX; ++x) {
      int destX = startDestX + x;

      // Only copy valid data?
      // float val = srcData[y][x];
      //  I don't think this matters for true intersection
      // if (val != Constants::DataUnavailable) {
      dstData[destY][destX] = srcData[y][x];
      // }
    }
  }
  return true;
} // LatLonGrid::OverlayAligned

void
LatLonGrid::deep_copy(std::shared_ptr<LatLonGrid> nsp) const
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
LatLonGrid::postRead(IOConfig& keys)
{
  // For now, we always unsparse to full.  Though say in rcopy we
  // would want to keep it sparse.  FIXME: have a key control this
  unsparse2D(getNumLats(), getNumLons(), keys);
} // LatLonGrid::postRead

void
LatLonGrid::preWrite(IOConfig& keys)
{
  // FIXME: Settings for sparse right
  sparse2D(); // Standard sparse of primary data (add dimension)
}

void
LatLonGrid::postWrite(IOConfig& keys)
{
  unsparseRestore();
}
