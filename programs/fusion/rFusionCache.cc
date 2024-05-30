#include "rFusionCache.h"

#include "rLLCoverageArea.h"

#include <fstream>

using namespace rapio;

std::string FusionCache::theRosterDir = "/home/mrms";

bool
FusionCache::writeRangeFile(const std::string& filefinal, LLCoverageArea& outg,
  std::vector<std::shared_ptr<AzRanElevCache> >& myLLProjections,
  LengthKMs maxRangeKMs)
{
  bool success = true;

  // Letting stage1 overwrite file each time for moment.  This 'could'
  // possibly be good enough for a marker.  These files are tiny so should be
  // ok....we'll see in practice.
  std::string filename = OS::getUniqueTemporaryFile("fusion");

  std::ofstream outFile(filename, std::ios::binary);

  if (!outFile.is_open()) {
    LogSevere("Can't write cache tmp file: " << filename << "\n");
    return false;
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
    llp.reset();

    AngleDegs atLat = startLat;
    for (size_t y = 0; y < outg.getNumY(); y++, atLat -= outg.getLatSpacing()) { // a north to south
      AngleDegs atLon = startLon;
      // Note: The 'range' is 0 to numX always, however the index to global grid is x+out.startX;
      for (size_t x = 0; x < outg.getNumX(); x++, atLon += outg.getLonSpacing(), llp.next()) { // a east to west row, changing lon per cell
        LengthKMs aLengthKMs;

        // Store as 2 bytes meters for space.  8 bytes is about 128 mb per file currently
        llp.getRangeKMsAt(aLengthKMs);

        // FusionRangeCache storedMeters = std::round(aLengthKMs * 1000.0);
        FusionRangeCache storedMeters = aLengthKMs;

        buffer.push_back(storedMeters);
      }
    }
  }

  // Write the array
  count = buffer.size();
  if (count != x * y * heightsKM.size()) {
    LogSevere("Size mismatch? " << x << ", " << y << ", " << heightsKM.size() << " != " << count << "\n");
  }
  outFile.write(reinterpret_cast<char *>(&count), sizeof(count));
  outFile.write(reinterpret_cast<char *>(buffer.data()), count * sizeof(FusionRangeCache));

  // Extra check file wrote successfully
  if (!outFile) {
    LogSevere("Couldn't write to tmp cache file " << filename << " for " << filefinal << "\n");
    outFile.close();
    return false;
  }

  outFile.close();

  // Finally move the tmp file to final location
  // Current multi-moment can clash on the move since both will write the same file,
  // so we'll try a couple times, then delete our tmp file to avoid filing the disk
  size_t attemptNumber      = 1;
  const size_t MAX_ATTEMPTS = 2;

  while (true) {
    if (OS::moveFile(filename, filefinal, true)) {
      LogInfo("Wrote cache file to " << filefinal << " with " << count << " points.\n");
      break;
    } else {
      if (attemptNumber > MAX_ATTEMPTS) {
        LogInfo("Couldn't move tmp " << filename << " to " << filefinal << "\n");
        OS::deleteFile(filename);
        success = false;
        break;
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
    }

    attemptNumber++;
  }

  #if 0
  // FIXME: Could write a unit test, but so much is required to reflect real time
  // operations, so I have a test here.
  // Read every file after writing test.
  LogInfo("Reading back cache file to check...\n");
  std::vector<FusionRangeCache> data2;
  size_t sx2, sy2, x2, y2;

  readRangeFile(filefinal, sx2, sy2, x2, y2, data2);
  bool bad = ((sx != sx2) || (sy != sy2) || (x != x2) || (y != y2) || (buffer.size() != data2.size()));

  if (bad) {
    LogSevere(
      "Uh oh ...read back not correct: (" << sx << "," << sx2 << ")(" << sy << "," << sy2 << ")(" << x << "," << x2 << ")(" << y << "," << y2 << ")(" << buffer.size() << "," << data2.size() <<
        "\n");
  } else {
    LogInfo("...Seems ok?\n");
  }
  #endif // if 0

  return success;
} // AzRanElevCache::writeRangeFile

bool
FusionCache::readRangeFile(const std::string& filename,
  size_t& sx, size_t& sy,
  size_t& x, size_t& y,
  std::vector<FusionRangeCache>& data)
{
  std::ifstream infile(filename, std::ios::binary);

  if (!infile) {
    LogInfo("No Cache file " << filename << " exists.\n");
    return false;
  }

  size_t count;

  infile.read(reinterpret_cast<char *>(&x), sizeof(x));
  infile.read(reinterpret_cast<char *>(&y), sizeof(y));
  infile.read(reinterpret_cast<char *>(&sx), sizeof(sx));
  infile.read(reinterpret_cast<char *>(&sy), sizeof(sy));

  // Why is count needed?  Isn't it a multiple of x,y,z or something?
  infile.read(reinterpret_cast<char *>(&count), sizeof(count));

  // We don't want to use a count that wasn't read correctly...
  if (infile) {
    try{
      // Read the data from the file into the vector
      data.resize(count);
      infile.read(reinterpret_cast<char *>(data.data()), count * sizeof(FusionRangeCache));
    }catch (std::bad_alloc& e) {
      // Memory check as well.
      LogSevere("Failed to allocation space for " << count << "\n");
    }
  }

  if (!infile) {
    // handle error reading file
    LogSevere("Couldn't read existing cache file " << filename << "\n");
    infile.close();
    // Copy file to someplace (Debugging)
    // static size_t count = 0;
    // OS::moveFile(filename, "/home/mrms/BADCACHE/badcache_" + std::to_string(count++) + ".cache");
    return false;
  }
  infile.close();

  return true;
} // FusionCache::readRangeFile

bool
FusionCache::writeMaskFile(const std::string& name, const std::string& filefinal, const Bitset& mask)
{
  bool success = true;

  std::string filename = OS::getUniqueTemporaryFile("fusion");

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

  // On successful write, move tmp file
  if (success) {
    if (OS::moveFile(filename, filefinal)) {
      LogInfo("Wrote mask file to " << filefinal << "\n");
    } else {
      LogInfo("Couldn't move tmp " << filename << " to " << filefinal << "\n");
      success = false;
    }
  }

  #if 0
  // FIXME: Could write a unit test, but so much is required to reflect real time
  // operations, so I have a test here.
  // Read every file after writing test.
  Bitset newmask;

  LogInfo("Reading back mask to check...\n");
  readMaskFile(filefinal, newmask);
  if (newmask.size() != mask.size()) {
    LogSevere("Uh oh ...read back not correct " << newmask.size() << " != " << mask.size() << "\n");
  } else {
    LogInfo("...Seems ok?\n");
  }
  #endif // if 0

  return success;
} // FusionCache::writeMaskFile

bool
FusionCache::readMaskFile(const std::string& filename, Bitset& mask)
{
  bool success = true;

  // FIXME: any 'extra' header stuff
  std::ifstream inFile(filename, std::ios::binary);

  if (inFile.is_open()) {
    mask.readBits(inFile);
  } else {
    success = false;
  }
  return success;
}

void
FusionCache::setRosterDir(const std::string& folder)
{
  if (!OS::isDirectory(folder)) {
    if (!OS::ensureDirectory(folder)) {
      LogSevere("Roster folder path '" << folder << "' can't be accessed or created.\n");
      exit(1);
    }
  }
  theRosterDir = folder;
  if (!Strings::endsWith(theRosterDir, "/")) {
    theRosterDir += "/";
  }
}
