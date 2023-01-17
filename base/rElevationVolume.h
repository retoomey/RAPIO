#pragma once

#include <rData.h>
#include <rDataType.h>
#include <rFactory.h>

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

  /** For STL use only. */
  Volume()
  { }

  /** Create a new volume */
  Volume(const std::string& k) : myKey(k){ };

  // --------------------------------------------------------
  // Factory methods for doing things by name.  Usually if you
  // want to support command line choosing of a Volume
  // algorithm.

  /** Use this to introduce default built-in RAPIO TerrainBlockage subclasses.
   * Note: You don't have to use this ability, it's not called by default algorithm.
   * To use it, call Volume::introduceSelf()
   * To override or add another, call Volume::introduce(myvolumekey, newvolume)
   */
  static void
  introduceSelf();

  /** Introduce a new Volume by name */
  static void
  introduce(const std::string & key,
    std::shared_ptr<Volume>   factory);

  /** Attempt to create Volume by name.  This looks up any registered
   * classes and attempts to call the virtual create method on it. */
  static std::shared_ptr<Volume>
  createVolume(
    const std::string & key,
    const std::string & params,
    const std::string & historyKey);

  /** Called on subclasses by the Volume to create/setup the Volume.
   * To use by name, you would override this method and return a new instance of your
   * Volume class. */
  virtual std::shared_ptr<Volume>
  create(
    const std::string& historyKey, const std::string & params)
  {
    return nullptr;
  }

  /** Purge data base using given time as current.
   * FIXME: out of time order data will jitter here */
  void
  purgeTimeWindow(const Time& fromTime);

  /** Get the key */
  std::string
  getKey() const { return myKey; }

  /** Get the volume list */
  const std::vector<std::shared_ptr<DataType> >&
  getVolume() const { return myVolume; }

  /** Add a datatype to our collection */
  virtual void
  addDataType(std::shared_ptr<DataType> d);

  /** Return a temp pointer vector.  This is a quick vector for looping that assumes
   * the shared ptrs and this object stay in scope and unchanged.  Used for speed in loops
   * calling the getSpread search below */
  void
  getTempPointerVector(std::vector<double>& levels, std::vector<DataType *>& pointers);

  /** Binary search (large N) Spread above and below from number vector.  Pointers for speed,
   * It's slow for small N not sure I'm gonna keep this function.  I'll keep it for now.
   * @deprecated */
  void
  getSpread(float at, const std::vector<double>& lookup, DataType *& lower, DataType *& upper, bool print = false);

  /** Linear search (faster for smaller N) Spread above and below from number vector.  Pointers for speed.
   * This is called during fusion/merger grid a billion times so we want to optimize here. */
  inline void
  getSpreadL(float at, const std::vector<double>& numbers, const std::vector<DataType *>& pointers, DataType *& lower,
    DataType *& upper)
  {
    // 10 20 30 40 50 60
    // 5 --> nullptr, 10
    // 10 --> 10 20
    // 15 --> 10 20
    // 65 --> 60 nullptr
    const size_t s = numbers.size();

    for (size_t i = 0; i < s; i++) {
      if (at < numbers[i]) { // Down to just one branch
        // We padded to optimize the loop, like so:
        // nullptr 10 20 30 40 50 60         nullptr
        // 10      20 30 40 50 60 MAX_DOUBLE
        lower = pointers[i];
        upper = pointers[i + 1];
        return;
      }
    }
    lower = upper = nullptr; // needed for s = 0
  };

  /** Get DataType matching a given subtype */
  std::shared_ptr<DataType>
  getSubType(const std::string& subtype);

  /** Delete DataTypes matching a given subtype */
  bool
  deleteSubType(const std::string& subtype);

protected:

  /** Remove at given index */
  virtual void
  removeAt(size_t at);

  /** Replace at given index */
  virtual void
  replaceAt(size_t at, std::shared_ptr<DataType> dt);

  /** Insert at given index */
  virtual void
  insertAt(size_t at, std::shared_ptr<DataType> dt);

  /** Add at given index */
  virtual void
  add(std::shared_ptr<DataType> dt);

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

  /** Introduce elevation volume simple type */
  static void
  introduceSelf();

  /** For STL use only. */
  ElevationVolume()
  { }

  /** Create a new elevation volume */
  ElevationVolume(const std::string& k) : Volume(k){ };

  /** Called on subclasses by the Volume to create/setup the Volume.
   * To use by name, you would override this method and return a new instance of your
   * Volume class. */
  virtual std::shared_ptr<Volume>
  create(
    const std::string& historyKey, const std::string & params)
  {
    return std::make_shared<ElevationVolume>(ElevationVolume(historyKey));
  }

  /** Add a datatype to our collection */
  virtual void
  addDataType(std::shared_ptr<DataType> d) override;
};

/** Output a Volume */
std::ostream&
operator << (std::ostream&,
  const rapio::Volume&);
}
