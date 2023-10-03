#include "rFusionCache.h"

#include "rLLCoverageArea.h"

#include <fstream>

using namespace rapio;

void
FusionCache::writeRangeFile(const std::string& filename, LLCoverageArea& outg,
  std::vector<std::shared_ptr<AzRanElevCache> >& myLLProjections,
  LengthKMs maxRangeKMs)
{
  std::ifstream fileExists(filename);

  if (fileExists.good()) {
    LogInfo("Cache file " << filename << " exists, not writing..\n");
    return;
  }
  std::ofstream outFile(filename, std::ios::binary);

  if (!outFile.is_open()) {
    LogSevere("Can't write cache file " << filename << "\n");
    return;
  }

  // FIXME: Could I create an iterator for this?
  // The iterator should keep lat, lon, etc. inline
  // My concern is ordering and ease of breaking here

  // FIXME: general gzip binary file class, we could clean up and compress

  // Header (write the grid out so roster doesn't have to bother recalculating it):
  auto x  = outg.getNumX();
  auto y  = outg.getNumY();
  auto sx = outg.getStartX();
  auto sy = outg.getStartY();

  size_t count = 0;

  outFile.write(reinterpret_cast<char *>(&x), sizeof(x));
  outFile.write(reinterpret_cast<char *>(&y), sizeof(y));
  outFile.write(reinterpret_cast<char *>(&sx), sizeof(sx));
  outFile.write(reinterpret_cast<char *>(&sy), sizeof(sy));

  // F(Lat,Lat,Height) --> Virtual Az, Elev, Range projection add spacing/2 to get cell cellcenters
  const AngleDegs startLat = outg.getNWLat() - (outg.getLatSpacing() / 2.0); // move south (lat decreasing)
  const AngleDegs startLon = outg.getNWLon() + (outg.getLonSpacing() / 2.0); // move east (lon increasing)
  auto heightsKM = outg.getHeightsKM();

  std::vector<FusionRangeCache> buffer;

  for (size_t layer = 0; layer < heightsKM.size(); layer++) {
    auto& llp = *myLLProjections[layer];
    //    auto& ssc = *(mySinCosCache);
    llp.reset();
    //    ssc.reset();

    AngleDegs atLat = startLat;
    for (size_t y = 0; y < outg.getNumY(); y++, atLat -= outg.getLatSpacing()) { // a north to south
      AngleDegs atLon = startLon;
      // Note: The 'range' is 0 to numX always, however the index to global grid is x+out.startX;
      for (size_t x = 0; x < outg.getNumX(); x++, atLon += outg.getLonSpacing()) { // a east to west row, changing lon per cell
        AngleDegs AzDegs, ElevDegs, aLengthKMs;

        // Store as 2 bytes meters for space.  8 bytes is about 128 mb per file currently
        llp.get(AzDegs, ElevDegs, aLengthKMs); // FIXME: Don't need everything

        // FusionRangeCache storedMeters = std::round(aLengthKMs * 1000.0);
        FusionRangeCache storedMeters = aLengthKMs;

        static size_t counter = 0;
        //        outFile.write(reinterpret_cast<char *>(&storedMeters), sizeof(FusionRangeCache));
        buffer.push_back(storedMeters);
        if (counter++ == 1000) {
          LogSevere("VALUE at location 1000 is " << storedMeters << " and " << aLengthKMs << "\n");
        }
        // outFile.write(reinterpret_cast<char *>(&aLengthKMs), sizeof(LengthKMs));
        // myRanges[myAt];

        //        ssc.get(vv.sinGcdIR, vv.cosGcdIR);
      }
    }
  }

  // Write the array
  count = buffer.size();
  outFile.write(reinterpret_cast<char *>(&count), sizeof(count));
  outFile.write(reinterpret_cast<char *>(buffer.data()), count * sizeof(FusionRangeCache));

  outFile.close();
  LogInfo("Wrote cache file to " << filename << " with " << count << " points.\n");
} // AzRanElevCache::writeRangeFile

std::vector<FusionRangeCache>
FusionCache::readRangeFile(const std::string& filename,
  size_t& sx, size_t& sy,
  size_t& x, size_t& y)
{
  std::ifstream infile(filename, std::ios::binary);

  if (!infile) {
    LogInfo("No Cache file " << filename << " exists.\n");
    return std::vector<FusionRangeCache>();
  }

  // Determine the file size
  //  FIXME: optimize by possibly having header and file length
  //  infile.seekg(0, std::ios::end);
  //  std::streampos fileSize = infile.tellg();

  //  infile.seekg(0, std::ios::beg);

  // Calculate the number of FusionRangeCache in the file
  // std::size_t numShorts = fileSize / sizeof(FusionRangeCache);

  // Create a vector to store the data
  // std::vector<FusionRangeCache> data(numShorts);

  // size_t x;
  // size_t y;
  // size_t sx;
  // size_t sy;
  size_t count;

  infile.read(reinterpret_cast<char *>(&x), sizeof(x));
  infile.read(reinterpret_cast<char *>(&y), sizeof(y));
  infile.read(reinterpret_cast<char *>(&sx), sizeof(sx));
  infile.read(reinterpret_cast<char *>(&sy), sizeof(sy));

  infile.read(reinterpret_cast<char *>(&count), sizeof(count));

  // Read the data from the file into the vector
  // FIXME: I think we should use the gzip methods like mrms binary does
  // Stage 1 'should' have the time to compress the file
  std::vector<FusionRangeCache> data(count);

  infile.read(reinterpret_cast<char *>(data.data()), count * sizeof(FusionRangeCache));

  if (!infile) {
    // handle error reading file
    LogSevere("Couldn't read cache file " << filename << "\n");
  }
  infile.close();

  // LogSevere("Read in value of " << data[1000] << "\n");
  // LogSevere("Number of points: " << count << "\n");

  #if 0
  LLCoverageArea(AngleDegs north, AngleDegs west, AngleDegs south, AngleDegs east, AngleDegs southDelta,
    AngleDegs eastSpacing,
    size_t aNumX, size_t aNumY) : nwLat(north), nwLon(west), seLat(south), seLon(east),
    latSpacing(southDelta), lonSpacing(eastSpacing), startX(0), startY(0), numX(aNumX), numY(aNumY){ }

  /** Set values, called by readers */
  void
    set(AngleDegs north, AngleDegs west, AngleDegs south, AngleDegs east, AngleDegs southDelta,
    AngleDegs eastSpacing, size_t aNumX, size_t aNumY);
  #endif

  return data;
} // FusionCache::readRangeFile

bool
FusionCache::writeMaskFile(const std::string& name, const std::string& filename, const Bitset& mask)
{
  bool success = true;

  // FIXME: any 'extra' header stuff
  std::ofstream outFile(filename, std::ios::binary);

  if (outFile.is_open()) {
    mask.writeBits(outFile);
    size_t on     = mask.getAllOnBits();
    size_t all    = mask.size();
    float percent = (all > 0) ? (float) (on) / (float) (all) : 0;
    percent = 100.0 - (percent * 100.0);

    // LogInfo("Wrote " << filename << " ("<< percent << ") " << on << " of " << all << "\n");
    LogInfo("Wrote " << name << " (" << percent << " % saved) " << on << " of " << all << "\n");
  } else {
    LogSevere("Couldn't write bitset to " << filename << "\n");
    success = false;
  }
  outFile.close();
  return success;
}

bool
FusionCache::readMaskFile(const std::string& filename, Bitset& mask)
{
  bool success = true;

  // FIXME: any 'extra' header stuff
  std::ifstream inFile(filename, std::ios::binary);

  if (inFile.is_open()) {
    mask.readBits(inFile);
  } else {
    LogInfo("Couldn't read mask " << filename << "\n");
    success = false;
  }
  return success;
}
