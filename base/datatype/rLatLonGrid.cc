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
LatLonGrid::Clone()
{
  auto nsp = std::make_shared<LatLonGrid>();

  LatLonGrid::deep_copy(nsp);
  return nsp;
}

void
LatLonGrid::RemapInto(std::shared_ptr<LatLonGrid> out, std::shared_ptr<ArrayAlgorithm> pipeline)
{
  /** * Maps geographic coordinates between two LatLonGrids.
   * Encapsulates the logic originally found in LatLonGrid::RemapInto.
   */
  class LatLonGeoMapper : public CoordMapper {
public:
    LatLonGeoMapper(const LatLonGrid& source, const LatLonGrid& dest)
    {
      // Source metadata for pixel conversion
      //   myInNWLat      = source.getLocation().getLatitudeDeg();
      //   myInNWLon      = source.getLocation().getLongitudeDeg();
      myInNWLat = source.getLocation().getLatitudeDeg() - (source.getLatSpacing() / 2.0);
      myInNWLon = source.getLocation().getLongitudeDeg() + (source.getLonSpacing() / 2.0);

      myInLatSpacing = source.getLatSpacing();
      myInLonSpacing = source.getLonSpacing();

      // Destination metadata for coordinate generation
      // Note: HMRG/RAPIO uses Top-Left of the cell, so we center the sample
      myOutStartLat   = dest.getLocation().getLatitudeDeg() - (dest.getLatSpacing() / 2.0);
      myOutStartLon   = dest.getLocation().getLongitudeDeg() + (dest.getLonSpacing() / 2.0);
      myOutLatSpacing = dest.getLatSpacing();
      myOutLonSpacing = dest.getLonSpacing();
    }

    bool
    map(int destI, int destJ, float& outU, float& outV) override
    {
      // 1. Calculate absolute Lat/Lon for the destination pixel center
      double atLat = myOutStartLat - (destI * myOutLatSpacing);
      double atLon = myOutStartLon + (destJ * myOutLonSpacing);

      // 2. Convert that Lat/Lon into the fractional pixel index of the source grid
      outU = (myInNWLat - atLat) / myInLatSpacing;
      outV = (atLon - myInNWLon) / myInLonSpacing;

      // Valid if within source bounds (handled by ArrayAlgorithm::sampleAt/resolveX)
      return true;
    }

private:
    double myInNWLat, myInNWLon, myInLatSpacing, myInLonSpacing;
    double myOutStartLat, myOutStartLon, myOutLatSpacing, myOutLonSpacing;
  };

  if (!out || !pipeline) { return; }
  // Instantiate the geo-spatial helper
  LatLonGeoMapper geoMapper(*this, *out);

  // Get the underlying 2D data arrays
  auto srcData = this->getFloat2D();
  auto dstData = out->getFloat2D();

  // Execute the unified process loop
  pipeline->remap(srcData, dstData, &geoMapper);

  #if 0
  Leaving old code for now.It makes sense to have a helper object class here,
    which allows us to optimize the ArrayAlgorithm process call

    auto& r = *remapper;
  auto& o   = *out;

  // FIXME: We're only mapping primary data array
  r.setSource(getFloat2D());

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

  auto& array = o.getFloat2DRef();

  for (size_t y = 0; y < numY; ++y, atLat -= o.getLatSpacing()) {
    const float yof = (inNWLatDegs - atLat) / inLatSpacingDegs;

    AngleDegs atLon = startLon;
    for (size_t x = 0; x < numX; ++x, atLon += o.getLonSpacing()) {
      const float xof = (atLon - inNWLonDegs ) / inLonSpacingDegs;
      float value;
      if (r.sampleAt(yof, xof, value)) {
        array[y][x] = value; // y/x is boost memory order for speed
        counter++;
      }
    } // endX
  }   // endY

  if (counter > 0) {
    fLogInfo("Sample hit counter is {}", counter);
  }
  #endif // if 0
} // LatLonGrid::RemapInto

#if 0
bool
LatLonGrid::BlockCopyInto(std::shared_ptr<LatLonGrid> dest)
{
  if (!dest) { return false; }

  // 1. Validate Resolution
  if (!Arith::feq(getLatSpacing(), dest->getLatSpacing()) ||
    !Arith::feq(getLonSpacing(), dest->getLonSpacing()))
  {
    fLogSevere("BlockCopyInto failed: Resolution mismatch. Expected spacing ({}, {}), got ({}, {})",
      dest->getLatSpacing(), dest->getLonSpacing(), getLatSpacing(), getLonSpacing());
    return false;
  }

  auto& srcData = getFloat2DRef();
  auto& dstData = dest->getFloat2DRef();

  const size_t srcRows = getNumLats();
  const size_t srcCols = getNumLons();
  const size_t dstRows = dest->getNumLats();
  const size_t dstCols = dest->getNumLons();

  // 2. Calculate destination starting indices based on coordinate offsets
  int startDestY =
    std::round((dest->getLocation().getLatitudeDeg() - getLocation().getLatitudeDeg()) / getLatSpacing());
  int startDestX = std::round(
    (getLocation().getLongitudeDeg() - dest->getLocation().getLongitudeDeg()) / getLonSpacing());

  // 3. Validate Origin Bounds
  if ((startDestY < 0) || (startDestX < 0) ||
    (startDestY >= static_cast<int>(dstRows)) ||
    (startDestX >= static_cast<int>(dstCols)))
  {
    fLogSevere(
      "BlockCopyInto failed: Origin out of bounds. Calculated start indices: (X:{}, Y:{}) against grid size (X:{}, Y:{})",
      startDestX, startDestY, dstCols, dstRows);
    return false;
  }

  // 4. Tightly loop over the smaller source tile bounds
  for (size_t y = 0; y < srcRows; ++y) {
    size_t destY = startDestY + y;
    if (destY >= dstRows) { continue; }

    for (size_t x = 0; x < srcCols; ++x) {
      size_t destX = startDestX + x;
      if (destX >= dstCols) { continue; }

      float val = srcData[y][x];
      if (val != Constants::DataUnavailable) {
        dstData[destY][destX] = val;
      }
    }
  }
  return true;
} // LatLonGrid::BlockCopyInto

#endif // if 0

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
