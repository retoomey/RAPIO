#pragma once
#include "rVolume.h"

namespace rapio {
/**
 * @class VolumeOfN
 * @ingroup rapio_data
 * @brief A standard volume that stores N DataTypes, keyed and sorted by unique subtype.
 *
 * A common use case involves RadialSets where each item of the set corresponds
 * to a distinct elevation angle (subtype). Subtypes are maintained in sorted order
 * (low to high) to facilitate extremely fast bounding searches during vertical
 * interpolation. Newer data matching an existing subtype replaces the old data,
 * and old data is purged by the time window constraint.
 * @author Robert Toomey
 */
class VolumeOfN : public Volume {
public:
  /** @brief Default constructor intended for STL/Factory prototype use only. */
  VolumeOfN() : Volume(""){ }

  /**
   * @brief Constructs a new VolumeOfN utilizing the global history window.
   * @param k The unique identifier for this volume history.
   */
  VolumeOfN(const std::string& k) : Volume(k){ }

  /**
   * @brief Constructs a new VolumeOfN with a custom expiration window.
   * @param k The unique identifier for this volume history.
   * @param customWindow The specific TimeDuration to keep data alive.
   */
  VolumeOfN(const std::string& k, const TimeDuration& customWindow) : Volume(k, customWindow){ }

  /**
   * @brief Initializes and registers this subclass with the central Factory.
   */
  static void
  introduceSelf()
  {
    std::shared_ptr<VolumeOfN> newOne = std::make_shared<VolumeOfN>();
    Factory<Volume>::introduce("simple", newOne);
  }

  /**
   * @brief Retrieves the specific help string for VolumeOfN.
   * @param fkey The key under which the subclass was registered.
   * @return A brief description of the volume subclass behavior.
   */
  virtual std::string
  getHelpString(const std::string& fkey) override
  {
    return "Stores all unique subtypes/tilts (Note: VCP ignored, virtual volume).";
  }

  /**
   * @brief Factory callback to generate a new VolumeOfN instance.
   * @param historyKey The unique identifier assigned to the new volume instance.
   * @param params Additional parameters for initialization.
   * @return A shared pointer to the newly created VolumeOfN.
   */
  virtual std::shared_ptr<Volume>
  create(
    const std::string& historyKey, const std::string & params) override
  {
    return std::make_shared<VolumeOfN>(historyKey);
  }

  /**
   * @brief Adds a DataType to the collection using insertion sort.
   * Automatically replaces data if the subtype already exists, or inserts it
   * into the correct sorted position to maintain order.
   * @param d The DataType to store.
   */
  virtual void
  addDataType(std::shared_ptr<DataType> d) override;

  /**
   * @brief Generates a multi-layer pointer cache populated with all current
   * active levels and pointers, heavily padded for rapid boundary lookups.
   * @param c The cache to populate.
   */
  virtual void
  getTempPointerVector(VolumePointerCache& c) override;
};
} // namespace rapio
