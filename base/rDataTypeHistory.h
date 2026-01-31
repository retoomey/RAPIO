#pragma once

#include <rDataType.h>
#include <rRecord.h>
#include <rElevationVolume.h>
#include <rRAPIOData.h>

#include <memory>

#include <vector>

namespace rapio {
/** Store a history of objects of some sort.
 * API meant to be called for incoming data of interest.
 *
 * The main algorithm purges the history cache based on the -h
 * history.
 *
 * 2. For NSE grids or auto watching a subtype.  The NSE grid
 * would have a 'followGrid' which had a key and default lookup
 * to match a given DataType.  Also, I don't think it time purges,
 * or if it does it would be different than the history.  An NSE grid
 * might hang around a lot longer.
 *
 * FIXME: Guess we could introduce history subclasses.
 * I'm just hardcoding them for the moment...
 *
 * @ingroup rapio_data
 * @brief Base class for storing history of DataType.
 * @author Robert Toomey
 */
class DataTypeHistory {
public:

  #if 0
  // Keeping for the moment

  /** Update the virtual volume for this DataType if possible,
   * Volumes are named by product and contain N subtypes, for example
   * a Reflectivity volume containing 10 elevations that can be used for
   * interpolating values in space time (CAPPI/VSLICE)
   */
  static void
  updateVolume(std::shared_ptr<DataType> data);

  // Any other type of history/storage we can have functions added here

  /** Get a full slice of moments matching a given subtype.  An empty vector
   * is returned if any moments are still missing.  Note this function doesn't work
   * for multi-radar input since there could be say two subtypes of 01.80 degree for
   * Reflectivity for two radars.  However for single radar processing with N moments
   * that share subtypes, this allows processing those moments in group.  */
  static std::vector<std::shared_ptr<DataType> >
  getFullSubtypeGroup(const std::string& subtype, const std::vector<std::string>& keys);

  /** Purge a given subtype from all stored volumes.  This can be used to make sure
   * a processed SubtypeGroup is not processed again */
  static size_t
  deleteSubtypeGroup(const std::string& subtype, const std::vector<std::string>& keys);
  #endif // if 0

  /** Called automatically by the algorithm on new incoming records */
  static void
  processNewData(RAPIOData& d);

  /** Purge history for volumes, collections, etc.  Called by
   * RAPIO automatically you don't need to call it.  Use the -h
   * option to determine max time window */
  static void
  purgeTimeWindow(const Time& time);
};

/** A simple volume history. Volume classes register here in order
 * to receive the -h time window from the algorithm.  This allows
 * volumes to expire their tilts.
 *
 * @ingroup rapio_data
 * @brief Base class for volume based histories of DataType.
 * @author Robert Toomey
 */
class VolumeHistory : public DataTypeHistory {
public:
  friend DataTypeHistory;

  /** Register a volume for a particular key (overriding the updateVolume creation method) */
  static void
  registerVolume(const std::string& key, std::shared_ptr<Volume> v);

protected:
  /** Purge history for volumes */
  static void
  purgeTimeWindow(const Time& time);

  // Storage for volumes
  static std::map<std::string, std::shared_ptr<Volume> > myVolumes;
};

/** Storage of DataTypes that come in rarely, such as NSE grids
 * @ingroup rapio_data
 * @brief Base class for storing NSE grid history.
 */
class NSEGridHistory : public DataTypeHistory {
public:
  friend DataTypeHistory;

  /** Follow a given grid.  Currently I'm hardcoding the datatype and the
   * default value, though MRMS uses an xml config lookup. */
  static int
  followGrid(const std::string& key, const std::string& aTypeName, float aDefault);

  /** Do we handle this data? */
  static void
  processNewData(RAPIOData& d);

  /** Get a grid.  If followGrid has been called, this will return a
   * valid grid for use.  This is a cached grid and shared.
   * Maybe we could return a reference to the primary to make alg
   * locic simplier? */
  static std::shared_ptr<DataType>
  getGrid(const std::string& aTypeName);

  /** Direct query by Lat lon */
  static float
  queryGrid(size_t key, AngleDegs lat, AngleDegs lon)
  {
    // FIXME: This will be slower than direct index, but less ram.
    // We might optimize this.
    if (key < myDataTypeCache.size()) {
      auto& d = myDataTypeCache[key];
      if (d) {
        auto p = d->getProjection();
        return (p->getValueAtLL(lat, lon));
      } else {
        return myDefaultValues[key];
      }
    }
    return 0; // what to do
  }

protected:

  // FIXME: maybe a class

  /** List of followed keys  */
  static std::vector<std::string> myKeys;

  /** List of followed TypeNames */
  static std::vector<std::string> myDataTypeNames;

  /** Cache of DataTypes, if existing */
  static std::vector<std::shared_ptr<DataType> > myDataTypeCache;

  /** Cache of default values */
  static std::vector<float> myDefaultValues;
};
}
