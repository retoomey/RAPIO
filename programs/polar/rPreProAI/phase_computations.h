#pragma once

#include <memory>

namespace rapio {
// Forward declaration
class RadialSet;

/**
 * Eliminates inital system phase in the PhiDP data by identifying the value
 * of phase in the first valid radar echo for each individual radial.
 *
 * This first phase value is collected and evalauted to identify the
 * min_system_phase. We then remove the computed system phase from
 * each gate of PhiDP and return the PhiDP.
 *
 * This subrotuine returns a PhiDP RadialSet with a system_phase
 * value of zero.
 *
 * If you already know the min_system_phase send it in. If you want the
 * code to compute it for you use nan
 *
 * @param PhiDP            The input phase to be corrected. Hint; use clone PhiDP->Clone()
 * @param CC               The aligned to PhiDP, CC cross correlation coeffient
 * @param min_system_phase If known, send in, else send -9999.0 and receive result
 */
void
correct_system_phase(std::shared_ptr<RadialSet> PhiDP,
  std::shared_ptr<RadialSet>                    CC,
  float                                         & min_system_phase);

/**
 * This code does the same task as the basic correct_system_phase, but
 * makes an adjustment based on the radial value of the first good segment.
 * There is a max diviation threshold of 10 from the system value.
 * If abs diff of the system_init_phase and the radial_init_phase is > 10 we
 * default back to the system_init_phase.
 *
 * We generally use an elevations worth of data to get a system phase value, but
 * you can use some other way and send it in.
 *
 * @param PhiDP            The input phase to be corrected. Hint; use clone PhiDP->Clone()
 * @param CC               The aligned to PhiDP, CC cross correlation coeffient
 * @param min_system_phase If known, send in, else send -9999.0 and receive result
 */

void
correct_system_phase_radial_by_radial(std::shared_ptr<RadialSet> PhiDP,
  std::shared_ptr<RadialSet>                                     CC,
  float                                                          & min_system_phase);
} // namespace rapio
