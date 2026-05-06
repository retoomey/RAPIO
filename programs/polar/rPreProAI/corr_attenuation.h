#pragma once

#include <memory>

namespace rapio {
// Forward declaration
class RadialSet;

/*Reference:
 *
 * Title: Radar Polarimetry for Weather Observations
 * by Alexander V. Ryzhkov and Dušan S. Zrnić.
 * Series: Springer Atmospheric Sciences
 * Published: 2019
 * ISBN-13: 978-3030050924
 *
 */

/**
 * Corrects the Reflectivity data for horizontal attenuation using a simple
 * formula from Ryzhkov book. pg. table 6.4
 *
 * @param Ref            The input Ref to be corrected. Hint; use clone Ref->Clone()
 * @param long_PhiDP     The long average PhiDP data.
 */
void
correct_Zh_for_attenuation(std::shared_ptr<RadialSet> Ref,
  std::shared_ptr<RadialSet>                          long_PhiDP);

/**
 * Corrects the Zdr data for horizontal attenuation using a simple
 * formula from Ryzhkov book. pg. table 6.4
 *
 * @param Zdr            The input Zdr to be corrected. Hint; use clone Zdr->Clone()
 * @param long_PhiDP     The long average PhiDP data.
 */
void
correct_Zdr_for_attenuation(std::shared_ptr<RadialSet> Zdr,
  std::shared_ptr<RadialSet>                           long_PhiDP);
} // namespace rapio
