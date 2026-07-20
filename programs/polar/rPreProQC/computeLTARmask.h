#pragma once

#include <memory>
#include <rRadialSet.h>

namespace rapio {

//often you will need this because you want to pass a radialSet. Our
//example is too simple. See the rPrePro example for complex version
//class RadialSet:

/* Reference:
 * 
 * function computes a QC mask from Long Term Average Reflectivity (LTAR)
 * (30 day averages of reflectivity) 
 *
 * We add a dependance on non-local LTAR (9 gate average) to allow for (small) fluctuations 
 * in the path of the beam. It is likely that any particular beam path
 * is sligtly different day to day. While a 30 day average of Reflectivity does a 
 * good job of finding the most likely location of ground clutter, it might actually 
 * show up in a near by gate. A dilation of the LTAR field where we use the max value
 * of LTAR in a 3x3 box around the target gate accounts for this.   
 *
 */

 /**
 * Computes the QC mask for LTAR for SHV transmission S-Band 
 * radars like the WSR-88D. Requires LTAR reference data to be computed 
 *
 * @param Ref:     The input Reflectivity  
 * @param LTAR:    The 2D dilated LTAR data, see processOptions() 
 */

std::shared_ptr<rapio::RadialSet> computeLTARmask( std::shared_ptr<rapio::RadialSet> & Ref,
                                                   std::shared_ptr<rapio::RadialSet> & LTAR );

 /**
 * Reads the LTAR file and outputs the data 
 *
 * @param full_filepath :     The input filepath to the LTAR reference data 
 */
std::shared_ptr<rapio::RadialSet> readLTARData(const std::string& filepath); 
}
