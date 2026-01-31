#pragma once

#include <rDataType.h>
#include <rFactory.h>
#include <rRAPIOOptions.h>
#include <rVolumeValue.h>
#include <rStrings.h>

#include <memory>
#include <vector>

#include <fmt/format.h>

namespace rapio {
/** Fast temp pointer cache for using the current volume information.
 * Only good until volume changes.  Used during a single volume query
 * pass say in fusion
 * @ingroup rapio_data
 * @brief Caches volume data pointers for fast access in loops.
 **/
class VolumePointerCache {
public:
  /** Subtypes in order */
  std::vector<double> levels;

  /** DataTypePointerCache per item*/
  std::vector<DataTypePointerCache *> pc;
};

/* Volumes are named by product and contain N subtypes, for example
 * a Reflectivity volume containing 10 elevations that can be used for
 * interpolating values in space time (CAPPI/VSLICE)
 *
 * Note: How this collection of DataTypes is handled is determined by the subclass.
 *
 * @author Robert Toomey
 * @ingroup rapio_data
 * @brief Stores a collection of space unique common DataTypes.
 */
class Volume {
public:

  /** For STL use only. */
  Volume()
  { }

  /** Create a new volume */
  Volume(const std::string& k) : myKey(k){ };

  /** Destruction */
  virtual
  ~Volume() = default;

  // --------------------------------------------------------
  // Factory methods for doing things by name.  Usually if you
  // want to support command line choosing of a Volume
  // algorithm.

  /** Use this to introduce default built-in RAPIO Volume subclasses.
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

  /** Introduce dynamic help */
  static std::string
  introduceHelp();

  /** Introduce suboptions */
  static void
  introduceSuboptions(const std::string& name, RAPIOOptions& o);

  /** Attempt to create Volume by name.  This looks up any registered
   * classes and attempts to call the virtual create method on it. */
  static std::shared_ptr<Volume>
  createVolume(
    const std::string & key,
    const std::string & params,
    const std::string & historyKey);

  // --------------------------------------------------------

  /** Help function, subclasses return help information. */
  virtual std::string
  getHelpString(const std::string& fkey) = 0;

  /** Called on subclasses by the Volume to create/setup the Volume.
   * To use by name, you would override this method and return a new instance of your
   * Volume class. */
  virtual std::shared_ptr<Volume>
  create(
    const std::string& historyKey, const std::string & params) = 0;

  /** Add a datatype to our collection, default does nothing */
  virtual void
  addDataType(std::shared_ptr<DataType> d) = 0;

  /** Get DataType from our volume matching a given subtype, if any */
  std::shared_ptr<DataType>
  getSubType(const std::string& subtype);

  /** Delete DataType in our volume matching a given subtype, if any */
  bool
  deleteSubType(const std::string& subtype);

  /** Clear the entire volume */
  void
  clearVolume(){ myVolume.clear(); }

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

  // --------------------------------------------------------
  // Iteratation speed utilities
  //

  /** Return a temp pointer vector.  This is a quick vector for looping that assumes
   * the shared ptrs and this object stay in scope and unchanged.  Used for speed in loops
   * calling the getSpread search below */
  virtual void
  getTempPointerVector(VolumePointerCache&) = 0;

  /** Linear search (faster for smaller N) Spread above and below from number vector.  Pointers for speed.
   * This is called during fusion/merger grid a billion times so we want to optimize here. */
  static inline void
  getSpreadL(
    const VolumePointerCache& c,
    VolumeValue             & v)
  {
    // Inline calls should be quick references.
    const std::vector<double>& subtypes = c.levels;
    const auto& pcp = c.pc;

    float& at = v.virtualElevDegs;

    // 10 20 30 40 50 60
    // 5 --> nullptr, 10
    // 10 --> 10 20
    // 15 --> 10 20
    // 65 --> 60 nullptr
    const size_t s = subtypes.size();

    for (size_t i = 0; i < s; i++) {
      if (at < subtypes[i]) { // Down to just one branch
        // We padded to optimize the loop, like so:
        // 0  1  2  3  4  5  6  7  8  9
        // NP NP 10 20 30 40 50 60 NP NP  (padding)
        // 10 20 30 40 50 60 MAX_DOUBLE
        // for 29, we have i == 2 since 29 < 30 ---> then:
        // The lower is i - 1 at 20, next lower i - 2;
        // The upper is i, the next upper is i + 1;

        // The pointer cache for the datatype
        v.setPC(VolumeValue::Layer::Lower2, pcp[i]);
        v.setPC(VolumeValue::Layer::Lower, pcp[i + 1]);
        v.setPC(VolumeValue::Layer::Upper, pcp[i + 2]);
        v.setPC(VolumeValue::Layer::Upper2, pcp[i + 3]);
        return;
      }
    }
    // needed for s = 0;
    v.clearPC();
  }; // getSpreadNew

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

/** A volume that stores the single latest DataType received (ignoring subtype).
 * Having only one might allow us to speed things up in this unique case.
 * This is used for the 2D projection fusion that will use a single DataType
 * Newer time on incoming replaces the single older one, subtypes older than the
 * time window are removed by a call from the main algorithm as new
 * data comes in.*/
class VolumeOf1 : public Volume {
public:
  /** For STL use only. */
  VolumeOf1()
  { }

  /** Create a new volume of 1 */
  VolumeOf1(const std::string& k) : Volume(k){ };

  /** Introduce volume of N simple type */
  static void
  introduceSelf()
  {
    std::shared_ptr<VolumeOf1> newOne = std::make_shared<VolumeOf1>();
    Factory<Volume>::introduce("one", newOne);
  };

  /** Help function, subclasses return help information. */
  virtual std::string
  getHelpString(const std::string& fkey) override { return "Stores ONLY the single latest received subtype/tilt."; }

  /** Called on subclasses by the Volume to create/setup the Volume.
   * To use by name, you would override this method and return a new instance of your
   * Volume class. */
  virtual std::shared_ptr<Volume>
  create(
    const std::string& historyKey, const std::string & params) override
  {
    return std::make_shared<VolumeOf1>(VolumeOf1(historyKey));
  }

  /** Add a datatype to our collection */
  virtual void
  addDataType(std::shared_ptr<DataType> d) override;

  /** Return a temp pointer vector.  This is a quick vector for looping that assumes
   * the shared ptrs and this object stay in scope and unchanged.  Used for speed in loops
   * calling the getSpread search below */
  virtual void
  getTempPointerVector(VolumePointerCache&) override;
};

/** A volume that stores N Datatypes by unique subtype.
 * For example, for RadialSets we might get Radar and moment when
 * each item of the set has a unique elevation.
 *
 * Key                 TimeSet
 * "KTLX Reflectivity" (0.50, 1.50, etc..)
 *
 * Newer subtypes replace older ones, subtypes older than the
 * time window are removed by a call from the main algorithm as new
 * data comes in.
 *
 * The collection is sorted by subtype from low to high, for the
 * search routines.
 */
class VolumeOfN : public Volume {
public:
  /** For STL use only. */
  VolumeOfN()
  { }

  /** Create a new volume of N */
  VolumeOfN(const std::string& k) : Volume(k){ };

  /** Introduce volume of N simple type */
  static void
  introduceSelf()
  {
    std::shared_ptr<VolumeOfN> newOne = std::make_shared<VolumeOfN>();
    Factory<Volume>::introduce("simple", newOne);
  };

  /** Help function, subclasses return help information. */
  virtual std::string
  getHelpString(const std::string& fkey) override
  {
    return "Stores all unique subtypes/tilts (Note: VCP ignored, virtual volume).";
  }

  /** Called on subclasses by the Volume to create/setup the Volume.
   * To use by name, you would override this method and return a new instance of your
   * Volume class. */
  virtual std::shared_ptr<Volume>
  create(
    const std::string& historyKey, const std::string & params) override
  {
    return std::make_shared<VolumeOfN>(VolumeOfN(historyKey));
  }

  /** Add a datatype to our collection */
  virtual void
  addDataType(std::shared_ptr<DataType> d) override;

  /** Return a temp pointer vector.  This is a quick vector for looping that assumes
   * the shared ptrs and this object stay in scope and unchanged.  Used for speed in loops
   * calling the getSpread search below */
  virtual void
  getTempPointerVector(VolumePointerCache&) override;
};

/** Output a Volume */
std::ostream&
operator << (std::ostream&,
  const rapio::Volume&);
}

/** Format library support, allows fLogInfo("Volume {}", volume) */
template <>
struct fmt::formatter<rapio::Volume> {
  // Standard parse function
  constexpr auto parse(fmt::format_parse_context& ctx){ return ctx.begin(); }

  template <typename FormatContext>
  auto
  format(const rapio::Volume& v, FormatContext& ctx) const
  {
    rapio::Time latest(0);
    std::vector<double> out1;
    std::vector<rapio::Time> times;

    // Collect data and find the latest timestamp
    for (auto const& x : v.getVolume()) {
      const auto os_str = rapio::Strings::removeNonNumber(x->getSubType());
      double d = 0;
      try {
        d = std::stod(os_str);
      } catch (const std::exception& e) { }

      rapio::Time newer = x->getTime();
      if (newer > latest) {
        latest = newer;
      }
      out1.push_back(d);
      times.push_back(newer);
    }

    // Start formatting to the output buffer
    auto out = fmt::format_to(ctx.out(), "Current Virtual Volume: ");

    for (size_t c = 0; c < out1.size(); c++) {
      out = fmt::format_to(out, "{}", out1[c]);

      if (times[c] == latest) {
        out = fmt::format_to(out, " (latest)");
      }

      // Add comma separator except for the last element
      if (c < out1.size() - 1) {
        out = fmt::format_to(out, ", ");
      }
    }

    return out;
  } // format
};
