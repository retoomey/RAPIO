#include "calc_kdp.h"
#include "fastMedian.h"
#include <rRadialSet.h>
#include <rError.h>
#include <rConstants.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>
#include <utility>

namespace rapio {
namespace {
// Note: This is the LOCAL namespace on only used inside this file.
// -------------------------------------------------------------------------
// Helper: Fast Median Filter (1D), FIXME: Add to FastMedian.cc (simple_median)
// -------------------------------------------------------------------------
void
median_filter(int filter_length, const std::vector<float>& in, std::vector<float>& out)
{
  int n        = in.size();
  int half_len = filter_length / 2;

  out.resize(n, Constants::MissingData);

  std::vector<float> window;

  window.reserve(filter_length);

  for (int i = 0; i < n; ++i) {
    window.clear();
    int start = std::max(0, i - half_len);
    int end   = std::min(n - 1, i + half_len);

    for (int j = start; j <= end; ++j) {
      if (Constants::isGood(in[j])) {
        window.push_back(in[j]);
      }
    }

    if (window.size() > static_cast<size_t>(half_len)) {
      out[i] = findSimpleMedian(window);
    }
  }
}

// -------------------------------------------------------------------------
// Helper: Calculate Standard Deviation using Median as Mean
// -------------------------------------------------------------------------
void
calc_stddev(int filter_length, const std::vector<float>& diff, std::vector<float>& out)
{
  int n        = diff.size();
  int half_len = filter_length / 2;

  out.resize(n, Constants::MissingData);

  for (int i = 0; i < n; ++i) {
    int start = std::max(0, i - half_len);
    int end   = std::min(n - 1, i + half_len);

    float sum = 0.0f;
    int count = 0;

    // Calculate mean
    for (int j = start; j <= end; ++j) {
      if (Constants::isGood(diff[j])) {
        sum += diff[j];
        count++;
      }
    }

    if (count > half_len) {
      float mean    = sum / count;
      float var_sum = 0.0f;
      for (int j = start; j <= end; ++j) {
        if (Constants::isGood(diff[j])) {
          float v = diff[j] - mean;
          var_sum += v * v;
        }
      }
      out[i] = std::sqrt(var_sum / (count - 1));
    }
  }
} // calc_stddev

// -------------------------------------------------------------------------
// Helper: Linear Interpolation
// -------------------------------------------------------------------------
float
linear_interpolation(int x, int x0, int x1, float y0, float y1)
{
  if (x1 == x0) { return y0; }
  return y0 + (y1 - y0) * (x - x0) / static_cast<float>(x1 - x0);
}

// -------------------------------------------------------------------------
// Helper: Linear Least Squares to find slope (b)
// -------------------------------------------------------------------------
void
linearleastsquares(const std::vector<float>& x, const std::vector<float>& y, float& slope)
{
  int n = x.size();

  if (n < 2) {
    slope = 0.0f;
    return;
  }
  float sum_x = 0, sum_y = 0, sum_xx = 0, sum_xy = 0;

  for (int i = 0; i < n; ++i) {
    sum_x  += x[i];
    sum_y  += y[i];
    sum_xx += x[i] * x[i];
    sum_xy += x[i] * y[i];
  }
  float denom = (n * sum_xx - sum_x * sum_x);

  if (std::abs(denom) < 1e-6f) {
    slope = 0.0f;
  } else {
    slope = (n * sum_xy - sum_x * sum_y) / denom;
  }
}
} // anonymous namespace

// -------------------------------------------------------------------------
// Main Implementation
// -------------------------------------------------------------------------
std::shared_ptr<RadialSet>
compute_triple_median_Kdp(std::shared_ptr<RadialSet> PhiDP,
  std::shared_ptr<RadialSet>                         CC,
  int                                                KDP_filter_length_meters)
{
  if (!PhiDP || !CC) {
    fLogSevere("compute_triple_median_Kdp: Nullptr provided for PhiDP or CC.");
    return nullptr;
  }

  const size_t num_az    = PhiDP->getNumRadials();
  const size_t num_gates = PhiDP->getNumGates();

  if ((num_az != CC->getNumRadials()) || (num_gates != CC->getNumGates())) {
    fLogSevere("compute_triple_median_Kdp: Dimension mismatch between PhiDP and CC.");
    return nullptr;
  }

  // Prepare Output RadialSet
  auto kdpOutput = PhiDP->Clone();

  kdpOutput->setTypeName("SpecificDifferentialPhase");
  kdpOutput->setUnits("deg/km");
  kdpOutput->setDataAttributeValue("ColorMap", "KDP");

  auto& phiData = PhiDP->getFloat2DRef();
  auto& ccData  = CC->getFloat2DRef();
  auto& kdpData = kdpOutput->getFloat2DRef();

  // Fill output with missing initially
  kdpOutput->getFloat2D()->fill(Constants::MissingData);

  double gateWidthMeters = PhiDP->getGateWidthKMs() * 1000.0;

  if (gateWidthMeters <= 0.0) { 
    fLogSevere("compute_triple_median_Kdp: missing gateWidthKMs. using 250m override");
    gateWidthMeters = 250.0; // Failsafe
  }
  double distFirstGateM = PhiDP->getDistanceToFirstGateM();

  // Constant logic parameters from original support class
  //
  // Should these move into the local namespace as parameters instead?
  //
  const float min_valid_interval_size_meters = 2000.0f;
  const float max_allowed_diff_stddev        = 10.0f;
  const float min_allowed_CC = 0.8f;

  // Compute filter lengths based on bin spacing
  int phi_filter_length = std::round((250.0f * 5.0f) / gateWidthMeters);

  if (phi_filter_length % 2 == 0) { 
      phi_filter_length += 1; // Force odd
  }

  int kdp_filter_length      = std::round(KDP_filter_length_meters / gateWidthMeters);
  if (kdp_filter_length % 2 == 0) { 
      kdp_filter_length += 1; // Force odd
  }
  int half_kdp_filter_length = kdp_filter_length / 2; //integer division makes integers

  int min_valid_interval_size_bins = std::round(min_valid_interval_size_meters / gateWidthMeters);

  // Working buffers
  std::vector<float> raw_phi(num_gates);
  std::vector<float> single_med_phi(num_gates);
  std::vector<float> triple_med_phi(num_gates);
  std::vector<float> tmp_phi(num_gates);
  std::vector<float> diff_phi(num_gates);
  std::vector<float> stddev_phi(num_gates);
  std::vector<float> final_phi(num_gates);
  std::vector<int> mask(num_gates);

  for (size_t a = 0; a < num_az; ++a) {
    // Extract radial
    for (size_t g = 0; g < num_gates; ++g) {
      raw_phi[g] = phiData[a][g];
    }

    // 1. Triple Median Filter
    median_filter(phi_filter_length, raw_phi, single_med_phi);
    median_filter(phi_filter_length, single_med_phi, tmp_phi);
    median_filter(phi_filter_length, tmp_phi, triple_med_phi);

    // 2. Difference & Standard Deviation
    for (size_t g = 0; g < num_gates; ++g) {
      if (Constants::isGood(raw_phi[g]) && Constants::isGood(triple_med_phi[g])) {
        diff_phi[g] = raw_phi[g] - triple_med_phi[g];
      } else {
        diff_phi[g] = Constants::MissingData;
      }
    }

    calc_stddev(phi_filter_length, diff_phi, stddev_phi);

    // 3. Create Mask and set to 0
    std::fill(mask.begin(), mask.end(), 0);
    for (size_t g = 0; g < num_gates; ++g) {
      if (Constants::isGood(stddev_phi[g]) &&
        (stddev_phi[g] < max_allowed_diff_stddev) &&
        Constants::isGood(ccData[a][g]) &&
        (ccData[a][g] > min_allowed_CC) )
      {
        mask[g] = 1; // good data = 1
      }
    }
    // Boundaries are strictly bad
    mask[0] = 0;
    mask[num_gates - 1] = 0;

    // 4. Identify Valid Data Intervals
    std::vector<std::pair<int, int> > valid_intervals;
    int valid = 0, start = 0;

    for (size_t g = 0; g < num_gates; ++g) {
      if ((valid == 0) && (mask[g] == 1)) {
        start = g;
        valid = 1;
      }
      if ((valid == 1) && (mask[g] == 0)) {
        int end = g;
        valid = 0;
        if ((end - start) > min_valid_interval_size_bins) {
          valid_intervals.push_back(std::make_pair(start, end));
        }
      }
    }

    // 5. Interpolation & Final PhiDP Assembly
    std::fill(final_phi.begin(), final_phi.end(), 0.0f);
    int start_location = -1;

    if (!valid_intervals.empty()) {
      // Trim intervals
      int trim = std::round((min_valid_interval_size_bins - 1) / 2.0f);
      for (auto& iv : valid_intervals) {
        iv.first  += trim;
        iv.second -= trim;
      }

      start_location = valid_intervals[0].first;

      // Fill valid intervals with lightly smoothed (single median) phi
      for (const auto& iv : valid_intervals) {
        for (int g = iv.first; g < iv.second; ++g) {
          final_phi[g] = single_med_phi[g];
        }
      }

      // Interpolate between valid intervals
      int interp_start = valid_intervals[0].second - 1;
      for (size_t v = 1; v < valid_intervals.size(); ++v) {
        int interp_end = valid_intervals[v].first;
        //
        // Possible improvement:
        // limit the maximum amount of phidp that can be gained/lost
        // through the interpolation
        //
        for (int i = interp_start; i <= interp_end; ++i) {
          final_phi[i] = linear_interpolation(i, interp_start, interp_end,
              single_med_phi[interp_start], single_med_phi[interp_end]);
        }
        interp_start = valid_intervals[v].second - 1;
      }

      // Fill from last valid to edge
      int final_valid_gate = valid_intervals.back().second - 1;
      for (size_t g = final_valid_gate; g < num_gates; ++g) {
        final_phi[g] = single_med_phi[final_valid_gate];
      }

      // save and return the smoothed Phi data
      for (size_t g = 0; g < num_gates; ++g) {
        phiData[a][g] = final_phi[g];
      }
    } else {
      // no valid intervals for phiDP. Set the PhiDP data to zero.
      for (size_t g = 0; g < num_gates; ++g) {
        phiData[a][g] = 0.0;
      }
    }

    // 6. Calculate KDP (0.5 * slope * convert to deg/km=1000.0)
    std::vector<float> x_pts, y_pts;
    x_pts.reserve(kdp_filter_length + 1);
    y_pts.reserve(kdp_filter_length + 1);

    for (int g = 0; g < static_cast<int>(num_gates); ++g) {
      if ((start_location == -1) || (g < start_location) ) {
        kdpData[a][g] = 0.0f; // Assumes min phase 0
        continue;
      }

      int start_idx, end_idx;
      if (g < half_kdp_filter_length) {
        start_idx = 0;
        end_idx   = g + half_kdp_filter_length;
      } else if (g > static_cast<int>(num_gates) - half_kdp_filter_length - 1) {
        start_idx = g - half_kdp_filter_length;
        end_idx   = num_gates - 1;
      } else {
        start_idx = g - half_kdp_filter_length;
        end_idx   = g + half_kdp_filter_length;
      }

      x_pts.clear();
      y_pts.clear();
      for (int i = start_idx; i <= end_idx; ++i) {
        if (Constants::isGood(final_phi[i])) {
          x_pts.push_back(distFirstGateM + i * gateWidthMeters);
          y_pts.push_back(final_phi[i]);
        }
      }

      if (x_pts.size() > 2) {
        float slope = 0.0f;
        linearleastsquares(x_pts, y_pts, slope);
        kdpData[a][g] = 0.5f * slope * 1000.0f; // deg/m to deg/km
      } else {
        kdpData[a][g] = 0.0f;
      }
    }
  }

  PhiDP->setTypeName("SmoothedDifferentialPhase");
  PhiDP->setDataAttributeValue("FilterLength", std::to_string(KDP_filter_length_meters));
  return kdpOutput;
} // compute_triple_median_Kdp

std::shared_ptr<rapio::RadialSet>
combine_Kdp(std::shared_ptr<RadialSet> short_Kdp,
  std::shared_ptr<RadialSet>           long_Kdp,
  std::shared_ptr<RadialSet>           Ref,
  float                                refl_thresh)
{
  if (!short_Kdp || !long_Kdp || !Ref) {
    fLogSevere("combine_Kdp: Nullptr provided for one or more input RadialSets.");
    return nullptr;
  }

  const size_t num_az    = short_Kdp->getNumRadials();
  const size_t num_gates = short_Kdp->getNumGates();

  // Verify all input RadialSets have matching dimensions
  if ((num_az != long_Kdp->getNumRadials()) || (num_gates != long_Kdp->getNumGates()) ||
    (num_az != Ref->getNumRadials()) || (Ref->getNumGates() < num_gates) )
  {
    // Note: Ref  is often "long" in num_gates and that's ok don't use it there.
    fLogSevere("combine_Kdp: Dimension mismatch between input RadialSets.");
    fLogSevere("combine_Kdp: Long Kdp {} {}", long_Kdp->getNumRadials(), long_Kdp->getNumGates());
    fLogSevere("combine_Kdp: short Kdp {} {}", short_Kdp->getNumRadials(), short_Kdp->getNumGates());
    fLogSevere("combine_Kdp: Ref {} {}", Ref->getNumRadials(), Ref->getNumGates());
    // Note: If inputs might be on different grids, a RadialSetProjection
    // approach would be needed here. For now, we enforce pre-aligned grids.
    return nullptr;
  }

  // Clone the short_Kdp to inherit base metadata, dimensions, and arrays
  auto combined_Kdp = short_Kdp->Clone();

  combined_Kdp->setTypeName("SpecificDifferentialPhase");
  combined_Kdp->setDataAttributeValue("ColorMap", "KDP");

  // Fetch references to the underlying 2D float data arrays for high-speed access
  auto& shortData = short_Kdp->getFloat2DRef();
  auto& longData  = long_Kdp->getFloat2DRef();
  auto& refData   = Ref->getFloat2DRef();
  auto& outData   = combined_Kdp->getFloat2DRef();

  for (size_t a = 0; a < num_az; ++a) {
    for (size_t g = 0; g < num_gates; ++g) {
      float zVal = refData[a][g];

      // If the data is valid and exceeds the reflectivity threshold (convective),
      // utilize the short Kdp. Otherwise, use the long Kdp.
      if (Constants::isGood(zVal) && (zVal > refl_thresh) ) {
        outData[a][g] = shortData[a][g];
      } else {
        outData[a][g] = longData[a][g];
      }
    }
  }

  return combined_Kdp;
} // combine_Kdp
} // namespace rapio
