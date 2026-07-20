#pragma once

#include <memory>
#include <rRadialSet.h>

namespace rapio {

//often you will need this because you want to pass a radialSet. Our
//example is too simple. See the rPrePro example for complex version
//class RadialSet:

/* Reference:
 * 
 * function computes a QC mask from standard deviation of PhiDP 
 *
 * We compute the linear standard deviation of PhiDP using a 3x median of PhiDP as the 
 * the average value in the computation. We then take a 2d median box filter of the 
 * standard deviation values and use that here. While PhiDP itself is not 2D, the standard deviation
 * of the values of PhiDP should be.  
 *
 * We add a dependance on non-local Reflectivty (9 gate 2d average) to allow for detections 
 * inside severe thunderstroms,specifically for hail cores. 
 *
 */

 /**
 * Computes the QC mask for the standard deviation of PhiDP for S-Band 
 * radars like the WSR-88D. Added a dependance on non-local Reflectivity
 *
 * @param Refsm:          The input smoothed non-local Reflectivity,  5x5 box, max value filter
 * @param stdPhiDP:       The input stadard deviation of PhiDP, also smoothed, 3x3 median value filter
 */

std::shared_ptr<rapio::RadialSet> computeStdPhiDPmask( std::shared_ptr<rapio::RadialSet> & Refsm,
                                                       std::shared_ptr<rapio::RadialSet> & stdPhiDP);

}
