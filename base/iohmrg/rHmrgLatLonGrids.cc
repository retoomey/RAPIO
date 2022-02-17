#include "rHmrgLatLonGrids.h"

#include "rOS.h"
#include "rLatLonGrid.h"
#include "rLatLonHeightGrid.h"

using namespace rapio;

std::shared_ptr<DataType>
IOHmrgLatLonGrids::readLatLonGrids(gzFile fp, const int year, bool debug)
{
  // Timestamp
  // h.year passed in;    // 1-4
  const int month = readInt(fp); // 5-8
  const int day   = readInt(fp); // 9-12
  const int hour  = readInt(fp); // 13-16
  const int min   = readInt(fp); // 17-20
  const int sec   = readInt(fp); // 21-24

  // RAPIO time handles this
  Time time(year, month, day, hour, min, sec, 0.0);

  // Dimensions
  const int num_x = readInt(fp); // 25-28
  const int num_y = readInt(fp); // 29-32
  const int num_z = readInt(fp); // 33-36

  const std::string projection = readChar(fp, 4); // 37-40
  // Perhaps LL is the only one actually used, we can warn on others though
  // "    " proj1=0;
  // "PS  " proj1=1;
  // "LAMB" proj1=2;
  // "MERC" proj1=3;
  // "LL  " proj1=4;

  const int map_scale    = readInt(fp);                  // 41-44
  const float lat1       = readScaledInt(fp, map_scale); // 45-48
  const float lat2       = readScaledInt(fp, map_scale); // 49-52
  const float lon        = readScaledInt(fp, map_scale); // 53-56
  const float lonNWDegs1 = readScaledInt(fp, map_scale); // 57-60
  const float latNWDegs1 = readScaledInt(fp, map_scale); // 61-64

  // Manually scale since scale after the values
  const int xy_scale         = readInt(fp); // 65-68 Deprecated, used anywhere?
  const int temp1            = readInt(fp); // 69-72
  const int temp2            = readInt(fp); // 73-76
  const int dxy_scale        = readInt(fp); // 77-80
  const float lonSpacingDegs = (float) temp1 / (float) dxy_scale;
  const float latSpacingDegs = (float) temp2 / (float) dxy_scale;
  // HMRG uses center of cell for northwest corner, while we use the actual northwest corner
  // or actual top left of the grid cell.  We need to adjust using lat/lon spacing...so
  // that 'should' be just shifting NorthWest by half of the deg spacing, which means
  //  W2/RAPIO lon = + (.5*HMRG lon), W2/RAPIO lat = - (.5*HMRG lat)
  const float lonNWDegs = lonNWDegs1 + .5 * (lonSpacingDegs);
  const float latNWDegs = latNWDegs1 - .5 * (latSpacingDegs);

  // Read the height levels, scaled by Z_scale
  std::vector<int> levels;

  levels.resize(num_z);
  gzread(fp, &levels[0], num_z * sizeof(int));

  const int z_scale = readInt(fp);
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
  std::string varName = readChar(fp, 20);
  std::string varUnit = readChar(fp, 6);

  // Convert HMRG names etc to w2 expected
  HmrgToW2(varName, varName);
  LogSevere("FINAL VARNAME " << varName << "\n");

  // Common code here with Radial
  // Scale for scaling data values
  int dataScale = readInt(fp);

  if (dataScale == 0) {
    LogSevere("Data scale in hmrg is zero, forcing to 1.  Is data corrupt?\n");
    dataScale = 1;
  }

  const int dataMissingValue = readInt(fp);

  int numRadars = readInt(fp);
  std::vector<std::string> radars;

  for (size_t i = 0; i < numRadars; i++) {
    std::string r = readChar(fp, 4);
    radars.push_back(r);
  }

  // Common code here with Radial
  std::vector<short int> rawBuffer;

  if (debug) {
    for (size_t i = 0; i < numRadars; i++) {
      LogInfo("   Got radar '" << radars[i] << "'\n");
    }
    LogInfo(
      "   Date: " << year << " " << month << " " << day << " " << hour << " " << min << " " << sec << "\n");
    LogInfo("   Time: " << time << "\n");
    LogInfo("   Dimensions: " << num_x << " " << num_y << " " << num_z << "\n");
    LogInfo("   Projection: '" << projection << "'\n"); // always "LL  "?
    LogInfo("   Lat, lat2, lon: " << lat1 << ", " << lat2 << ", " << lon << "\n");
    LogInfo("   VarNameUnit: " << varName << ", " << varUnit << "\n");
    LogInfo("   VarScale/Missing: " << dataScale << " " << dataMissingValue << "\n");
    if (heightMeters.size() > 0) {
      LogInfo("  OK height Z at 0 is " << heightMeters[0] << "\n");
      LogInfo("  Spacing is " << latSpacingDegs << ", " << lonSpacingDegs << "\n");
    }
  }

  LLH location(latNWDegs, lonNWDegs, heightMeters[0] / 1000.0); // takes kilometers...

  // Fill in the data here
  const bool needSwap = OS::isBigEndian(); // data is little
  // size_t countm = 0;
  size_t rawBufferIndex = 0;

  // Think using the missing scaled up will help prevent float drift here
  // and avoid some divisions in loop
  const int dataMissing     = dataMissingValue * dataScale;
  const int dataUnavailable = -9990; // FIXME: table lookup * dataScale;

  // Read in the data from file
  int count = num_x * num_y * num_z;

  rawBuffer.resize(count);
  ERRNO(gzread(fp, &rawBuffer[0], count * sizeof(short int))); // should be 2 bytes, little endian order

  // Handle single layer
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
          convertDataValue(rawBuffer[rawBufferIndex++], needSwap, dataUnavailable, dataMissing, dataScale);
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
          data[jflip][i][z] = convertDataValue(rawBuffer[rawBufferIndex++], needSwap, dataUnavailable, dataMissing,
              dataScale);
        }
      }
    }
    LogInfo(">>Finished reading full LatLonHeightGrid\n");

    return latLonHeightGridSP;
  }

  return nullptr;
} // IOHmrg::readLatLonGrid
