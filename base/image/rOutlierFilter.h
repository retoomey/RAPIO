#pragma once

#include <rArrayFilter.h>
#include <rConstants.h>
#include <string>
#include <vector>
#include <memory>

namespace rapio {
/**
 * @class OutlierFilter
 * @brief Removes statistical outliers from a 1D dataset based on Z-score.
 *
 * @details Calculates the mean and standard deviation of valid data points
 * in the sequence. Any value whose Z-score exceeds the defined threshold
 * is replaced by either the previous valid value or a fallback value.
 * Missing or unavailable data sentinels are ignored during calculations.
 *
 * @ingroup rapio_image
 */
class OutlierFilter : public ArrayFilter {
public:
  OutlierFilter() = default;
  virtual
  ~OutlierFilter() = default;

  /**
   * @brief Registers this filter with the factory.
   */
  static void
  introduceSelf();

  std::string
  getHelpString() override;
  bool
  parseOptions(const std::string& params) override;

  bool
  supportsDimensions(size_t dims) const override
  {
    return (dims == 1);
  }

  /**
   * @brief Processes a 1D RAPIO Array.
   * @param src The source data array.
   * @param dst The destination data array.
   */
  void
  process1D(const std::shared_ptr<Array<float, 1> >& src,
    const std::shared_ptr<Array<float, 1> >        & dst) override;

  /**
   * @brief Processes a standard 1D std::vector.
   * @param src The source data vector.
   * @param dst The destination data vector.
   */
  void
  process1D(const std::vector<float>& src,
    std::vector<float>              & dst) override;

private:

  /**
   * @brief Internal core processing logic decoupled from data containers.
   * @param srcData Pointer to the contiguous source memory.
   * @param dstData Pointer to the contiguous destination memory.
   * @param size The number of elements to process.
   */
  void
  process1DRaw(const float * srcData, float * dstData, size_t size);

  float myZThreshold = 1.0f;
  float myFallback   = 0.0f;
};
} // namespace rapio
