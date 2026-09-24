#pragma once
#include "rVolume.h"

namespace rapio {
/**
 * @class VolumeOf1
 * @ingroup rapio_data
 * @brief A volume constraint that retains only the single latest DataType received.
 *
 * This implementation enforces a strict history size of one. When a new DataType
 * arrives that is chronologically newer or equal to the stored DataType, it overwrites
 * the existing data regardless of subtype. It is highly optimized for scenarios like
 * 2D projection fusion where only the most recent data layer is relevant.
 * @author Robert Toomey
 */
class VolumeOf1 : public Volume {
public:
  /** @brief Default constructor intended for STL/Factory prototype use only. */
  VolumeOf1() : Volume(""){ }

  /**
   * @brief Constructs a new VolumeOf1 utilizing the global history window.
   * @param k The unique identifier for this volume history.
   */
  VolumeOf1(const std::string& k) : Volume(k){ }

  /**
   * @brief Constructs a new VolumeOf1 with a custom expiration window.
   * @param k The unique identifier for this volume history.
   * @param customWindow The specific TimeDuration to keep data alive.
   */
  VolumeOf1(const std::string& k, const TimeDuration& customWindow) : Volume(k, customWindow){ }

  /**
   * @brief Initializes and registers this subclass with the central Factory.
   */
  static void
  introduceSelf()
  {
    std::shared_ptr<VolumeOf1> newOne = std::make_shared<VolumeOf1>();
    Factory<Volume>::introduce("one", newOne);
  }

  /**
   * @brief Retrieves the specific help string for VolumeOf1.
   * @param fkey The key under which the subclass was registered.
   * @return A brief description of the volume subclass behavior.
   */
  virtual std::string
  getHelpString(const std::string& fkey) override
  {
    return "Stores ONLY the single latest received subtype/tilt.";
  }

  /**
   * @brief Factory callback to generate a new VolumeOf1 instance.
   * @param historyKey The unique identifier assigned to the new volume instance.
   * @param params Additional parameters for initialization.
   * @return A shared pointer to the newly created VolumeOf1.
   */
  virtual std::shared_ptr<Volume>
  create(
    const std::string& historyKey, const std::string & params) override
  {
    return std::make_shared<VolumeOf1>(historyKey);
  }

  /**
   * @brief Adds or replaces the DataType.
   * If the group already contains an item, it is overwritten if the incoming
   * DataType is chronologically newer or equal.
   * @param d The DataType to store.
   */
  virtual void
  addDataType(std::shared_ptr<DataType> d) override;

  /**
   * @brief Generates a highly optimized pointer cache configured specifically for
   * a single-layer depth, automatically resolving bounds queries to hit this layer.
   * @param c The cache to populate.
   */
  virtual void
  getTempPointerVector(VolumePointerCache& c) override;
};
} // namespace rapio
