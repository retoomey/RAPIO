#pragma once

#include <memory>
#include <rRadialSet.h>

namespace rapio {

//often you will need this because you want to pass a radialSet. Our
//example is too simple. See the rPrePro example for complex version
//class RadialSet:

/* Reference:
 * 
 */

 /**
 * Computes the QC mask for Cross Correclation Coefficient (CC) aka RhoHV 
 * for SHV transmission S-Band radars like the WSR-88D. 
 *
 * @param filter_length: The filter length in meters to use  
 * @param CC:          The input CC, which we will test for  >0.9999 
 */

std::shared_ptr<rapio::RadialSet> computeCCmask( int filter_length_meters,
                                                 std::shared_ptr<rapio::RadialSet> & CC);

}
