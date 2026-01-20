#include "rHmrgLatLonGrids.h"

#include "rOS.h"
#include "rLatLonGrid.h"
#include "rLatLonHeightGrid.h"
#include "rLLHGridN2D.h"
#include "rBinaryIO.h"

using namespace rapio;

void
HmrgLatLonGrids::introduceSelf(IOHmrg * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<HmrgLatLonGrids>();

  // DataTypes we handle
  owner->introduce("LatLonGrid", io);
  owner->introduce("SparseLatLonGrid", io);

  // We handle 3D directly as well
  owner->introduce("LatLonHeightGrid", io);
  owner->introduce("SparseLatLonHeightGrid", io);
}

std::shared_ptr<DataType>
HmrgLatLonGrids::read(
  std::map<std::string, std::string>& keys,
  std::shared_ptr<DataType>         dt)
{
  StreamBuffer * g = IOHmrg::keyToStreamBuffer(keys);

  if (g != nullptr) {
    int dataYear;
    try{
      dataYear = std::atoi(keys["DataYear"].c_str());
    }catch (...) {
      dataYear = Time().getYear();
    }
    return readLatLonGrids(*g, dataYear);
  } else {
    fLogSevere("Invalid stream buffer pointer, cannot read");
  }
  return nullptr;
}

bool
HmrgLatLonGrids::write(
  std::shared_ptr<DataType>         dt,
  std::map<std::string, std::string>& keys)
{
  bool success     = false;
  StreamBuffer * g = IOHmrg::keyToStreamBuffer(keys);

  if (g != nullptr) {
    auto latlonarea = std::dynamic_pointer_cast<LatLonArea>(dt);
    if (latlonarea != nullptr) {
      success = writeLatLonGrids(*g, latlonarea);
    }
  } else {
    fLogSevere("Invalid stream buffer pointer, cannot write");
  }

  return success;
}

std::shared_ptr<DataType>
HmrgLatLonGrids::readLatLonGrids(StreamBuffer& g, const int year)
{
  // Time
  Time time = g.readTime(year);

  // Dimensions
  const int num_x = g.readInt(); // 25-28
  const int num_y = g.readInt(); // 29-32
  const int num_z = g.readInt(); // 33-36

  // Projection
  const std::string projection = g.readString(4); // 37-40
  // Perhaps LL is the only one actually used, we can warn on others though
  // "    " proj1=0;
  // "PS  " proj1=1;
  // "LAMB" proj1=2;
  // "MERC" proj1=3;
  // "LL  " proj1=4;

  const int map_scale    = g.readInt();                // 41-44
  const float lat1       = g.readScaledInt(map_scale); // 45-48
  const float lat2       = g.readScaledInt(map_scale); // 49-52
  const float lon        = g.readScaledInt(map_scale); // 53-56
  const float lonNWDegs1 = g.readScaledInt(map_scale); // 57-60
  const float latNWDegs1 = g.readScaledInt(map_scale); // 61-64

  // Manually scale since scale after the values
  const int xy_scale         = g.readInt(); // 65-68 Deprecated, used anywhere?
  const int temp1            = g.readInt(); // 69-72
  const int temp2            = g.readInt(); // 73-76
  const int dxy_scale        = g.readInt(); // 77-80
  const float lonSpacingDegs = (float) temp1 / (float) dxy_scale;
  const float latSpacingDegs = (float) temp2 / (float) dxy_scale;
  // HMRG uses center of cell for northwest corner, while we use the actual northwest corner
  // or actual top left of the grid cell.  We need to adjust using lat/lon spacing...so
  // that 'should' be just shifting NorthWest by half of the deg spacing, which means
  //  W2/RAPIO lon = - (.5*HMRG lon), W2/RAPIO lat = + (.5*HMRG lat)
  const float lonNWDegs = lonNWDegs1 - (.5 * lonSpacingDegs);
  const float latNWDegs = latNWDegs1 + (.5 * latSpacingDegs);

  // Read the height levels, scaled by Z_scale
  std::vector<int> levels;

  levels.resize(num_z);
  g.readVector(&levels[0], num_z * sizeof(int));

  const int z_scale = g.readInt();
  std::vector<float> heightMeters;

  heightMeters.resize(num_z);
  for (size_t i = 0; i < levels.size(); i++) {
    heightMeters[i] = (float) levels[i] / (float) (z_scale);
  }

  // Read the placeholder array
  std::vector<int> placeholder;

  placeholder.resize(10);
  g.readVector(&placeholder[0], 10 * sizeof(int));

  // Get the variable name and units
  std::string varName = g.readString(20);
  std::string varUnit = g.readString(6);

  // Convert HMRG names etc to w2 expected
  std::string orgName = varName;

  IOHmrg::HmrgToW2Name(varName, varName);
  fLogDebug("Convert: {} to {}", orgName, varName);

  // Common code here with Radial
  // Scale for scaling data values
  int dataScale = g.readInt();

  if (dataScale == 0) {
    fLogSevere("Data scale in hmrg is zero, forcing to 1.  Is data corrupt?");
    dataScale = 1;
  }

  const int dataMissingValue = g.readInt();

  // Read number and names of contributing radars
  int numRadars = g.readInt();
  std::vector<std::string> radars;

  for (size_t i = 0; i < numRadars; i++) {
    std::string r = g.readString(4);
    radars.push_back(r);
  }

  // Common code here with Radial
  std::vector<short int> rawBuffer;

  #if 0
  for (size_t i = 0; i < numRadars; i++) {
    fLogDebug("   Got radar '{}'", radars[i]);
  }
  fLogDebug("   LatLons: {}, {}, {}, {}, {}, {}",
    lonSpacingDegs, latSpacingDegs, lonNWDegs1, latNWDegs1, lonNWDegs, latNWDegs);
  fLogDebug("   Date: {}", << time.getString("%Y %m %d %H %M %S"));
  fLogDebug("   Time: ", time);
  fLogDebug("   Dimensions: {} {} {}", num_x, num_y, num_z);
  fLogDebug("   Projection: '{}'", projection); // always "LL  "?
  fLogDebug("   Lat, lat2, lon: {}, {}, {}, nw:{}, {}", lat1, lat2, lon, latNWDegs1, lonNWDegs1);
  fLogDebug("   VarNameUnit: {}, {}", varName, varUnit);
  fLogDebug("   VarScale/Missing: {} {}", dataScale, dataMissingValue);
  if (heightMeters.size() > 0) {
    fLogDebug("  OK height Z at 0 is {}", heightMeters[0]);
    fLogDebug("  Spacing is {}, {}", latSpacingDegs, lonSpacingDegs);
  }
} // HmrgLatLonGrids::readLatLonGrids

  #endif // if 0

  LLH location(latNWDegs, lonNWDegs, heightMeters[0] / 1000.0); // takes kilometers...

  // Think using the missing scaled up will help prevent float drift here
  // and avoid some divisions in loop
  const int dataMissing     = dataMissingValue * dataScale;
  const int dataUnavailable = -9990; // FIXME: table lookup * dataScale;

  // Read in the data from file
  int count = num_x * num_y * num_z;

  rawBuffer.resize(count);
  g.readVector(&rawBuffer[0], count * sizeof(short int)); // FIXME: vector doesn't check endian right?

  // Handle single layer
  size_t at = 0;

  if (num_z == 1) {
    fLogInfo("HMRG reader: --Single layer LatLonGrid--");

    // Create a LatLonGrid using the data
    auto latLonGridSP = LatLonGrid::Create(
      varName,
      varUnit,
      location, // using height 0 as location
      time,
      latSpacingDegs,
      lonSpacingDegs,
      num_y, // num_lats
      num_x  // num_lons
    );

    LatLonGrid& grid = *latLonGridSP;
    grid.setReadFactory("netcdf"); // Default would call us to write

    // Grid 2d primary
    auto array = grid.getFloat2D(Constants::PrimaryDataName);
    auto& data = array->ref();

    // NOTE: flipped order from RadialSet array if you try to merge the code
    for (size_t j = 0; j < num_y; ++j) {
      const size_t jflip = num_y - j - 1;
      for (size_t i = 0; i < num_x; ++i) { // row order for the data, so read in order
        data[jflip][i] =
          IOHmrg::fromHmrgValue(rawBuffer[at++], dataUnavailable, dataMissing, dataScale);
      }
    }
    // fLogInfo("    Found {} missing values", countm);
    return latLonGridSP;

    // Z > 1 create a LatLonHeightGrid
  } else {
    fLogInfo("HMRG reader: --Multi layer LatLonGrid--");

    // Create a LatLonGrid using the data
    auto latLonHeightGridSP = LatLonHeightGrid::Create(
      varName,
      varUnit,
      location, // using height 0 as location
      time,
      latSpacingDegs,
      lonSpacingDegs,
      num_y, // num_lats
      num_x, // num_lons
      num_z  // num layers
    );

    LatLonHeightGrid& grid = *latLonHeightGridSP;
    grid.setReadFactory("netcdf"); // Default would call us to write

    // Heights are stored in the layers
    const auto layerSize = grid.getNumLayers();
    for (size_t i = 0; i < layerSize; i++) {
      grid.setLayerValue(i, heightMeters[i]);
    }

    // This gonna be slower than W2 because the data ordering is different
    // so we can't just directly push into memory.
    auto array = grid.getFloat3D(Constants::PrimaryDataName);
    auto& data = array->ref();
    for (size_t z = 0; z < num_z; ++z) {
      for (size_t j = 0; j < num_y; ++j) {
        const size_t jflip = num_y - j - 1;
        for (size_t i = 0; i < num_x; ++i) { // row order for the data, so read in order
          data[z][jflip][i] = IOHmrg::fromHmrgValue(rawBuffer[at++], dataUnavailable,
              dataMissing,
              dataScale);
        }
      }
    }
    fLogInfo(">>Finished reading full LatLonHeightGrid");

    return latLonHeightGridSP;
  }

  return nullptr;
} // IOHmrg::readLatLonGrid

bool
HmrgLatLonGrids::writeLatLonGrids(StreamBuffer& g, std::shared_ptr<LatLonArea> llgp)
{
  bool success = false;
  auto& llg = *llgp;

  // ------------------------------------------------------------------
  // Lookup table information to get scaling, etc.
  // and convert W2 to MRMS binary fields
  // This pulls from the table, but in theory we 'should' make this in
  // out parameter map to allow writers to manually override.
  // FIXME: become a function probably. Shared with radial set
  std::string units = llg.getUnits();
  std::string typeName = llg.getTypeName();
  std::string name = typeName; // name used

  // Defaults if missing in table
  int dataMissingValue = -99;
  int dataNCValue = -999;
  int dataScale = 10;

  // --------------------------
  // FIXME: Not 100% sure what these represent, we'll tweak later
  const int map_scale = 1000;
  const int z_scale = 1;
  const float lat1 = 30;     // FIXME: what is this?  Calculate these properly
  const float lat2 = 60;     // FIXME: what is this?
  const float lon = -60.005; // FIXME: what is this?

  // Values appear fixed, not sure they matter much, they only compress a couple values
  const int xy_scale = 1000; // Not used?
  const int dxy_scale = 100000;
  // --------------------------

  ProductInfo * pi = IOHmrg::getProductInfo(typeName, units);

  if (pi != nullptr) {
    fLogInfo("(Found) MRMS binary product info for {}/{}", typeName, units);
    name = pi->varName;
    units = pi->varUnit;
    dataMissingValue = pi->varMissing;
    dataNCValue = pi->varNoCoverage;
    dataScale = pi->varScale;
    // FIXME: use the w2missing and w2unavailable table values for replace?
  } else {
    fLogInfo("No mrms binary product info for {}, using defaults", typeName);
  }
  // End field conversion
  // ------------------------------------------------------------------

  // Time
  g.writeTime(llg.getTime());

  // Dimensions
  const int num_y = llg.getNumLats();
  const int num_x = llg.getNumLons();
  const int num_z = llg.getNumLayers(); // LLG is 1, LLHG can be 1 or more

  g.writeInt(num_x);
  g.writeInt(num_y);
  g.writeInt(num_z);

  // Projection
  const std::string projection = "LL  ";

  g.writeString(projection, 4);

  // Calculate scaled lat/lon spacing values
  const float lonSpacingDegs = llg.getLonSpacing();
  const float latSpacingDegs = llg.getLatSpacing();
  const auto aLoc = llg.getTopLeftLocationAt(0, 0);
  const float lonNWDegs = aLoc.getLongitudeDeg();
  const float latNWDegs = aLoc.getLatitudeDeg();

  const int temp1 = lonSpacingDegs * dxy_scale;
  const int temp2 = latSpacingDegs * dxy_scale;
  // Shift NW lat/lon to match hmrg convention
  // NW grid cell coord in hmrg references the center of the cell
  // NW grid cell coord in W2 references the upper left-hand corner
  // So, shift lat and lon to the South and East by half a grid cell
  const float lonNWDegs1 = lonNWDegs + (.5 * lonSpacingDegs); // -94.995;
  const float latNWDegs1 = latNWDegs - (.5 * latSpacingDegs); // 37.495;

  g.writeInt(map_scale);
  g.writeScaledInt(lat1, map_scale);
  g.writeScaledInt(lat2, map_scale);
  g.writeScaledInt(lon, map_scale);
  g.writeScaledInt(lonNWDegs1, map_scale);
  g.writeScaledInt(latNWDegs1, map_scale);

  g.writeInt(xy_scale);
  g.writeInt(temp1);
  g.writeInt(temp2);
  g.writeInt(dxy_scale);

  const auto layerSize = llg.getNumLayers();

  for (size_t h = 0; h < layerSize; h++) {
    float aHeightMeters = llg.getLayerValue(h);
    // FIXME: Humm aren't these just scaled int writes?
    g.writeInt(aHeightMeters * z_scale);
    // g.writeScaledInt(aHeightMeters, z_scale);
  }
  g.writeInt(z_scale);

  // Write the placeholder array of 10
  // I think we could use this to 'extend' the format if wanted
  for (size_t p = 0; p < 10; ++p) {
    g.writeInt(0);
  }

  // Write the variable name and units
  g.writeString(name, 20);
  g.writeString(units, 6);
  g.writeInt(dataScale);
  g.writeInt(dataMissingValue);

  #if 0
  fLogDebug("Convert: {} to {}", typeName, name);
  fLogDebug("Output units is {}", units);
  fLogDebug("   LatLons: {}, {}, {}, {}, {}, {}",
    lonSpacingDegs, latSpacingDegs, lonNWDegs1, latNWDegs1, lonNWDegs, latNWDegs);
  #endif

  // Write number and names of contributing radars
  // Leaving this none for now.  We could use attributes in the LatLonGrid
  // to store this on a read to perserve anything from incoming mrms binary files
  // Also, W2 netcdf could use this ability to be honest
  g.writeInt(1);
  std::string radar = "none";

  g.writeString(radar, 4);

  // Write in the data from LatLonGrid
  // For now convert which eats some ram for big stuff.  I think we could
  // add the mrms binary ability to our LatLonGrid data structure to compress ram/disk
  int count = num_x * num_y * num_z;

  if (count == 0) {
    return true; // didn't write anything
  }

  // FIXME: Make configurable. Alpha: Add write block ability due to having to write pretty
  // large grids, and this avoids doubling RAM usage. Well have to test this more
  std::vector<short int> rawBuffer;
  const size_t maxItems = 524288; // Max items to write at once, here about 1 MB per 500K items

  if (count > maxItems) {
    count = maxItems; // trim buffer
  }
  rawBuffer.resize(count); // Up to maxItems *size(short int)

  size_t at = 0;

  // Various classes we support writing. FIXME: Should probably use specializer API and
  // break up this code that way instead.

  const int dataMissing = dataMissingValue * dataScale; // prescaled for speed
  const int dataUnavailable = dataNCValue * dataScale;  // prescaled for speed

  if (auto llgptr = std::dynamic_pointer_cast<LatLonGrid>(llgp)) {
    fLogInfo("HMRG writer: --LatLonGrid--");
    auto& data = llg.getFloat2DRef(Constants::PrimaryDataName);

    // NOTE: flipped order from RadialSet array if you try to merge the code
    for (size_t j = num_y - 1; j != SIZE_MAX; --j) {
      for (size_t i = 0; i < num_x; ++i) { // row order for the data, so read in order
        rawBuffer[at] = IOHmrg::toHmrgValue(data[j][i], dataUnavailable, dataMissing, dataScale);
        if (++at >= count) {
          g.writeVector(rawBuffer.data(), count * sizeof(short int));
          at = 0;
        }
      }
    }
    if (at != 0) { // final left over
      g.writeVector(rawBuffer.data(), at * sizeof(short int));
    }
    success = true;
  } else if (auto llnptr = std::dynamic_pointer_cast<LLHGridN2D>(llgp)) {
    fLogInfo("HMRG writer: --Multi layer N 2D layers (LLHGridN2D)--");
    auto& lln = *llnptr;

    for (size_t z = 0; z < num_z; ++z) {
      // Each 3D is a 2N layer here
      auto llg = lln.get(z);
      auto& data = llg->getFloat2DRef();

      for (size_t j = num_y - 1; j != SIZE_MAX; --j) {
        for (size_t i = 0; i < num_x; ++i) { // row order for the data, so read in order
          rawBuffer[at] = IOHmrg::toHmrgValue(data[j][i], dataUnavailable, dataMissing, dataScale);
          if (++at >= count) {
            g.writeVector(rawBuffer.data(), count * sizeof(short int));
            at = 0;
          }
        }
      }
    }
    if (at != 0) { // final left over
      g.writeVector(rawBuffer.data(), at * sizeof(short int));
    }
    success = true;
  } else if (auto llhgptr = std::dynamic_pointer_cast<LatLonHeightGrid>(llgp)) {
    fLogInfo("HMRG writer: --Multi layer LatLonArea--");

    // Only for the 3D implementation
    auto& data = llg.getFloat3DRef(Constants::PrimaryDataName);

    for (size_t z = 0; z < num_z; ++z) {
      // NOTE: flipped order from RadialSet array if you try to merge the code
      // Same code as 2D though the data array type is different.  Could use a template method or macro
      for (size_t j = num_y - 1; j != SIZE_MAX; --j) {
        for (size_t i = 0; i < num_x; ++i) { // row order for the data, so read in order
          rawBuffer[at] = IOHmrg::toHmrgValue(data[z][j][i], dataUnavailable, dataMissing, dataScale);
          if (++at >= count) {
            g.writeVector(rawBuffer.data(), count * sizeof(short int));
            at = 0;
          }
        }
      }
    }
    if (at != 0) { // final left over
      g.writeVector(rawBuffer.data(), at * sizeof(short int));
    }
    success = true;
  } else {
    fLogSevere("HMRG: LatLonArea unsupported type, can't write this");
  }

  return success;
} // HmrgLatLonGrids::writeLatLonGrids
