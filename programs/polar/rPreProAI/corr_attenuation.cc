#include "corr_attenuation.h"
#include <rRadialSet.h>
#include <rError.h>
#include <rConstants.h>

namespace rapio {
void
correct_Zh_for_attenuation(std::shared_ptr<RadialSet> Ref,
  std::shared_ptr<RadialSet>                          long_PhiDP)
{
  if (!Ref || !long_PhiDP) {
    fLogSevere("correct_Zh_for_attenuation: Nullptr provided for Ref or long_PhiDP.");
    return;
  }

  const size_t num_az    = Ref->getNumRadials();
  const size_t num_gates = Ref->getNumGates();
  const size_t last_gate = long_PhiDP->getNumGates() - 1;

  // Ensure the dimensions match before doing gate-to-gate comparisons
  if (num_az != long_PhiDP->getNumRadials()) {
    // note Ref is often "long" more gates compared to dualpol moments
    fLogSevere("correct_Zh_for_attenuation: Dimension mismatch between Ref and long_PhiDP.");
    return;
  }

  // Fetch the 2D arrays by reference for high-speed modification/access
  auto& refData = Ref->getFloat2DRef();
  auto& phiData = long_PhiDP->getFloat2DRef();

  for (size_t a = 0; a < num_az; ++a) {
    for (size_t g = 0; g < num_gates; ++g) {
      float phiVal;
      if (g < last_gate) {
        phiVal = phiData[a][g];
      } else {
        phiVal = phiData[a][last_gate];
      }
      float refVal   = refData[a][g];
      float delta_Zh = 0.0f;

      // Compute the attenuation delta if phase data is valid and non-negative
      if (Constants::isGood(phiVal) && (phiVal >= 0.0f) ) {
        // Formula from Ryzhkov book (Table 6.4)
        delta_Zh = 0.02f * phiVal;
      }

      // Apply correction to valid Reflectivity data
      if (Constants::isGood(refVal)) {
        refData[a][g] = refVal + delta_Zh;
      }
    }
  }
} // correct_Zh_for_attenuation

void
correct_Zdr_for_attenuation(std::shared_ptr<RadialSet> Zdr,
  std::shared_ptr<RadialSet>                           long_PhiDP)
{
  if (!Zdr || !long_PhiDP) {
    fLogSevere("correct_Zdr_for_attenuation: Nullptr provided for Zdr or long_PhiDP.");
    return;
  }

  const size_t num_az    = Zdr->getNumRadials();
  const size_t num_gates = Zdr->getNumGates();

  // Ensure the dimensions match before doing gate-to-gate comparisons
  if ((num_az != long_PhiDP->getNumRadials()) || (num_gates != long_PhiDP->getNumGates())) {
    fLogSevere("correct_Zdr_for_attenuation: Dimension mismatch between Zdr and long_PhiDP.");
    return;
  }

  // Fetch the 2D arrays by reference for high-speed modification/access
  auto& zdrData = Zdr->getFloat2DRef();
  auto& phiData = long_PhiDP->getFloat2DRef();

  for (size_t a = 0; a < num_az; ++a) {
    for (size_t g = 0; g < num_gates; ++g) {
      float phiVal    = phiData[a][g];
      float zdrVal    = zdrData[a][g];
      float delta_Zdr = 0.0f;

      // Compute the attenuation delta if phase data is valid and non-negative
      if (Constants::isGood(phiVal) && (phiVal >= 0.0f) ) {
        // Formula from Ryzhkov book (Table 6.4)
        delta_Zdr = 0.004f * phiVal;
      }

      // Apply correction to valid Zdr data
      if (Constants::isGood(zdrVal)) {
        zdrData[a][g] = zdrVal + delta_Zdr;
      }
    }
  }
} // correct_Zdr_for_attenuation
} // namespace rapio
