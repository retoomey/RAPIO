#pragma once

#include <memory>

namespace rapio {
// forward declaration
class RadialSet;
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
 * radars like the WSR-88D. The gate-by-gate function
 *
 * @param cc:            The input corellation coefficent (aka RhoHV)
 * @param zdr:           The input differential Reflectivity (aka Zdr)
 * @param missing_flag:  The input missing_data value used by the system (as a float)
 * @returns: DR:         Returns the depolarization ration estimate for SHV as a ratio
 */

float
computeDR(float cc, float zdr, float missing_flag);

/**
 * Computes the equivalent to Circular Depolarization Ration (DR) for SHV transmission S-Band
 * radars like the WSR-88D. This function handles an entire RadialSet at once
 *
 * @param: CC            The input corellation coefficent (aka RhoHV) as a RadialSet
 * @param: Zdr           The input differential Reflectivity (aka Zdr) as a RadialSet
 * @returns: DR         Returns the depolarization ration estimate for SHV as a ratio as a RadialSet
 */

std::shared_ptr<rapio::RadialSet>
computeDR(std::shared_ptr<rapio::RadialSet> CC,
  std::shared_ptr<rapio::RadialSet>         Zdr);

/**
 * Computes a mask to seperate non-meteorlogical (0 ) from meteorological (1) targets
 * using a simple DR threshold value. See Kilambi et. al. (2018)
 *
 * @param: DR           The input depolarization ratio (DR) as a RadialSet
 * @param: DR_threshold The threshold to use (-12.0) as a float
 * @returns: QCmask     Returns the QCmask as a RadialSet
 */

std::shared_ptr<rapio::RadialSet>
computeQCmask(std::shared_ptr<rapio::RadialSet> DR,
  float                                         DR_threshold);

/**
 * Iterates through the data setting data with QCmask = 0 to MissingData.
 * (assumes that the data are aligned and that Data[a] is on the same
 *  azimuth as QCMask[a] and that the gate spacing and range to first gate
 *  are also the same)
 *
 * @param: Data         The input data to QC, anything 
 * @returns: QCmask     Returns the QCmask as a RadialSet
 */

void
applyQCmask(std::shared_ptr<rapio::RadialSet> Data, std::shared_ptr<rapio::RadialSet> QCmask);
}
