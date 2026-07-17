#pragma once

#include <memory>
#include <rRadialSet.h>

namespace rapio {

//often you will need this because you want to pass a radialSet. Our
//example is too simple. See the rPrePro example for complex version
//class RadialSet:

/* Reference:
 *   All new. 2026, Krause
 */

 /**
 * Computes the NBF fill flags for Reflectivity correction. NBF data is
 * often flagged by DR, CC, or StdPhiDP as non-meteorological. This algorithm
 * is a simple method to identify gates where NBF exists and where we should *NOT*
 * remove the Reflectivity data. 
 *
 * (A subsitute method would be a running sum of Reflectivity)
 *
 * @param filter_length: The filter length in meters to use  
 * @param Refsm:    The input smoothed Reflectivity using a 5x5 BlurFilter (max value of box)
 * @param PhiDPam:  The input long gate smoothed PhiDP, post Kdp production 
 */
std::shared_ptr<rapio::RadialSet> NBFdetection( int filter_length_meters,
                                                std::shared_ptr<rapio::RadialSet> & Refsm,
                                                std::shared_ptr<rapio::RadialSet> & PhiDPsm,
                                                std::shared_ptr<rapio::RadialSet> & LTAR_mask,
                                                std::shared_ptr<rapio::RadialSet> & DR_mask 
                                              ); 

}
