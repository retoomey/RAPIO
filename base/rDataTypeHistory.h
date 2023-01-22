#pragma once

#include <rData.h>
#include <rDataType.h>
#include <rRecord.h>
#include <rElevationVolume.h>

#include <memory>

#include <vector>

namespace rapio {
/** Store a history of objects of some sort.
 * API meant to be called for incoming data of interest.
 * FIXME: Maybe we go though DataType and/or flags to
 * auto do this?  This current way requires an algorithm
 * to call update commands to add to caches.
 *
 * The main algorithm purges the history cache based on the -h
 * history.
 *
 * @author Robert Toomey
 */
class DataTypeHistory : public Data {
public:

  /** Register a volume for a particular key (overriding the updateVolume creation method) */
  static void
  registerVolume(const std::string& key, std::shared_ptr<Volume> v);

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

  /** Purge history for volumes, collections, etc.  Called by
   * RAPIO automatically you don't need to call it.  Use the -h
   * option to determine max time window */
  static void
  purgeTimeWindow(const Time& time);

  /** New record stuff for now */
  static void
  addRecord(Record& rec);
protected:

  // Storage for volumes
  static std::map<std::string, std::shared_ptr<Volume> > myVolumes;
};
}
