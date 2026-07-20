#pragma once

#include <memory>
#include <rRadialSet.h>

namespace rapio {

//often you will need this because you want to pass a radialSet. Our
//example is too simple. See the rPrePro example for complex version
//class RadialSet:

/* Reference:
 * 
 * function computes a QC mask from  Depolarization Ratio 
 *
 * using Equation 6 in Ryzhkov et al. (2016):
 * Ryzhkov, A. V., S. Matrosov, V. Melnikov, D. Zrnic, P. Zhang, Q. Cao, M. Knight, 
 *   S. Troemel, and C. Simmer, 2016: Measurements of depolarization ratio using radars
 *   with simultaneous transmission / reception. J. Appl. Meteor. Climatol., submitted.
 *   https://journals.ametsoc.org/view/journals/apme/56/7/jamc-d-16-0098.1.xml
 *
 * How to use DR to determine meto/non-meto
 *
 * Kilambi, A., Fabry, F., & Meunier, V. (2018). 
 *   A simple and effective method for separating meteorological from nonmeteorological 
 *     targets using dual-polarization data. 
 *  Journal of Atmospheric and Oceanic Technology, 35(7), 1415–1424. https://doi.org/10.1175/JTECH-D-17-0175.1
 *
 * We add a dependance on non-local Reflectivty (9 gate average) to allow for detections 
 * inside severe thunderstroms,specifically for hail cores. Use with LTAR to handle ground clutter and 
 * windfarm contamination. 
 * Our approach using non-local Reflectivity may reduce DR's performance eliminating Anomolus Propogation (AP)
 *
 */

 /**
 * Computes the QC mask for Circular Depolarization Ration (DR) for SHV transmission S-Band 
 * radars like the WSR-88D. Added a dependance on non-local Reflectivity
 *
 * @param Ref:          The input Reflectivity, which we will average over 9 gates for non-local Ref 
 * @param DR:           The input depolarization ratio which identifies bad data based on Zdr and CC 
 */

std::shared_ptr<rapio::RadialSet> computeDRmask( std::shared_ptr<rapio::RadialSet> & Ref,
                                                   std::shared_ptr<rapio::RadialSet> & DR);

}
