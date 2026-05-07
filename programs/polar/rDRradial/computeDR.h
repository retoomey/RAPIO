#pragma once

#include <memory>

namespace rapio {

//often you will need this because you want to pass a radialSet. Our
//example is too simple. See the rPrePro example for complex version
//class RadialSet:

/* Reference:
 * 
 * function computes the Depolarization Ratio using Equation 6 in Ryzhkov et al. (2016):
 *
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
 */

 /**
 * Computes the equivalent to Circular Depolarization Ration (DR) for SHV transmission S-Band 
 * radars like the WSR-88D.
 *
 * @param cc:            The input corellation coefficent (aka RhoHV)
 * @param zdr:           The input differential Reflectivity (aka Zdr)
 * @param missing_flag:  The input missing_data value used by the system (as a float)
 */

float computeDR( float cc, float zdr, float missing_flag); 

}
