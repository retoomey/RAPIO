#pragma once

#include <rData.h>
#include <rDataType.h>

#include <memory>
#include <vector>

namespace rapio {
/* Volumes are named by product and contain N subtypes, for example
 * a Reflectivity volume containing 10 elevations that can be used for
 * interpolating values in space time (CAPPI/VSLICE)
 * Newer subtypes replace older ones, subtypes older than the
 * time window are removed by a call from the main algorithm as new
 * data comes in.
 *
 * @author Robert Toomey
 */
class Volume : public Data {
public:

  /** Create a new volume */
  Volume(const std::string& k) : myKey(k){ };

  /** Purge data base using given time as current.
   * FIXME: out of time order data will jitter here */
  void
  purgeTimeWindow(const Time& fromTime);

  /** Print the volume information out */
  void
  printVolume();

  /** Add a datatype to our collection */
  virtual void
  addDataType(std::shared_ptr<DataType> d);

  /** Get DataType matching a given subtype */
  std::shared_ptr<DataType>
  getSubType(const std::string& subtype);

  /** Delete DataTypes matching a given subtype */
  bool
  deleteSubType(const std::string& subtype);

private:

  /** Unique key for volume used by history */
  std::string myKey;

  /** The current volume we store */
  std::vector<std::shared_ptr<DataType> > myVolume;
};

/** An elevation volume stores a time-window based history for a particular
 * Radar and moment when each item of the set has a unique elevation.
 *
 * Key                 TimeSet
 * "KTLX Reflectivity" (0.50, 1.50, etc..)
 *
 * @author Robert Toomey */
class ElevationVolume : public Volume {
public:

  /** Create a new elevation volume */
  ElevationVolume(const std::string& k) : Volume(k){ };

  /** Add a datatype to our collection */
  virtual void
  addDataType(std::shared_ptr<DataType> d) override;
};
}
