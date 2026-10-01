#pragma once

#include <rArrayFilter.h>
#include <rConstants.h>
#include <string>
#include <vector>
#include <memory>

namespace rapio {
/**
 * @class SavitzkyGolayFilter
 * @brief Applies a 1D Savitzky-Golay smoothing filter.
 *
 * @details This filter applies a 7-point causal Savitzky-Golay smoothing
 * algorithm to 1D data arrays or vectors. It is useful for smoothing noisy
 * signal data while preserving the shape and height of signal peaks better
 * than a simple moving average.
 *
 * @ingroup rapio_image
 */
class SavitzkyGolayFilter : public ArrayFilter {
public:
  SavitzkyGolayFilter() = default;
  virtual
  ~SavitzkyGolayFilter() = default;

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
};
} // namespace rapio
