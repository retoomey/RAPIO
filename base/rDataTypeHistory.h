#pragma once

#include <rData.h>
#include <rDataType.h>
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

  /** Update the virtual volume for this DataType if possible,
   * Volumes are named by product and contain N subtypes, for example
   * a Reflectivity volume containing 10 elevations that can be used for
   * interpolating values in space time (CAPPI/VSLICE)
   */
  static void
  updateVolume(std::shared_ptr<DataType> data);

  // Any other type of history/storage we can have functions added here

  /** Purge history for volumes, collections, etc.  Called by
   * RAPIO automatically you don't need to call it.  Use the -h
   * option to determine max time window */
  static void
  purgeTimeWindow(const Time& time);

protected:

  // Storage for volumes
  static std::map<std::string, std::shared_ptr<Volume> > myVolumes;
};
}
