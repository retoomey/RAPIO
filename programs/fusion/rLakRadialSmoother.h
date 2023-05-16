#pragma once

#include <rUtility.h>
#include <rRadialSet.h>

namespace rapio {
/** For first pass, implement Lak's radial set moving average smoother.
 * FIXME: Make generic plugin class for prefilters.  For now we'll have
 * a toggle flag to turn on/off
 *
 * A moving average replacing algorithm presented in the paper:
 * "A Real-Time, Three-Dimensional, Rapidly Updating, Heterogeneous Radar
 * Merger Technique for Reflectivity, Velocity, and Derived Products"
 * Weather and Forecasting October 2006
 * Valliappa Lakshmanan, Travis Smith, Kurt Hondl, Greg Stumpf
 *
 * In particular, page 10 describing Virtual Volumes and the elevation
 * influence.
 *
 * Notes:  I'm debating the accuracy of this smoothing technique
 * vs. one using weighted sampling in the CONUS plane.  This technique
 * is cheaper, which is likely why Lak choose it on older harder systems.
 * We could probably supersample around the CONUS cell center and possibly
 * improve the smooothing at the cost of more CPU.  Pictures will probably
 * be needed.  This alternative method might be more akin to the horizontal
 * interpolation attempted in w2merger.
 */
class LakRadialSmoother : public Utility {
public:

  static void
  smooth(std::shared_ptr<RadialSet> rs, int half_size);
};
}
