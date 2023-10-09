#pragma once

/** RAPIO API */
#include <RAPIO.h>
#include "rTerrainBlockage.h"
#include "rLLCoverageArea.h"
#include "rVolumeValueResolver.h"
#include "rLLHGridN2D.h"

namespace rapio {
/**  Use 8 bits for FusionKey, with a max value of 255.  This means our volume
 * tilts need to be limited to 254 (0 key is no value) at a time in memory. This
 * is usually fine for a single moment.
 * This can be increased here safely if needed but it will cost more RAM. */
typedef uint8_t FusionKey;

/** Uses this size for storing range (meter level resolution at moment) */
typedef float FusionRangeCache;

/** This cache stores sin and cos for a 2D grid.  The advantage here is that
 * calculating sin/cos is fairly slow.  We can reuse the math when doing things
 * like calculating range, etc. for converting from polar to grid.
 */
class SinCosLatLonCache : public Utility
{
public:
  SinCosLatLonCache(size_t numX, size_t numY) :
    myNumX(numX), myNumY(numY), mySinGcdIR(numX * numY), myCosGcdIR(numX * numY), myAt(0)
  { }

  /** Return index into 2D lookup */
  inline size_t
  getIndex(size_t x, size_t y)
  {
    return (y * myNumX) + x;
  }

  /** Reset iterator functions */
  inline void
  reset()
  {
    myAt = 0;
  }

  /** Add at current position (a bit faster).  FIXME: could make an iterator */
  inline void
  add(const double sinGcdIR, const double cosGcdIR)
  {
    mySinGcdIR[myAt] = sinGcdIR;
    myCosGcdIR[myAt] = cosGcdIR;
    myAt++;
  }

  /** Get at current position (a bit faster) */
  inline void
  get(double& sinGcdIR, double& cosGcdIR)
  {
    sinGcdIR = mySinGcdIR[myAt];
    cosGcdIR = myCosGcdIR[myAt];
    myAt++;
  }

  /** The X size of our cache */
  size_t myNumX;

  /** The Y size of our cache */
  size_t myNumY;

  /** Current location for raw iteration */
  size_t myAt;

  /** Cached sin */
  std::vector<double> mySinGcdIR;

  /** Cached cos */
  std::vector<double> myCosGcdIR;
};

/** This is the trick, the lightbulb combining Lak's big brain with my slightly smaller one.
 * The radar Lat lon height doesn't change, and the grid of Lat lon height (conus output)
 * we project to is constant during the run.  So we can cache all projection math for this
 * that is calculating the virtual azimuth, elevation and range from radar center to
 * a lat lon height point in our output grid.
 *
 * Basically the entire output of BeamPath_LLHtoAzRangeElev.
 *
 * This would be per _RADAR_ and _OUTPUT_ grid.  So for N moments or incoming tilts this
 * can be shared.  Awesome.
 *
 * Tried using a 3D array of cache objects with pointer math and still slower than this,
 * interesting. It might be that a set of 2D caches better in RAM vs 3D due to L3, etc.
 */
class AzRanElevCache : public Utility
{
public:

  /** Create lookup of given size */
  AzRanElevCache(size_t numX, size_t numY) :
    myNumX(numX), myNumY(numY),
    myAzimuths(numX * numY), myVirtualElevations(numX * numY), myRanges(numX * numY),
    myTerrainAzimuthSpacing(1), myUpperElev(numX * numY), myLowerElev(numX * numY),
    myNextUpperElev(numX * numY), myNextLowerElev(numX * numY),
    myAt(0)
  { }

  /** Return index into 2D lookup */
  inline size_t
  getIndex(size_t x, size_t y)
  {
    return (y * myNumX) + x;
  }

  /** Store cache lookup given x and y */
  inline void
  set(size_t x, size_t y, const AngleDegs inAzDegs, const AngleDegs inElevDegs, const LengthKMs inRanges)
  {
    const size_t index = getIndex(x, y);

    myAzimuths[index] = inAzDegs;
    myVirtualElevations[index] = inElevDegs;
    myRanges[index] = inRanges;
  }

  /** Store upper/lower markers. Return true if it changes.  When these change, we recalculate data values */
  inline bool
  set(size_t x, size_t y, DataType * lower, DataType * upper, DataType * nextLower, DataType * nextUpper)
  {
    const size_t index = getIndex(x, y);

    bool changed = false;

    const FusionKey lowerKey = (lower == nullptr) ? 0 : lower->getID();

    if (myLowerElev[index] != lowerKey) {
      myLowerElev[index] = lowerKey;
      changed = true;
    }
    const FusionKey upperKey = (upper == nullptr) ? 0 : upper->getID();

    if (myUpperElev[index] != upperKey) {
      myUpperElev[index] = upperKey;
      changed = true;
    }
    const FusionKey nextLowerKey = (nextLower == nullptr) ? 0 : nextLower->getID();

    if (myNextLowerElev[index] != nextLowerKey) {
      myNextLowerElev[index] = nextLowerKey;
      changed = true;
    }
    const FusionKey nextUpperKey = (nextUpper == nullptr) ? 0 : nextUpper->getID();

    if (myNextUpperElev[index] != nextUpperKey) {
      myNextUpperElev[index] = nextUpperKey;
      changed = true;
    }
    return changed;
  }

  /** Reset iterator functions */
  inline void
  reset()
  {
    myAt = 0;
  }

  /** Add at current position (a bit faster).  FIXME: could make an iterator */
  inline void
  add(const AngleDegs inAzDegs, const AngleDegs inElevDegs, const LengthKMs inRanges)
  {
    myAzimuths[myAt] = inAzDegs;
    myVirtualElevations[myAt] = inElevDegs;
    myRanges[myAt] = inRanges;
    myAt++;
  }

  /** Get at current position (a bit faster) */
  inline void
  get(AngleDegs& outAzDegs, AngleDegs& outElevDegs, LengthKMs& outRanges)
  {
    outAzDegs   = myAzimuths[myAt];
    outElevDegs = myVirtualElevations[myAt];
    outRanges   = myRanges[myAt];
    myAt++;
  }

  /** Cache lookup given x and y */
  inline void
  get(size_t x, size_t y, AngleDegs& outAzDegs, AngleDegs& outElevDegs, LengthKMs& outRanges)
  {
    const size_t index = getIndex(x, y);

    outAzDegs   = myAzimuths[index];
    outElevDegs = myVirtualElevations[index];
    outRanges   = myRanges[index];
  }

public:

  /** The X size of our cache */
  size_t myNumX;

  /** The Y size of our cache */
  size_t myNumY;

  /** Cached azimuth degrees for each cell */
  std::vector<AngleDegs> myAzimuths;

  /** Cached virtual elevation degrees for each cell */
  std::vector<AngleDegs> myVirtualElevations;

  // FIXME: meters better here maybe?
  /** Cached range kilometers for each cell */
  std::vector<LengthKMs> myRanges;

  /** Store the terrain azimuth spacing for our cached terrain */
  AngleDegs myTerrainAzimuthSpacing;

  /** Cached last upper elevation for this x, y.  This is only important as a number */
  std::vector<FusionKey> myUpperElev;

  /** Cached last lower elevation for this x, y.  This is only important as a number */
  std::vector<FusionKey> myLowerElev;

  /** Cached last next upper elevation for this x, y.  This is only important as a number */
  std::vector<FusionKey> myNextUpperElev;

  /** Cached last lower elevation for this x, y.  This is only important as a number */
  std::vector<FusionKey> myNextLowerElev;

  /** Current location for raw iteration */
  size_t myAt;
};

/** Common routines for the disk/ram caching required for fusion */
class FusionCache : public Utility
{
public:

  /** Common code for directories for stage1/roster */
  static std::string
  getDirectory(const std::string& subfolder, const LLCoverageArea& grid)
  {
    std::stringstream ss;

    ss << theRosterDir;

    ss << "GRID" << "_";
    ss << grid.getNWLat() << "_";
    ss << grid.getNWLon() << "_";
    ss << grid.getSELat() << "_";
    ss << grid.getSELon() << "_";
    ss << grid.getLatSpacing() << "_";
    ss << grid.getLonSpacing() << "_";
    ss << grid.getStartX() << "_";
    ss << grid.getStartY() << "_";
    ss << grid.getNumX() << "_";
    ss << grid.getNumY() << "/" << subfolder << "/";
    return ss.str();
  }

  /** Common code for filenames for stage1/roster */
  static std::string
  getFilename(const std::string& name, const std::string& subfolder,
    const std::string& suffix,
    const LLCoverageArea& grid, std::string& directory)
  {
    directory = getDirectory(subfolder, grid);
    return (name + suffix);
  };

  /** Get the directory for range/marker files */
  static std::string
  getRangeDirectory(const LLCoverageArea& grid)
  {
    return getDirectory("active", grid);
  }

  /** Get the range/marker from stage1 filename and directory */
  static std::string
  getRangeFilename(const std::string& name, const LLCoverageArea& grid,
    std::string& directory)
  {
    return getFilename(name, "active", ".cache", grid, directory);
  }

  /** Get the directory for mask files */
  static std::string
  getMaskDirectory(const LLCoverageArea& grid)
  {
    return getDirectory("mask", grid);
  }

  /** Get the mask from roster filename and directory */
  static std::string
  getMaskFilename(const std::string& name, const LLCoverageArea& grid,
    std::string& directory)
  {
    return getFilename(name, "mask", ".mask", grid, directory);
  }

  /** Set the base directory used for rostering */
  static void
  setRosterDir(const std::string& folder);

  /** Write ranges to a binary file.
   * The range files are written by a running stage1 since the range
   * math can take a bit and this allows horizontal scaling.
   */
  static bool
  writeRangeFile(const std::string& filename, LLCoverageArea& outg,
    std::vector<std::shared_ptr<AzRanElevCache> >& myLLProjections,
    LengthKMs maxRangeKMs);

  /** Read ranges from a binary file.
   * Range files are read by roster in order to do the nearest
   * active neighbor calculation.
   */
  static std::vector<FusionRangeCache>
  readRangeFile(const std::string& filename,
    size_t& startX, size_t& startY,
    size_t& numX, size_t& numY);

  /** Write mask to a binary file.
   * Masks are 1 and 0 small files, written by roster after the
   * nearest active neighbor calculation.  There is one written
   * intended for each stage1.
   */
  static bool
  writeMaskFile(const std::string& name, const std::string& filename, const Bitset& mask);

  /** Read mask to a binary file.
   * Masks are 1 and 0 small files, written by roster after the
   * nearest active neighbor calculation.  There is one written
   * intended for each stage1.
   */
  static bool
  readMaskFile(const std::string& filename, Bitset& mask);

  /** The base directory used for rostering */
  static std::string theRosterDir;
};
}
