#include "rFusionRoster.h"
#include "rFusionCache.h"
#include "rError.h"
#include "rConfigRadarInfo.h"
#include "rDirWalker.h"

using namespace rapio;

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

  // Default sync heartbeat to 1 mins
  // Note that roster probably takes longer than this, but that's fine
  // we'll just get another trigger immediately
  // Format is seconds then mins
  o.setRequiredValue("sync", "0 */1 * * * *");
}

/** RAPIOAlgorithms process options on start up */
void
RAPIOFusionRosterAlg::processOptions(RAPIOOptions& o)
{
  o.getLegacyGrid(myFullGrid);

  FusionCache::setRosterDir(o.getString("roster"));
} // RAPIOFusionRosterAlg::processOptions

void
RAPIOFusionRosterAlg::firstTimeSetup()
{
  // Full tile/CONUS we represent
  int NFULL = myFullGrid.getNumX() * myFullGrid.getNumY() * myFullGrid.getNumZ();
  ProcessTimer createGrid("Created");

  LogInfo("Creating nearest array of size " << NFULL << ", each cell " << sizeof(NearestIDs) << " bytes.\n");
  // FIXME: StaticVectorT<NearestIDs>({dimensions})
  // All the SparseVector/etc. I need a clean up pass on that design
  myNearest      = std::vector<NearestIDs>(NFULL);
  myNearestIndex = DimensionMapper({ myFullGrid.getNumX(), myFullGrid.getNumY(), myFullGrid.getNumZ() });

  LogInfo(createGrid << "\n");
}

void
RAPIOFusionRosterAlg::processNewData(rapio::RAPIOData& d)
{
  // We won't be running this way probably
  // performRoster();
}

void
RAPIOFusionRosterAlg::ingest(const std::string& sourcename, const std::string& fullpath)
{
  ProcessTimer merge("Reading range/merging 1 took:\n");

  size_t startX, startY, numX, numY; // subgrid coordinates

  // We're reading to get the data and grid..which we need to generate the mask
  auto out = FusionCache::readRangeFile(fullpath, startX, startY, numX, numY);

  // Make sure we have correct size..or skip
  const size_t numZ     = myFullGrid.getNumZ();
  const size_t expected = numX * numY * numZ;

  if (out.size() != expected) {
    LogSevere("Expected " << expected << " points but got " << out.size() << " for " << fullpath << "\n");
    return;
  }
  // FIXME: check read errors?

  static bool firstTime = true;

  if (firstTime) {
    // zero is reserved as missing...so plug in one
    mySourceInfos.push_back(SourceInfo());
    firstTime = false;
  }

  // Find the info and id for this source...technically if we're reading once only then
  // we'll always be adding new IDs
  SourceInfo * info = nullptr;
  auto infoitr      = myNameToInfo.find(sourcename);

  std::string newname = sourcename;

  if (infoitr != myNameToInfo.end()) {
    info = &mySourceInfos[infoitr->second]; // existing
    LogInfo("Found '" << info->name << "' in source database.\n");
  } else {
    mySourceInfos.push_back(SourceInfo()); // FIXME: constructor
    info = &mySourceInfos[mySourceInfos.size() - 1];

    info->id     = mySourceInfos.size() - 1;
    info->startX = startX; // gotta keep info to generate matching mask later
    info->startY = startY; // start NORTH (lat)
    info->numX   = numX;
    info->numY   = numY;

    // A mask of bits
    info->mask = Bitset({ numX, numY, numZ }, 1);
    info->mask.clearAllBits();
    info->name = newname;
    myNameToInfo[newname] = info->id;

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
RAPIOFusionRosterAlg::generateNearest()
{
  // Link cache files to our ingest method...
  class myDirWalker : public DirWalker
  {
public:
    /** Create dir walker helper */
    myDirWalker(RAPIOFusionRosterAlg * owner) : myOwner(owner)
    { }

    /** Process a regular file */
    virtual Action
    processRegularFile(const std::string& filePath, const struct stat * info) override
    {
      // FIXME: Let base class filter filenames?
      if (Strings::endsWith(filePath, ".cache")) {
        // FIXME: time filter only latest

        // For the moment, the source name is part of file name..we 'could' embed it
        std::string f = filePath;
        Strings::removeSuffix(f, ".cache");
        size_t lastSlash = f.find_last_of('/');
        if (lastSlash != std::string::npos) {
          f = f.substr(lastSlash + 1);
        }
        // LogInfo("Found '" << f << "' source range cache...\n");
        // printPath("Source: ", filePath, info);
        myOwner->ingest(f, filePath); // ingest current radar file
      }
      return Action::CONTINUE;
    }

    /** Process a directory */
    virtual Action
    processDirectory(const std::string& dirPath, const struct stat * info) override
    {
      // Ok manually force single directory.  FIXME: Could let base have this ability
      // So we don't recursivity walk
      if (dirPath + "/" == getCurrentRoot()) { // FIXME: messy
        return Action::CONTINUE;               // Process the root
      } else {
        return Action::SKIP_SUBTREE; // We're not recursing directories
      }
    }

protected:

    RAPIOFusionRosterAlg * myOwner;
  };

  myWalkTimer = new ProcessTimerSum();
  myWalkCount = 0;
  LogInfo("Grid is " << myFullGrid << "\n");
  //  int NFULL = myFullGrid.getNumX() * myFullGrid.getNumY() * myFullGrid.getNumZ();
  firstTimeSetup();
  std::string directory = FusionCache::getRangeDirectory(myFullGrid);
  myDirWalker walk(this);

  walk.traverse(directory);
  LogInfo("Total walk/merge time " << *myWalkTimer << " (" << myWalkCount << ") sources active.\n");
  delete myWalkTimer;
} // RAPIOFusionRosterAlg::generateNearest

void
RAPIOFusionRosterAlg::nearestNeighbor(std::vector<FusionRangeCache>& out, size_t id, size_t startX, size_t startY,
  size_t numX, size_t numY)
{
  size_t counter = 0;

  for (size_t z = 0; z < myFullGrid.getNumZ(); ++z) {
    for (size_t y = startY; y < startY + numY; ++y) {   // north to south
      for (size_t x = startX; x < startX + numX; ++x) { // east to west
        size_t i = myNearestIndex.getIndex3D(x, y, z);

        // Update the 'nearest' for this radar id, etc.
        auto v  = out[counter++];
        auto& c = myNearest[i];

        // Maintain sorted from lowest to highest.

        const size_t size = FUSION_MAX_CONTRIBUTING;
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
} // RAPIOFusionRosterAlg::nearestNeighbor

void
RAPIOFusionRosterAlg::generateMasks()
{
  ProcessTimer maskGen("Generating coverage bit masks.\n");

  // Clear old bits
  for (auto& s:mySourceInfos) {
    s.mask.clearAllBits();
  }

  // Mask generation algorithm
  for (size_t z = 0; z < myFullGrid.getNumZ(); ++z) {
    for (size_t y = 0; y < myFullGrid.getNumY(); ++y) {
      for (size_t x = 0; x < myFullGrid.getNumX(); ++x) {
        size_t i = myNearestIndex.getIndex3D(x, y, z);
        auto& c  = myNearest[i];

        for (size_t k = 0; k < FUSION_MAX_CONTRIBUTING; ++k) {
          // if we find a 0 id, then since we're insertion sorted by range,
          // then this means all are 0 later...so continue
          if (c.id[k] == 0) {
            continue;
          }
          // If not zero, we need the sourceInfo for it.  This is why
          // we store as ids in a vector it makes this lookup fast
          SourceInfo * info = &mySourceInfos[c.id[k]];
          Bitset& b         = info->mask; // refer to mask

          // FIXME: it's possible locally caching 'hits' would speed up things
          // since we know weather clumps...
          const size_t lx = x - (info->startX); // "shouldn't" underflow
          const size_t ly = y - (info->startY);
          const size_t li = b.getIndex3D(lx, ly, z);
          // My class here could be faster...let's see if it matters...
          // info->mask.set(li, 1); // humm this loops..FIXME: the 1 case can be faster
          info->mask.set1(li); // Set one bit
        }
      }
    }
  }
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
  LogInfo(maskTimer);
}

void
RAPIOFusionRosterAlg::performRoster()
{
  generateNearest(); // Read all stage1 info files, for example "KTLX.cache" and merge nearest values

  generateMasks(); // In RAM, create 1/0 bit array for each source

  writeMasks(); // Write bit array, for example "KTLX.mask" for stage1

  LogInfo("Finished one pass...\n");
  ;

  // For now exit since still running by hand
  exit(1);
} // RAPIOFusionRosterAlg::processNewData

void
RAPIOFusionRosterAlg::processHeartbeat(const Time& n, const Time& p)
{
  if (isDaemon()) { // just checking, don't think we get message if we're not
    LogInfo("Received heartbeat at " << n << " for event " << p << ".\n");
    performRoster();
  }
}

int
main(int argc, char * argv[])
{
  RAPIOFusionRosterAlg alg = RAPIOFusionRosterAlg();

  alg.executeFromArgs(argc, argv);
} // main
