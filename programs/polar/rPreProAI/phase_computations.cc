#include "phase_computations.h"
#include "fastMedian.h"
#include <rRadialSet.h>
#include <rError.h>
#include <algorithm>
#include <iostream>
#include <vector>

namespace rapio {
namespace {
// Helper to ensure azimuth is within [0, 360)
float
check_Az(float az)
{
  while (az < 0.0f) {
    az += 360.0f;
  }
  while (az >= 360.0f) {
    az -= 360.0f;
  }
  return az;
}
} // anonymous namespace

void
correct_system_phase(std::shared_ptr<RadialSet> PhiDP,
  std::shared_ptr<RadialSet>                    CC,
  float                                         & min_system_phase)
{
  if (!PhiDP || !CC) {
    fLogSevere("correct_system_phase: Nullptr provided for PhiDP or CC.");
    return;
  }

  const size_t num_az    = PhiDP->getNumRadials();
  const size_t num_gates = PhiDP->getNumGates();

  if ((num_az != CC->getNumRadials()) || (num_gates != CC->getNumGates())) {
    fLogSevere("correct_system_phase: Dimension mismatch between PhiDP and CC.");
    return;
  }

  auto& phiData = PhiDP->getFloat2DRef();
  auto& ccData  = CC->getFloat2DRef();

  auto& phiGateWidths = PhiDP->getGateWidthVector()->ref();

  // Check if a valid min_system_phase was provided
  // Original code used a specific NoDataValue check, here we use a common sentinel 
  // or just check if it's less than a highly negative number
  bool compute_phase = (min_system_phase <= -999.0);

  if (!compute_phase) {
    if (min_system_phase < 0) {
      fLogSevere("min_system_PhiDP supplied to correct_system_phase is negative.");
      fLogSevere("This will increase the value of the returned phase and is probably bad.");
      fLogSevere("min_system_PhiDP: {}", min_system_phase);
    }
    if (min_system_phase == 0) {
      fLogSevere("min_system_phase supplied to correct_system_phase is zero.");
      fLogSevere("PhiDP values are not changed by this routine with this input.");
      fLogSevere("No correction is applied.");
    }
  } else {
    // We need to compute the min_system_phase

    static const float PhiDP_corr_min_data_length_meters = 2750.0;
    static const float PhiDP_corr_min_cc_value   = 0.95;
    static const int PhiDP_corr_min_num_valid_az = 5;

    std::vector<float> valid_system_PhiDP;
    std::vector<float> data_store;

    for (size_t a = 0; a < num_az; ++a) {
      // Get gate width for this radial to determine required consecutive valid gates
      float binSpacing    = phiGateWidths[a] > 0 ? phiGateWidths[a] : 250.0f; // Default if missing
      int min_valid_count = static_cast<int>(std::round(PhiDP_corr_min_data_length_meters / binSpacing));

      int valid_gate_count = 0;
      data_store.clear();
      data_store.reserve(min_valid_count);

      for (size_t g = 0; g < num_gates; ++g) {
        float phiVal = phiData[a][g];
        float ccVal  = ccData[a][g];

        if (Constants::isGood(phiVal) && Constants::isGood(ccVal)) {
          if (ccVal > PhiDP_corr_min_cc_value) {
            ++valid_gate_count;
            data_store.push_back(phiVal);

            if (valid_gate_count >= min_valid_count) {
              valid_system_PhiDP.push_back(findSimpleMedian(data_store));
              break;
            }
          } else {
            valid_gate_count = 0;
            data_store.clear();
          }
        } else {
          valid_gate_count = 0;
          data_store.clear();
        }
      }
    }

    /*
     * int count = 0;
     * for (auto vp: valid_system_PhiDP) {
     *  fLogDebug("min_system_phase: {} count: {}", vp, count );
     ++count;
     * }
     */

    if (valid_system_PhiDP.size() < static_cast<size_t>(PhiDP_corr_min_num_valid_az)) {
      fLogInfo(
        "correct_system_phase: Not enough valid azimuths found to compute min system phase. Returning uncorrected data.");
      return; // Return without modifying
    } else {
      std::sort(valid_system_PhiDP.begin(), valid_system_PhiDP.end());
      int index = static_cast<int>(std::round(valid_system_PhiDP.size() * 0.1));
      if (index < 0) { index = 0; }
      min_system_phase = valid_system_PhiDP[index];
      fLogDebug("min_system_phase: {} size: {}", min_system_phase, valid_system_PhiDP.size()  );
    }
  }

  // Now apply the correction
  for (size_t a = 0; a < num_az; ++a) {
    for (size_t g = 0; g < num_gates; ++g) {
      if (Constants::isGood(phiData[a][g])) {
        phiData[a][g] = check_Az(phiData[a][g] - min_system_phase);
      }
    }
  }
} // correct_system_phase

void
correct_system_phase_radial_by_radial(std::shared_ptr<RadialSet> PhiDP,
  std::shared_ptr<RadialSet>                                     CC,
  float                                                          & min_system_phase)
{
  if (!PhiDP || !CC) {
    fLogSevere("correct_system_phase: Nullptr provided for PhiDP or CC.");
    return;
  }

  const size_t num_az    = PhiDP->getNumRadials();
  const size_t num_gates = PhiDP->getNumGates();

  if ((num_az != CC->getNumRadials()) || (num_gates != CC->getNumGates())) {
    fLogSevere("correct_system_phase: Dimension mismatch between PhiDP and CC.");
    return;
  }

  auto& phiData = PhiDP->getFloat2DRef();
  auto& ccData  = CC->getFloat2DRef();

  auto& phiGateWidths = PhiDP->getGateWidthVector()->ref();
  // Store the Azimuth values for gate by gate reference
  auto& azRef = PhiDP->getAzimuthRef();
  std::vector<float> Az(azRef.begin(), azRef.end());
  // Store a map linking the Azimuth value to the pair start,end of the good segment
  std::map<float, std::pair<size_t, size_t> > good_segments; // This is map[Azimuth]=pairof(start_good, end_good)

  // Check if a valid min_system_phase was provided
  // Original code used a specific NoDataValue check, here we use a common sentinel or just check if it's less than a highly negative number
  bool compute_phase   = (min_system_phase <= -999.0);
  int last_valid_count = 5;

  if (!compute_phase) {
    if (min_system_phase < 0) {
      fLogSevere("min_system_PhiDP supplied to correct_system_phase is negative.");
      fLogSevere("This will increase the value of the returned phase and is probably bad.");
      fLogSevere("min_system_PhiDP: {}", min_system_phase);
    }
    if (min_system_phase == 0) {
      fLogSevere("min_system_phase supplied to correct_system_phase is zero.");
      fLogSevere("PhiDP values are not changed by this routine with this input.");
      fLogSevere("No correction is applied.");
    }
  } else {
    // We need to compute the min_system_phase

    static const float PhiDP_corr_min_data_length_meters = 2750.0;
    static const float PhiDP_corr_min_cc_value   = 0.95;
    static const int PhiDP_corr_min_num_valid_az = 5;

    std::vector<float> valid_system_PhiDP;
    std::vector<float> data_store;

    for (size_t a = 0; a < num_az; ++a) {
      // Get gate width for this radial to determine required consecutive valid gates
      float binSpacing    = phiGateWidths[a] > 0 ? phiGateWidths[a] : 250.0f; // Default if missing
      int min_valid_count = static_cast<int>(std::round(PhiDP_corr_min_data_length_meters / binSpacing));
      last_valid_count = min_valid_count;

      int valid_gate_count = 0;
      data_store.clear();
      data_store.reserve(min_valid_count);

      for (size_t g = 0; g < num_gates; ++g) {
        float phiVal = phiData[a][g];
        float ccVal  = ccData[a][g];

        if (Constants::isGood(phiVal) && Constants::isGood(ccVal)) {
          if (ccVal > PhiDP_corr_min_cc_value) {
            ++valid_gate_count;
            data_store.push_back(phiVal);

            if (valid_gate_count >= min_valid_count) {
              valid_system_PhiDP.push_back(findSimpleMedian(data_store));
              // store the valid segment for the radial-by-radial computation
              good_segments[Az[a]] = std::make_pair( (g - valid_gate_count + 1), g);
              break;
            }
          } else {
            valid_gate_count = 0;
            data_store.clear();
          }
        } else {
          valid_gate_count = 0;
          data_store.clear();
        }
      }
    }

    /*
     * int count = 0;
     * for (auto vp: valid_system_PhiDP) {
     *  fLogDebug("min_system_phase: {} count: {}", vp, count );
     ++count;
     * }
     */

    if (valid_system_PhiDP.size() < static_cast<size_t>(PhiDP_corr_min_num_valid_az)) {
      fLogInfo(
        "correct_system_phase: Not enough valid azimuths found to compute min system phase. Returning uncorrected data.");
      return; // Return without modifying
    } else {
      std::sort(valid_system_PhiDP.begin(), valid_system_PhiDP.end());
      int index = static_cast<int>(std::round(valid_system_PhiDP.size() * 0.1));
      if (index < 0) { index = 0; }
      min_system_phase = valid_system_PhiDP[index];
      fLogDebug("min_system_phase: {} size: {}", min_system_phase, valid_system_PhiDP.size()  );
    }
  }

  std::vector<float> gate_values;

  gate_values.reserve(last_valid_count);
  float radial_init_phase = min_system_phase;

  // Now apply the correction
  for (size_t a = 0; a < num_az; ++a) {
    // Test the system phase radial by radial, and adjust it based on the results from
    // the first good segment in the radial
    auto it = good_segments.find(Az[a]);
    if (it != good_segments.end()) {
      // good segment exits
      gate_values.clear();
      auto sg = it->second.first;
      auto eg = it->second.second;
      for (size_t g = sg; g < eg; ++g) {
        gate_values.push_back(phiData[a][g]);
      }
      // find the minimum value
      radial_init_phase = findSimpleMedian(gate_values);
      // Test it for sanity
      if (fabs(min_system_phase - radial_init_phase) > 10.0) {
        fLogDebug("Radial_phase rejected: min_system_phase: {} radial_phase: {}", min_system_phase, radial_init_phase);
        radial_init_phase = min_system_phase;
      }
    } else {
      // no good segment, use min_system_phase as is....
      radial_init_phase = min_system_phase;
    }

    for (size_t g = 0; g < num_gates; ++g) {
      if (Constants::isGood(phiData[a][g])) {
        // phiData[a][g] = check_Az(phiData[a][g] - radial_init_phase);
        phiData[a][g] = phiData[a][g] - radial_init_phase; // may produce some small negative values
      }
    }
  }
} // correct_system_phase_radial_by_radial
} // namespace rapio
