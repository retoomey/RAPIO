#pragma once

#include <rDataTypeGroup.h>
#include <rDataType.h>
#include <rFactory.h>
#include <rRAPIOOptions.h>
#include <rVolumeValue.h>
#include <rStrings.h>

#include <memory>
#include <vector>

#include <fmt/format.h>

namespace rapio {
/**
 * @class VolumePointerCache
 * @ingroup rapio_data
 * @brief Fast temp pointer cache for using the current volume information.
 *
 * This cache remains valid only until the volume changes. It is heavily utilized
 * during a single volume query pass (e.g., in a fusion or merger algorithm) to
 * avoid the overhead of repeated map or vector lookups.
 * @author Robert Toomey
 */
class VolumePointerCache {
public:
  /** @brief Subtypes (typically elevations) maintained in sorted order. */
  std::vector<double> levels;

  /** @brief Pre-fetched DataType pointer cache corresponding to each item. */
  std::vector<DataTypePointerCache *> pc;
};

/**
 * @class Volume
 * @ingroup rapio_data
 * @brief Manages a collection of space-unique common DataTypes.
 *
 * Volumes are named by product and contain N subtypes. For example, a
 * Reflectivity volume might contain 10 elevations that can be used for
 * interpolating values in space and time (such as CAPPI or VSLICE generation).
 * @author Robert Toomey
 */
class Volume : public DataTypeGroup {
public:

  /**
   * @brief Default constructor intended for STL and Factory prototype use only.
   */
  Volume() : DataTypeGroup(""){ }

  /**
   * @brief Constructs a new Volume utilizing the global history window.
   * @param k The unique identifier for this volume history.
   */
  Volume(const std::string& k) : DataTypeGroup(k){ }

  /**
   * @brief Constructs a new Volume with a custom expiration window.
   * @param k The unique identifier for this volume history.
   * @param customWindow The specific TimeDuration to keep data alive.
   */
  Volume(const std::string& k, const TimeDuration& customWindow) : DataTypeGroup(k, customWindow){ }

  virtual
  ~Volume() = default;

  /**
   * @brief Generates the item key for the Volume based on the DataType's subtype.
   * @param dt The DataType to evaluate.
   * @return The SubType string (e.g., the elevation angle).
   */
  virtual std::string
  generateItemKey(const std::shared_ptr<DataType>& dt) const override
  {
    return dt->getSubType();
  }

  /**
   * @brief Initializes the factory registry with default built-in RAPIO Volume subclasses.
   */
  static void
  introduceSelf();

  /**
   * @brief Registers a new Volume type with the factory.
   * @param key The lookup key for the factory.
   * @param factory The Volume instance to act as a prototype.
   */
  static void
  introduce(const std::string & key, std::shared_ptr<Volume> factory);

  /**
   * @brief Returns dynamically generated help information detailing available Volume types.
   * @return A formatted help string.
   */
  static std::string
  introduceHelp();

  /**
   * @brief Binds registered Volume factory options as suboptions to a command-line argument.
   * @param name The primary command-line option name (e.g., "volume").
   * @param o The options parser to populate.
   */
  static void
  introduceSuboptions(const std::string& name, RAPIOOptions& o);

  /**
   * @brief Instantiates a specific Volume by querying the factory registry.
   * @param key The registered name of the Volume type (e.g., "simple", "one").
   * @param params Additional parameters for initialization.
   * @param historyKey The unique identifier assigned to this volume instance.
   * @return A shared pointer to the created Volume.
   */
  static std::shared_ptr<Volume>
  createVolume(
    const std::string & key,
    const std::string & params,
    const std::string & historyKey);

  /**
   * @brief Retrieves the specific help string for this volume subclass.
   * @param fkey The key under which the subclass was registered.
   * @return A brief description of the volume subclass behavior.
   */
  virtual std::string
  getHelpString(const std::string& fkey) = 0;

  /**
   * @brief Factory callback implemented by subclasses to generate a new instance.
   * @param historyKey The unique identifier assigned to the new volume instance.
   * @param params Additional parameters for initialization.
   * @return A shared pointer to the newly created Volume.
   */
  virtual std::shared_ptr<Volume>
  create(
    const std::string& historyKey, const std::string & params) = 0;

  /**
   * @brief Retrieves a stored DataType matching the specified subtype.
   * @param subtype The subtype identifier (e.g., elevation angle).
   * @return A shared pointer to the DataType, or nullptr if not found.
   */
  std::shared_ptr<DataType>
  getSubType(const std::string& subtype)
  {
    return getDataType(subtype);
  }

  /**
   * @brief Removes a stored DataType matching the specified subtype.
   * @param subtype The subtype identifier (e.g., elevation angle).
   * @return True if the item was found and removed, false otherwise.
   */
  bool
  deleteSubType(const std::string& subtype)
  {
    return removeDataType(subtype);
  }

  /**
   * @brief Exposes the underlying vector of DataTypes for iteration.
   * @return Constant reference to the item vector.
   */
  const std::vector<std::shared_ptr<DataType> >&
  getVolume() const
  {
    return myItems;
  }

  /**
   * @brief Populates a temporary pointer cache to drastically accelerate loop-based queries.
   * @param c The cache to populate.
   */
  virtual void
  getTempPointerVector(VolumePointerCache& c) = 0;

  /**
   * @brief Performs a highly optimized linear search to locate the bounding slices
   * (above and below) for a given virtual elevation.
   * @param c The populated VolumePointerCache.
   * @param v The VolumeValue context containing the target virtual elevation and
   * where the resolved pointers will be stored.
   */
  static inline void
  getSpreadL(const VolumePointerCache& c, VolumeValue& v)
  {
    const std::vector<double>& subtypes = c.levels;
    const auto& pcp = c.pc;

    float& at      = v.virtualElevDegs;
    const size_t s = subtypes.size();

    for (size_t i = 0; i < s; i++) {
      if (at < subtypes[i]) {
        v.setPC(VolumeValue::Layer::Lower2, pcp[i]);
        v.setPC(VolumeValue::Layer::Lower, pcp[i + 1]);
        v.setPC(VolumeValue::Layer::Upper, pcp[i + 2]);
        v.setPC(VolumeValue::Layer::Upper2, pcp[i + 3]);
        return;
      }
    }
    v.clearPC();
  }
};

/** @brief Formats a Volume for standard output streams. */
std::ostream&
operator << (std::ostream&, const rapio::Volume&);
} // namespace rapio

/** @brief Specializes fmt::formatter to support logging Volume objects via fmtlib. */
template <>
struct fmt::formatter<rapio::Volume> {
  constexpr auto parse(fmt::format_parse_context& ctx){ return ctx.begin(); }

  template <typename FormatContext>
  auto
  format(const rapio::Volume& v, FormatContext& ctx) const
  {
    rapio::Time latest(0);
    std::vector<double> out1;
    std::vector<rapio::Time> times;

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

    auto out = fmt::format_to(ctx.out(), "Current Virtual Volume: ");

    for (size_t c = 0; c < out1.size(); c++) {
      out = fmt::format_to(out, "{}", out1[c]);
      if (times[c] == latest) {
        out = fmt::format_to(out, " (latest)");
      }
      if (c < out1.size() - 1) {
        out = fmt::format_to(out, ", ");
      }
    }
    return out;
  } // format
};
