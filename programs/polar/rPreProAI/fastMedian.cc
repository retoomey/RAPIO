#include "fastMedian.h"
#include <rRadialSet.h>
#include <rConstants.h>
#include <algorithm>
#include <vector>
#include <cmath>

namespace rapio {
/**
 * Core Median Engine: Handles sorting, even/odd logic, and thresholding.
 * Extracted so it can be used by any N-dimensional window traversal.
 */
static float
computeMedianValue(std::vector<float>& window, size_t min_good_num, float currentVal,
  size_t rfCount, size_t snCount)
{
  size_t n = window.size();

  // Fast Median Calculation if we meet the data threshold
  if ((n >= min_good_num) && (n > 0)) {
    auto target = window.begin() + (n / 2);

    // O(n) selection
    std::nth_element(window.begin(), target, window.end());
    float upper_value = *target;

    // Handling even/odd logic
    if (n % 2 == 1) {
      return upper_value;
    } else {
      // Correctly dereference the iterator returned by max_element
      auto lower_value_iter = std::max_element(window.begin(), window.begin() + (n / 2));
      return (*lower_value_iter + upper_value) / 2.0f;
    }
  } else {
    // Fallback logic if we don't have enough valid data
    // If there were ANY valid or sentinel points in the window, guess the dominant sentinel
    if ((rfCount > 0) || (snCount > 0)) {
      if (rfCount >= snCount) {
        return Constants::RangeFolded;
      } else {
        return Constants::MissingData;
      }
    }
  }

  // Ultimate fallback: the window was completely DataUnavailable or out of bounds
  return currentVal;
} // computeMedianValue

/** * Performs a fast 2D median filter on a RadialSet (across Radials and Gates).
 */
void
applyFast2DMedian(std::shared_ptr<RadialSet> radialSet, int radialWin, int gateWin, float min_good_percent)
{
  if (!radialSet) { 
    fLogInfo( "applyFast2DMedian: invalid radialSet abort.");
    return; 
  }

  size_t numRadials = radialSet->getNumRadials();
  size_t numGates   = radialSet->getNumGates();

  // Access the 2D float grid
  auto& data  = radialSet->getFloat2D()->ref();
  auto output = data;

  // Calculate minimum required valid pixels
  size_t min_good_num = 0;

  if (min_good_percent > 1.0f) {
    fLogInfo("applyFast2DMedian: invalid good percentage > 1.0 must be between 0 and 1");
    min_good_num = static_cast<size_t>(std::floor((radialWin * gateWin) * (min_good_percent / 100.0f)));
  } else {
    min_good_num = static_cast<size_t>(std::floor((radialWin * gateWin) * min_good_percent));
  }

  int rHalf = radialWin / 2;
  int gHalf = gateWin / 2;

  for (size_t r = 0; r < numRadials; ++r) {
    for (size_t g = 0; g < numGates; ++g) {
      float currentVal = data[r][g];

      // Local counters for radar artifacts within this specific window
      size_t localRangeFoldedCount    = 0;
      size_t localBelowThresholdCount = 0;

      // Neighbor Collection
      std::vector<float> window;
      window.reserve(radialWin * gateWin);

      for (int i = -rHalf; i <= rHalf; ++i) {
        // Circular wrap-around for radials (Azimuth)
        int neighborR = (static_cast<int>(r) + i + numRadials) % numRadials;

        for (int j = -gHalf; j <= gHalf; ++j) {
          int neighborG = static_cast<int>(g) + j;

          // Boundary check for gates (Range)
          if ((neighborG >= 0) && (neighborG < static_cast<int>(numGates))) {
            float val = data[neighborR][neighborG];

            // Categorize the neighbor
            if (val == Constants::RangeFolded) {
              localRangeFoldedCount++;
            } else if (val == Constants::MissingData) {
              localBelowThresholdCount++;
            } else if (val != Constants::DataUnavailable) {
              // Only real meteorological data goes into the median pool
              window.push_back(val);
            }
          }
        }
      }

      output[r][g] = computeMedianValue(window, min_good_num, currentVal, localRangeFoldedCount,
          localBelowThresholdCount);
    }
  }

  // Write back the filtered data
  data = output;
} // applyFast2DMedian

/** * Performs a fast 1D median filter on a RadialSet (along Gates only).
 */
void
applyFast1DMedian_alongRadial(std::shared_ptr<RadialSet> radialSet, int gateWin, float min_good_percent)
{
  if (!radialSet) { 
    fLogInfo( "applyFast1DMedian_alongRadial: invalid radialSet abort.");
    return; 
  }

  size_t numRadials = radialSet->getNumRadials();
  size_t numGates   = radialSet->getNumGates();

  // Access the 2D float grid
  auto& data  = radialSet->getFloat2D()->ref();
  auto output = data;

  // Calculate minimum required valid pixels (1D window)
  size_t min_good_num = 0;

  if (min_good_percent > 1.0f) {
    fLogInfo("applyFast1DMedian_alongRadial: invalid good percentage > 1.0 must be between 0 and 1");
    min_good_num = static_cast<size_t>(std::floor(gateWin * (min_good_percent / 100.0f)));
  } else {
    min_good_num = static_cast<size_t>(std::floor(gateWin * min_good_percent));
  }

  int gHalf = gateWin / 2;

  for (size_t r = 0; r < numRadials; ++r) {
    for (size_t g = 0; g < numGates; ++g) {
      float currentVal = data[r][g];

      size_t localRangeFoldedCount    = 0;
      size_t localBelowThresholdCount = 0;

      // Neighbor Collection
      std::vector<float> window;
      window.reserve(gateWin);

      // 1D: Only iterate over the gates (j), keep radial (r) constant
      for (int j = -gHalf; j <= gHalf; ++j) {
        int neighborG = static_cast<int>(g) + j;

        // Boundary check for gates (Range)
        if ((neighborG >= 0) && (neighborG < static_cast<int>(numGates))) {
          float val = data[r][neighborG];

          // Categorize the neighbor
          if (val == Constants::RangeFolded) {
            localRangeFoldedCount++;
          } else if (val == Constants::MissingData) {
            localBelowThresholdCount++;
          } else if (val != Constants::DataUnavailable) {
            window.push_back(val);
          }
        }
      }

      output[r][g] = computeMedianValue(window, min_good_num, currentVal, localRangeFoldedCount,
          localBelowThresholdCount);
    }
  }

  // Write back the filtered data
  data = output;
} // applyFast1DMedian_alongRadial

/** * Performs a fast 1D median filter on a RadialSet across Radials (Azimuth only).
 */
void
applyFast1DMedian_acrossRadial(std::shared_ptr<RadialSet> radialSet, int radialWin, float min_good_percent)
{
  if (!radialSet) { 
    fLogInfo("applyFast1DMedian_acrossRadial: invalid radialSet abort.");
    return; 
  }

  size_t numRadials = radialSet->getNumRadials();
  size_t numGates   = radialSet->getNumGates();

  auto& data  = radialSet->getFloat2D()->ref();
  auto output = data;

  // Calculate minimum required valid pixels (1D window)
  size_t min_good_num = 0;

  if (min_good_percent > 1.0f) {
    fLogInfo("applyFast1DMedian_acrossRadial: invalid good percentage > 1.0 must be between 0 and 1");
    min_good_num = static_cast<size_t>(std::floor(radialWin * (min_good_percent / 100.0f)));
  } else {
    min_good_num = static_cast<size_t>(std::floor(radialWin * min_good_percent));
  }

  int rHalf = radialWin / 2;

  for (size_t r = 0; r < numRadials; ++r) {
    for (size_t g = 0; g < numGates; ++g) {
      float currentVal = data[r][g];

      size_t localRangeFoldedCount    = 0;
      size_t localBelowThresholdCount = 0;

      std::vector<float> window;
      window.reserve(radialWin);

      // 1D: Iterate over the radials (i) utilizing 360-degree wrap, keep gate (g) constant
      for (int i = -rHalf; i <= rHalf; ++i) {
        // Circular wrap-around for radials (Azimuth)
        int neighborR = (static_cast<int>(r) + i + numRadials) % numRadials;

        float val = data[neighborR][g];

        if (val == Constants::RangeFolded) {
          localRangeFoldedCount++;
        } else if (val == Constants::MissingData) {
          localBelowThresholdCount++;
        } else if (val != Constants::DataUnavailable) {
          window.push_back(val);
        }
      }

      output[r][g] = computeMedianValue(window, min_good_num, currentVal, localRangeFoldedCount,
          localBelowThresholdCount);
    }
  }

  data = output;
} // applyFast1DMedian_acrossRadial

// simple method find the median of a vector.
// does not handle mising or range folded data
float
findSimpleMedian(std::vector<float> nums)
{
  if (nums.empty()) {
    return Constants::MissingData;
  }

  int n       = nums.size();
  auto target = nums.begin() + (n / 2);

  // O(n) selection
  std::nth_element(nums.begin(), target, nums.end());
  float upper_value = *target;

  // Handling even/odd logic
  if (n % 2 == 1) {
    return upper_value;
  } else {
    // Correctly dereference the iterator returned by max_element
    auto lower_value_iter = std::max_element(nums.begin(), nums.begin() + (n / 2));
    return (*lower_value_iter + upper_value) / 2.0f;
  }
}
} // namespace rapio
