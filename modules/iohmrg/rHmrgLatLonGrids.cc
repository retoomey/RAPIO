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
  gzFile fp = IOHmrg::keyToGZFile(keys);

  if (fp != nullptr) {
    int dataYear;
    try{
      dataYear = std::atoi(keys["DataYear"].c_str());
    }catch (...) {
      dataYear = Time().getYear();
    }
    return readLatLonGrids(fp, dataYear, true);
  } else {
    LogSevere("Invalid gzfile pointer, cannot read\n");
  }
  return nullptr;
}

bool
HmrgLatLonGrids::write(
  std::shared_ptr<DataType>         dt,
  std::map<std::string, std::string>& keys)
{
  bool success = false;
  gzFile fp    = IOHmrg::keyToGZFile(keys);

  if (fp != nullptr) {
    auto latlonarea = std::dynamic_pointer_cast<LatLonArea>(dt);
    if (latlonarea != nullptr) {
      success = writeLatLonGrids(fp, latlonarea);
    }
  } else {
    LogSevere("Invalid gzfile pointer, cannot write\n");
  }

  return success;
}

std::shared_ptr<DataType>
HmrgLatLonGrids::readLatLonGrids(gzFile fp, const int year, bool debug)
{
  // Time
  Time time = BinaryIO::readTime(fp, year);

  // Dimensions
  const int num_x = BinaryIO::readInt(fp); // 25-28
  const int num_y = BinaryIO::readInt(fp); // 29-32
  const int num_z = BinaryIO::readInt(fp); // 33-36

  // Projection
  const std::string projection = BinaryIO::readChar(fp, 4); // 37-40
  // Perhaps LL is the only one actually used, we can warn on others though
  // "    " proj1=0;
  // "PS  " proj1=1;
  // "LAMB" proj1=2;
  // "MERC" proj1=3;
  // "LL  " proj1=4;

  const int map_scale    = BinaryIO::readInt(fp);                  // 41-44
  const float lat1       = BinaryIO::readScaledInt(fp, map_scale); // 45-48
  const float lat2       = BinaryIO::readScaledInt(fp, map_scale); // 49-52
  const float lon        = BinaryIO::readScaledInt(fp, map_scale); // 53-56
  const float lonNWDegs1 = BinaryIO::readScaledInt(fp, map_scale); // 57-60
  const float latNWDegs1 = BinaryIO::readScaledInt(fp, map_scale); // 61-64

  // Manually scale since scale after the values
  const int xy_scale         = BinaryIO::readInt(fp); // 65-68 Deprecated, used anywhere?
  const int temp1            = BinaryIO::readInt(fp); // 69-72
  const int temp2            = BinaryIO::readInt(fp); // 73-76
  const int dxy_scale        = BinaryIO::readInt(fp); // 77-80
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
  gzread(fp, &levels[0], num_z * sizeof(int));

  const int z_scale = BinaryIO::readInt(fp);
  std::vector<float> heightMeters;

  heightMeters.resize(num_z);
  for (size_t i = 0; i < levels.size(); i++) {
    heightMeters[i] = (float) levels[i] / (float) (z_scale);
  }

  // Read the placeholder array
  std::vector<int> placeholder;

  placeholder.resize(10);
  gzread(fp, &placeholder[0], 10 * sizeof(int));

  // Get the variable name and units
  std::string varName = BinaryIO::readChar(fp, 20);
  std::string varUnit = BinaryIO::readChar(fp, 6);

  // Convert HMRG names etc to w2 expected
  std::string orgName = varName;

  IOHmrg::HmrgToW2Name(varName, varName);
  LogDebug("Convert: " << orgName << " to " << varName << "\n");

  // Common code here with Radial
  // Scale for scaling data values
  int dataScale = BinaryIO::readInt(fp);

  if (dataScale == 0) {
    LogSevere("Data scale in hmrg is zero, forcing to 1.  Is data corrupt?\n");
    dataScale = 1;
  }

  const int dataMissingValue = BinaryIO::readInt(fp);

  // Read number and names of contributing radars
  int numRadars = BinaryIO::readInt(fp);
  std::vector<std::string> radars;

  for (size_t i = 0; i < numRadars; i++) {
    std::string r = BinaryIO::readChar(fp, 4);
    radars.push_back(r);
  }

  // Common code here with Radial
  std::vector<short int> rawBuffer;

  // FIXME: will probably remove at some point
  if (debug) {
    for (size_t i = 0; i < numRadars; i++) {
      LogDebug("   Got radar '" << radars[i] << "'\n");
    }
    LogDebug("   LatLons: " << lonSpacingDegs << ", " << latSpacingDegs << ", " << lonNWDegs1 << ", "
                            << latNWDegs1 << ", " << lonNWDegs << ", " << latNWDegs << "\n");
    LogDebug("   Date: " << time.getString("%Y %m %d %H %M %S") << "\n");
    LogDebug("   Time: " << time << "\n");
    LogDebug("   Dimensions: " << num_x << " " << num_y << " " << num_z << "\n");
    LogDebug("   Projection: '" << projection << "'\n"); // always "LL  "?
    LogDebug(
      "   Lat, lat2, lon: " << lat1 << ", " << lat2 << ", " << lon << ", nw:" << latNWDegs1 << ", " << lonNWDegs1 <<
        "\n");
    LogDebug("   VarNameUnit: " << varName << ", " << varUnit << "\n");
    LogDebug("   VarScale/Missing: " << dataScale << " " << dataMissingValue << "\n");
    if (heightMeters.size() > 0) {
      LogDebug("  OK height Z at 0 is " << heightMeters[0] << "\n");
      LogDebug("  Spacing is " << latSpacingDegs << ", " << lonSpacingDegs << "\n");
    }
  }

  LLH location(latNWDegs, lonNWDegs, heightMeters[0] / 1000.0); // takes kilometers...

  // Think using the missing scaled up will help prevent float drift here
  // and avoid some divisions in loop
  const int dataMissing     = dataMissingValue * dataScale;
  const int dataUnavailable = -9990; // FIXME: table lookup * dataScale;

  // Read in the data from file
  int count = num_x * num_y * num_z;

  rawBuffer.resize(count);
  ERRNO(gzread(fp, &rawBuffer[0], count * sizeof(short int))); // should be 2 bytes, little endian order

  // Handle single layer
  size_t at = 0;

  if (num_z == 1) {
    LogInfo("HMRG reader: --Single layer LatLonGrid--\n");

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
    // LogInfo("    Found " << countm << " missing values\n");
    return latLonGridSP;

    // Z > 1 create a LatLonHeightGrid
  } else {
    LogInfo("HMRG reader: --Multi layer LatLonGrid--\n");

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
    auto gridLayers = grid.getLayerValues();
    for (size_t i = 0; i < gridLayers.size(); i++) {
      gridLayers[i] = heightMeters[i];
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
    LogInfo(">>Finished reading full LatLonHeightGrid\n");

    return latLonHeightGridSP;
  }

  return nullptr;
} // IOHmrg::readLatLonGrid

bool
HmrgLatLonGrids::writeLatLonGrids(gzFile fp, std::shared_ptr<LatLonArea> llgp)
{
  bool success = false;

  // FIXME: Get from lookup table.  I think lookup table needs refactor at some point
  // We could also use keys to pass these down
  const int dataMissingValue = -99;
  const int dataScale        = 10;
  const int map_scale        = 1000;
  const int z_scale = 1;
  // FIXME: Not 100% sure what these represent, we'll tweak later
  const float lat1 = 30;      // FIXME: what is this?  Calculate these properly
  const float lat2 = 60;      // FIXME: what is this?
  const float lon  = -60.005; // FIXME: what is this?

  // Values appear fixed, not sure they matter much, they only compress a couple values
  const int xy_scale  = 1000; // Not used?
  const int dxy_scale = 100000;

  auto& llg = *llgp;

  // Time
  BinaryIO::writeTime(fp, llg.getTime());

  // Dimensions
  const int num_y = llg.getNumLats();
  const int num_x = llg.getNumLons();
  const int num_z = llg.getNumLayers(); // LLG is 1, LLHG can be 1 or more

  BinaryIO::writeInt(fp, num_x);
  BinaryIO::writeInt(fp, num_y);
  BinaryIO::writeInt(fp, num_z);

  // Projection
  const std::string projection = "LL  ";

  BinaryIO::writeChar(fp, projection, 4);

  // Calculate scaled lat/lon spacing values
  const float lonSpacingDegs = llg.getLonSpacing();
  const float latSpacingDegs = llg.getLatSpacing();
  const auto aLoc       = llg.getTopLeftLocationAt(0, 0);
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

  BinaryIO::writeInt(fp, map_scale);
  BinaryIO::writeScaledInt(fp, lat1, map_scale);
  BinaryIO::writeScaledInt(fp, lat2, map_scale);
  BinaryIO::writeScaledInt(fp, lon, map_scale);
  BinaryIO::writeScaledInt(fp, lonNWDegs1, map_scale);
  BinaryIO::writeScaledInt(fp, latNWDegs1, map_scale);

  BinaryIO::writeInt(fp, xy_scale);
  BinaryIO::writeInt(fp, temp1);
  BinaryIO::writeInt(fp, temp2);
  BinaryIO::writeInt(fp, dxy_scale);

  // Write the height levels, scaled by Z_scale
  auto& heights = llg.getLayerValues();

  for (size_t h = 0; h < heights.size(); h++) {
    float aHeightMeters = heights[h];
    BinaryIO::writeInt(fp, aHeightMeters * z_scale);
  }
  BinaryIO::writeInt(fp, z_scale);

  // Write the placeholder array
  std::vector<int> placeholder;

  placeholder.resize(10);
  ERRNO(gzwrite(fp, &placeholder[0], 10 * sizeof(int)));

  // Convert W2 names to HMRG names
  std::string typeName = llg.getTypeName();
  std::string name     = typeName;

  IOHmrg::W2ToHmrgName(name, name);

  // Write the variable name and units
  std::string varUnit = llg.getUnits();

  BinaryIO::writeChar(fp, name, 20);
  BinaryIO::writeChar(fp, varUnit, 6);

  BinaryIO::writeInt(fp, dataScale);

  BinaryIO::writeInt(fp, dataMissingValue);

  LogDebug("Convert: " << typeName << " to " << name << "\n");
  LogDebug("Output units is " << varUnit << "\n");
  LogDebug("   LatLons: " << lonSpacingDegs << ", " << latSpacingDegs << ", " << lonNWDegs1 << ", "
                          << latNWDegs1 << ", " << lonNWDegs << ", " << latNWDegs << "\n");

  // Scaled versions
  const int dataMissing = dataMissingValue * dataScale;

  const int dataUnavailable = -9990; // FIXME: table lookup * dataScale;

  // Write number and names of contributing radars
  // Leaving this none for now.  We could use attributes in the LatLonGrid
  // to store this on a read to perserve anything from incoming mrms binary files
  // Also, W2 netcdf could use this ability to be honest
  BinaryIO::writeInt(fp, 1);
  std::string radar = "none";

  BinaryIO::writeChar(fp, radar, 4);

  // Write in the data from LatLonGrid
  // For now convert which eats some ram for big stuff.  I think we could
  // add the mrms binary ability to our LatLonGrid data structure to compress ram/disk
  int count = num_x * num_y * num_z;

  if (count == 0) {
    return true; // didn't write anything
  }

  // FIXME: Make configurable. Alpha: Add write block ability due to having to write pretty
  // large grids, and this avoids doubling RAM usage. Well have to test this more
  // FIXME: Make a buffer object stream to hide all these details I think.  The only issue
  // is it would require one extra jump per item.  Something like:
  // hmrgOutBuffer(fp, maxsize); buff << value; buff.finalize();
  std::vector<short int> rawBuffer;
  const size_t maxItems = 524288; // Max items to write at once, here about 1 MB per 500K items

  if (count > maxItems) {
    count = maxItems; // trim buffer
  }
  rawBuffer.resize(count); // Up to maxItems *size(short int)

  size_t at = 0;

  // Various classes we support writing. FIXME: Should probably use specializer API and
  // break up this code that way instead.

  if (auto llgptr = std::dynamic_pointer_cast<LatLonGrid>(llgp)) {
    LogInfo("HMRG writer: --LatLonGrid--\n");
    auto& data = llg.getFloat2DRef(Constants::PrimaryDataName);

    // NOTE: flipped order from RadialSet array if you try to merge the code
    for (size_t j = num_y - 1; j != SIZE_MAX; --j) {
      for (size_t i = 0; i < num_x; ++i) { // row order for the data, so read in order
        rawBuffer[at] = IOHmrg::toHmrgValue(data[j][i], dataUnavailable, dataMissing, dataScale);
        if (++at >= count) {
          ERRNO(gzwrite(fp, &rawBuffer[0], count * sizeof(short int)));
          at = 0;
        }
      }
    }
    if (at != 0) { // final left over
      ERRNO(gzwrite(fp, &rawBuffer[0], at * sizeof(short int)));
    }
    success = true;
  } else if (auto llnptr = std::dynamic_pointer_cast<LLHGridN2D>(llgp)) {
    LogInfo("HMRG writer: --Multi layer N 2D layers (LLHGridN2D)--\n");
    auto& lln = *llnptr;

    for (size_t z = 0; z < num_z; ++z) {
      // Each 3D is a 2N layer here
      auto g     = lln.get(z);
      auto& data = g->getFloat2DRef();

      for (size_t j = num_y - 1; j != SIZE_MAX; --j) {
        for (size_t i = 0; i < num_x; ++i) { // row order for the data, so read in order
          rawBuffer[at] = IOHmrg::toHmrgValue(data[j][i], dataUnavailable, dataMissing, dataScale);
          if (++at >= count) {
            ERRNO(gzwrite(fp, &rawBuffer[0], count * sizeof(short int)));
            at = 0;
          }
        }
      }
    }
    if (at != 0) { // final left over
      ERRNO(gzwrite(fp, &rawBuffer[0], at * sizeof(short int)));
    }
    success = true;
  } else if (auto llhgptr = std::dynamic_pointer_cast<LatLonHeightGrid>(llgp)) {
    LogInfo("HMRG writer: --Multi layer LatLonArea--\n");

    // Only for the 3D implementation
    auto& data = llg.getFloat3DRef(Constants::PrimaryDataName);

    for (size_t z = 0; z < num_z; ++z) {
      // NOTE: flipped order from RadialSet array if you try to merge the code
      // Same code as 2D though the data array type is different.  Could use a template method or macro
      for (size_t j = num_y - 1; j != SIZE_MAX; --j) {
        for (size_t i = 0; i < num_x; ++i) { // row order for the data, so read in order
          rawBuffer[at] = IOHmrg::toHmrgValue(data[z][j][i], dataUnavailable, dataMissing, dataScale);
          if (++at >= count) {
            ERRNO(gzwrite(fp, &rawBuffer[0], count * sizeof(short int)));
            at = 0;
          }
        }
      }
    }
    if (at != 0) { // final left over
      ERRNO(gzwrite(fp, &rawBuffer[0], at * sizeof(short int)));
    }
    success = true;
  } else {
    LogSevere("HMRG: LatLonArea unsupported type, can't write this\n");
  }

  return success;
} // HmrgLatLonGrids::writeLatLonGrids
