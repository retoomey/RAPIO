#include "rFusionRoster.h"
#include "rFusionCache.h"
#include "rError.h"
#include "rConfigRadarInfo.h"
#include "rDirWalker.h"

using namespace rapio;

void
RAPIOFusionRosterAlg::declarePlugins()
{
  // We don't want the "-i" standard input at least for now, so we'll disable it
  // Could use it for the roster files, but we use --roster for that in stage 1,
  // so we'll stay consistent since we don't need notification
  removePlugin("i");
}

void
RAPIOFusionRosterAlg::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "RAPIO Fusion Roster Algorithm.  Watches metainfo from N stage one algorithms and updates generation masks.");
  o.setAuthors("Robert Toomey");

  // legacy use the t, b and s to define a grid
  o.declareLegacyGrid();

  // Set roster directory to home by default for the moment.
  // We might want to turn it off in archive mode or testing at some point
  const std::string home(OS::getEnvVar("HOME"));

  o.optional("roster", home, "Location of roster/cache folder.");

  #if USE_STATIC_NEAREST
  // In this case, we'll use FUSION_MAX_CONTRIBUTING
  #else
  o.optional("nearest", "3", "How many sources contribute? Note: The largest this is the more RAM to run roster.");
  #endif

  o.boolean("static", "Coverage files don't expire keeping a static nearest N.");
  o.addAdvancedHelp("static",
    "This has to do with radars/sources going up and down.  Coverage files from stage1 by default expire. This gives us a dynamic nearest N radars, where a further source can join in if one of the first N is down. However, with static turned on, all sources that exist are counted, even if offline.");

  // Default sync heartbeat to 1 mins
  // Note that roster probably takes longer than this, but that's fine
  // we'll just get another trigger immediately
  // Format is seconds then mins
  o.setDefaultValue("sync", "0 */1 * * * *");
}

/** RAPIOAlgorithms process options on start up */
void
RAPIOFusionRosterAlg::processOptions(RAPIOOptions& o)
{
  o.getLegacyGrid(myFullGrid);

  FusionCache::setRosterDir(o.getString("roster"));

  #ifdef USE_STATIC_NEAREST
  myNearestCount = FUSION_MAX_CONTRIBUTING;
  #else
  myNearestCount = o.getInteger("nearest");
  if (myNearestCount < 1) {
    LogInfo("Setting nearest to 1\n");
    myNearestCount = 1;
  }
  if (myNearestCount > 6) {
    LogInfo("Setting nearest to 6\n");
    myNearestCount = 6;
  }
  #endif // ifdef USE_STATIC_NEAREST
  LogInfo(">>>>>>Using a nearest neighbor value of " << myNearestCount << "\n");

  myStatic = o.getBoolean("static");
  if (myStatic) {
    LogInfo("Static mode on, we will use expired coverage files.\n");
  } else {
    LogInfo("Dynamic mode on, coverage files will expire.\n");
  }
} // RAPIOFusionRosterAlg::processOptions

void
RAPIOFusionRosterAlg::firstTimeSetup()
{
  // Full tile/CONUS we represent
  size_t size = myNearestCount;
  int NFULL   = myFullGrid.getNumX() * myFullGrid.getNumY() * myFullGrid.getNumZ();
  ProcessTimer createGrid("Created");

  myNearestIndex = DimensionMapper({ myFullGrid.getNumX(), myFullGrid.getNumY(), myFullGrid.getNumZ() });

  #ifdef USE_STATIC_NEAREST
  // Old way, grouped by static nearest (compile only size, probably slightly quicker)
  LogInfo("Creating nearest array of size " << NFULL << ", each cell " << sizeof(NearestIDs) << " bytes.\n");
  myNearest = std::vector<NearestIDs>(NFULL);
  #else

  // New way, grouped separately (dynamic for possible memory locality issues)
  LogInfo("Creating nearest array of size " << NFULL << ", each cell " <<
    (sizeof(SourceIDKey) + sizeof(float)) * (NFULL * size) << " bytes.\n");
  myNearestSourceIDKeys = std::vector<SourceIDKey>(NFULL * size, 0);
  myNearestRanges       = std::vector<float>(NFULL * size, std::numeric_limits<float>::max()); // The ranges per location
  #endif

  LogInfo(createGrid << "\n");
}

void
RAPIOFusionRosterAlg::processNewData(rapio::RAPIOData& d)
{
  // We won't be running this way probably
  // performRoster();
}

bool
RAPIOFusionRosterAlg::expiredCoverage(const std::string& sourcename, const std::string& fullpath)
{
  // Get the time of the coverage file.  If it's older than -h then we assume that source
  // is down or not reporting and we remove it from this masking pass.
  // 'Unless' static mode is on
  if (!myStatic) { // So dynamic mode the default...
    Time fileTime;

    if (OS::getFileModificationTime(fullpath, fileTime)) {
      // FIXME: Need a time age method I think
      const Time current    = Time::CurrentTime();
      const Time cutoffTime = current - myMaximumHistory;
      const time_t cutoff   = cutoffTime.getSecondsSinceEpoch();
      if (fileTime.getSecondsSinceEpoch() < cutoff) {
        // File is too old so we ignore it...
        LogInfo("Ignored '" << sourcename << "' since it is too old. " << fileTime << "\n");
        return true;
      }
    } else {
      // Say something here?
      return false; // Strange to happen since we're walking..maybe the file got deleted?
    }
  }
  return false;
}

void
RAPIOFusionRosterAlg::walkerCallback(const std::string& what, const std::string& sourcename,
  const std::string& fullpath)
{
  if (what == "INGEST") {
    // Reading in .cache files
    ingest(sourcename, fullpath);
  } else if (what == "DELETE") {
    // Deleting masks for old stuff we didn't have a cache file for
    deleteMask(sourcename, fullpath);
  }
}

void
RAPIOFusionRosterAlg::ingest(const std::string& sourcename, const std::string& fullpath)
{
  // Don't handle expired files (not included in mask calculations)
  if (expiredCoverage(sourcename, fullpath)) {
    return; // ignore it
  }

  ProcessTimer merge("Reading range/merging 1 took:\n");

  size_t startX, startY, numX, numY; // subgrid coordinates

  // We're reading to get the data and grid..which we need to generate the mask
  std::vector<FusionRangeCache> out;

  if (!FusionCache::readRangeFile(fullpath, startX, startY, numX, numY, out)) {
    return;
  }

  // Make sure we have correct size..or skip
  const size_t numZ     = myFullGrid.getNumZ();
  const size_t expected = numX * numY * numZ;

  if (out.size() != expected) {
    LogSevere("Expected " << expected << " points but got " << out.size() << " for " << fullpath << "\n");
    LogSevere("EXTRA: " << numX << ", " << numY << ", " << numZ << "\n");
    return;
  }
  // FIXME: check read errors?

  // We're going to redo this everytime, because of chicken/egg issues.

  if (mySourceInfos.size() == 0) {
    // zero is reserved as missing...so plug in one
    mySourceInfos.push_back(SourceInfo());
  }

  // Find the info and id for this source...technically if we're reading once only then
  // we'll always be adding new IDs
  // FIXME: since we add each time now...clean this up (the find shouldn't be necessary)
  SourceInfo * info = nullptr;
  auto infoitr      = myNameToInfo.find(sourcename);

  std::string newname = sourcename;

  if (infoitr != myNameToInfo.end()) {
    info = &mySourceInfos[infoitr->second];                        // existing
    LogInfo("Found '" << info->name << "' in source database.\n"); // This shouldn't happen anymore
  } else {
    mySourceInfos.push_back(SourceInfo()); // FIXME: constructor
    info = &mySourceInfos[mySourceInfos.size() - 1];

    info->id     = mySourceInfos.size() - 1;
    info->startX = startX; // gotta keep info to generate matching mask later
    info->startY = startY; // start NORTH (lat)
    info->numX   = numX;
    info->numY   = numY;
    info->name   = newname;
    myNameToInfo[newname] = info->id;

    // A mask of bits
    try{
      info->mask = Bitset({ numX, numY, numZ }, 1);
      info->mask.clearAllBits();
    }catch (std::bad_alloc& e) {
      LogSevere("We ran out of memory allocating mask. This will crash.\n");
      LogSevere("ATTEMPTED: '" << newname << "' (" << info->id << ") (" << startX << "," << startY << "," <<
        numX << "," << numY << "," << numZ << ") " << numX * numY * numZ << " points.\n");
      exit(1);
    }

    LogInfo("Added '" << newname << "' (" << info->id << ") (" << startX << "," << startY << "," <<
      numX << "," << numY << "," << numZ << ") " << numX * numY * numZ << " points.\n");
  }

  // Nearest neighbor merge this source's info into the nearest N array...
  // We'll make masks later
  nearestNeighbor(out, info->id, info->startX, info->startY, info->numX, info->numY);
  if (myWalkTimer != nullptr) {
    myWalkTimer->add(merge);
  }
  myWalkCount++;
} // RAPIOFusionRosterAlg::ingest

void
RAPIOFusionRosterAlg::deleteMask(const std::string& sourcename, const std::string& fullpath)
{
  // See if this mask is in our known cache list
  bool found = false;

  for (auto& s:myNameToInfo) {
    SourceInfo& source = mySourceInfos[s.second];
    if (source.name == sourcename) { // FIXME: check uppercase always
      found = true;
      break;
    }
  }

  if (!found) {
    LogInfo("Deleting mask for '" << sourcename << "', since we didn't get a good cache file for it.\n");

    if (!OS::deleteFile(fullpath)) {
      LogSevere("Unable to delete old mask file for '" << sourcename << "'\n");
    }
  }
}

void
RAPIOFusionRosterAlg::generateNearest()
{
  myWalkTimer = new ProcessTimerSum();
  myWalkCount = 0;
  LogInfo("Grid is " << myFullGrid << "\n");
  //  int NFULL = myFullGrid.getNumX() * myFullGrid.getNumY() * myFullGrid.getNumZ();
  firstTimeSetup();

  // Clear out our stored radars
  mySourceInfos.clear();
  myNameToInfo.clear();

  // Walk the cache directory looking for current/good files
  std::string directory = FusionCache::getRangeDirectory(myFullGrid);
  myDirWalker walk("INGEST", this, ".cache");

  walk.traverse(directory);

  LogInfo("Total walk/merge time " << *myWalkTimer << " (" << myWalkCount << ") sources active.\n");
  delete myWalkTimer;
} // RAPIOFusionRosterAlg::generateNearest

void
RAPIOFusionRosterAlg::nearestNeighbor(std::vector<FusionRangeCache>& out, size_t id, size_t startX, size_t startY,
  size_t numX, size_t numY)
{
  size_t counter    = 0;
  const size_t size = myNearestCount;

  #if USER_STATIC_NEAREST

  for (size_t z = 0; z < myFullGrid.getNumZ(); ++z) {
    for (size_t y = startY; y < startY + numY; ++y) {   // north to south
      for (size_t x = startX; x < startX + numX; ++x) { // east to west
        size_t at = myNearestIndex.getIndex3D(x, y, z);

        // Update the 'nearest' for this radar id, etc.
        auto v = out[counter++];

        // Maintain sorted from lowest to highest.
        auto& c = myNearest[at];

        // Find the correct position to insert v using linear search (small N)
        int index = 0;
        while (index < size && v >= c.range[index]) {
          ++index;
        }

        // If not pass end of array...
        if (index < size) {
          // ...then shift elements to make room for v
          // which pushes out the largest
          if (index < size - 1) {
            for (int i = size - 1; i > index; --i) {
              c.range[i] = c.range[i - 1];
              c.id[i]    = c.id[i - 1];
            }
          }

          // and add v
          c.range[index] = v;
          c.id[index]    = id;
        }
      }
    }
  }
  #else // if USER_STATIC_NEAREST

  // NEW WAY TESTING
  for (size_t z = 0; z < myFullGrid.getNumZ(); ++z) {
    for (size_t y = startY; y < startY + numY; ++y) {   // north to south
      for (size_t x = startX; x < startX + numX; ++x) { // east to west
        size_t at = myNearestIndex.getIndex3D(x, y, z);

        // Update the 'nearest' for this radar id, etc.
        auto v = out[counter++];

        // Find the correct position to insert v using linear search (small N)
        const size_t atr = at * size; // relative into group
        auto& k       = myNearestSourceIDKeys;
        auto& r       = myNearestRanges;
        size_t index2 = 0;
        while (index2 < size && v >= r[atr + index2]) {
          ++index2;
        }

        // If not pass end of array...
        if (index2 < size) {
          // ...then shift elements to make room for v
          // which pushes out the largest
          if (index2 < size - 1) {
            for (int i = size - 1; i > index2; --i) {
              r[atr + i] = r[atr + i - 1];
              k[atr + i] = k[atr + i - 1];
            }
          }

          // and add v
          k[atr + index2] = id;
          r[atr + index2] = v;
        }
      }
    }
  }
  #endif // if USER_STATIC_NEAREST
} // RAPIOFusionRosterAlg::nearestNeighbor

void
RAPIOFusionRosterAlg::generateMasks()
{
  LogInfo("Generating converage bit masks...\n");
  ProcessTimer maskGen("Generating coverage bit masks took:\n");
  const size_t size = myNearestCount;

  // Clear old bits
  for (auto& s:mySourceInfos) {
    s.mask.clearAllBits();
  }

  #if USE_STATIC_NEAREST
  // Mask generation algorithm
  for (size_t z = 0; z < myFullGrid.getNumZ(); ++z) {
    for (size_t y = 0; y < myFullGrid.getNumY(); ++y) {
      for (size_t x = 0; x < myFullGrid.getNumX(); ++x) {
        size_t at = myNearestIndex.getIndex3D(x, y, z);

        auto& c = myNearest[at];

        for (size_t i = 0; i < size; ++i) {
          // if we find a 0 id, then since we're insertion sorted by range,
          // then this means all are 0 later...so continue
          if (c.id[i] == 0) {
            continue;
          }
          // If not zero, we need the sourceInfo for it.  This is why
          // we store as ids in a vector it makes this lookup fast
          SourceInfo * info = &mySourceInfos[c.id[i]];
          Bitset& b         = info->mask;         // refer to mask
          const size_t lx   = x - (info->startX); // "shouldn't" underflow
          const size_t ly   = y - (info->startY);
          const size_t li   = b.getIndex3D(lx, ly, z);
          info->mask.set1(li); // Set one bit
          counter++;
        }
      }
    }
  }
  #else // if USE_STATIC_NEAREST

  // Mask generation algorithm
  auto& k = myNearestSourceIDKeys;
  auto& r = myNearestRanges;
  for (size_t z = 0; z < myFullGrid.getNumZ(); ++z) {
    for (size_t y = 0; y < myFullGrid.getNumY(); ++y) {
      for (size_t x = 0; x < myFullGrid.getNumX(); ++x) {
        size_t at = myNearestIndex.getIndex3D(x, y, z);

        const size_t atr = at * size; // relative into group

        for (size_t i = 0; i < size; ++i) {
          // if we find a 0 id, then since we're insertion sorted by range,
          // then this means all are 0 later...so continue
          if (k[atr + i] == 0) {
            continue;
          }
          // If not zero, we need the sourceInfo for it.  This is why
          // we store as ids in a vector it makes this lookup fast
          SourceInfo * info = &mySourceInfos[k[atr + i]];
          Bitset& b         = info->mask;         // refer to mask
          const size_t lx   = x - (info->startX); // "shouldn't" underflow
          const size_t ly   = y - (info->startY);
          const size_t li   = b.getIndex3D(lx, ly, z);
          info->mask.set1(li); // Set one bit
        }
      }
    }
  }
  #endif // if USE_STATIC_NEAREST
  LogInfo(maskGen);
} // RAPIOFusionRosterAlg::generateMasks

void
RAPIOFusionRosterAlg::writeMasks()
{
  // Write masks to disk
  ProcessTimer maskTimer("Writing masks took:\n");
  size_t maskCount = 0;

  std::string directory = FusionCache::getMaskDirectory(myFullGrid);

  OS::ensureDirectory(directory);
  LogInfo("Mask directory is " << directory << "\n");
  size_t all = 0;
  size_t on  = 0;

  for (auto& s:myNameToInfo) {
    SourceInfo& source   = mySourceInfos[s.second];
    std::string maskFile = FusionCache::getMaskFilename(source.name, myFullGrid, directory);
    std::string fullpath = directory + maskFile;
    FusionCache::writeMaskFile(source.name, fullpath, source.mask);
    on  += source.mask.getAllOnBits();
    all += source.mask.size();
    maskCount++;
  }
  LogInfo("Writing " << maskCount << " masks took: " << maskTimer << "\n");

  float percent = (all > 0) ? (float) (on) / (float) (all) : 0;

  percent = 100.0 - (percent * 100.0);
  LogInfo("Reduction of calculated points (higher better): " << percent << "%\n");

  // Now we need to delete masks for things we didn't have a cache file for.
  myDirWalker walk("DELETE", this, ".mask");

  walk.traverse(directory);

  LogInfo(maskTimer);
} // RAPIOFusionRosterAlg::writeMasks

void
RAPIOFusionRosterAlg::performRoster()
{
  generateNearest(); // Read all stage1 info files, for example "KTLX.cache" and merge nearest values

  generateMasks(); // In RAM, create 1/0 bit array for each source

  writeMasks(); // Write bit array, for example "KTLX.mask" for stage1

  LogInfo("Finished one pass...\n");
} // RAPIOFusionRosterAlg::processNewData

void
RAPIOFusionRosterAlg::processHeartbeat(const Time& n, const Time& p)
{
  LogInfo("Received heartbeat at " << n << " for event " << p << ".\n");
  performRoster();
}

int
main(int argc, char * argv[])
{
  RAPIOFusionRosterAlg alg = RAPIOFusionRosterAlg();

  alg.executeFromArgs(argc, argv);
} // main
