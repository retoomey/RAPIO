//#include <rArrayFilter.h>
#include "computeFilter.h" 
#include <numeric>
#include <cmath>
#include <rError.h>
#include <rConstants.h>
#include <rRadialSet.h>

namespace rapio {

static float
computeAveValue(std::vector<float>& window, size_t min_good_num, float currentVal, size_t rfCount, size_t snCount)
{
  size_t n = window.size();
  // Average Calculation if we meet the data threshold
  if ((n >= min_good_num) && (n > 0)) {
    float sum = std::accumulate(window.begin(), window.end(), 0LL); 
    return sum/ (float) n;

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

static float
computeMaxValue(std::vector<float>& window, size_t min_good_num, float currentVal, size_t rfCount, size_t snCount)
{
   
  size_t n = window.size();
  // Average Calculation if we meet the data threshold
  if ((n >= min_good_num) && (n > 0)) {
    // 2. Find the max element. 
    // Note: This returns an ITERATOR pointing to the max value, not the value itself.
    auto max_it = std::max_element(window.begin(), window.end());

    // 3. Dereference the iterator (using *) to get the actual float value
    return *max_it; 

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


std::shared_ptr<rapio::RadialSet>  apply2DBlurFilter( std::shared_ptr<rapio::RadialSet> & input_radialSet, int radialWin, int gateWin, float min_good_percent) {
  if (!input_radialSet) {
    fLogInfo( "apply2DBlurFilter: invalid radialSet abort.");
    return nullptr;
  }

  auto radialSet = input_radialSet->Clone();
  size_t numRadials = radialSet->getNumRadials();
  size_t numGates   = radialSet->getNumGates();

  // Access the 2D float grid
  auto& data  = radialSet->getFloat2DRef();
  auto output = data; 

    // Calculate minimum required valid pixels
  size_t min_good_num = 0;

  if (min_good_percent > 1.0f) {
    fLogInfo("apply2DBlurFilter: invalid good percentage > 1.0 must be between 0 and 1");
    min_good_num = static_cast<size_t>(std::floor((radialWin * gateWin) * (min_good_percent / 100.0f)));
  } else {
    min_good_num = static_cast<size_t>(std::floor((radialWin * gateWin) * min_good_percent));
  }

//
  int rHalf = radialWin/ 2; //integer math on purpose 
  int gHalf = gateWin/ 2;

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
              // Only real meteorological data goes into the window pool
              window.push_back(val);
            }
          }
        }
      }
          output[r][g] = computeAveValue(window, min_good_num, currentVal, localRangeFoldedCount, localBelowThresholdCount);
    }
    }
    data = output;
    return radialSet;
}



std::shared_ptr<rapio::RadialSet>  apply2DDilationFilter( std::shared_ptr<rapio::RadialSet> & input_radialSet, int radialWin, int gateWin, float min_good_percent) {


  auto radialSet = input_radialSet->Clone();

  if (!radialSet) {
    fLogInfo( "apply2DDilationFilter: invalid radialSet abort.");
    return nullptr;
  }

 
  size_t numRadials = radialSet->getNumRadials();
  size_t numGates   = radialSet->getNumGates();

  // Access the 2D float grid
  auto& data  = radialSet->getFloat2DRef();
  auto output = data; 

    // Calculate minimum required valid pixels
  size_t min_good_num = 0;

  if (min_good_percent > 1.0f) {
    fLogInfo("apply2DDilationFilter: invalid good percentage > 1.0 must be between 0 and 1");
    min_good_num = static_cast<size_t>(std::floor((radialWin * gateWin) * (min_good_percent / 100.0f)));
  } else {
    min_good_num = static_cast<size_t>(std::floor((radialWin * gateWin) * min_good_percent));
  }

//
  int rHalf = radialWin/ 2; //integer math on purpose 
  int gHalf = gateWin/ 2;

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
              // Only real meteorological data goes into the window pool
              window.push_back(val);
            }
          }
        }
      }
          output[r][g] = computeMaxValue(window, min_good_num, currentVal, localRangeFoldedCount, localBelowThresholdCount);
    }
    }
    data = output;
    return radialSet;
}

}//rapio 
