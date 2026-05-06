#include "computeDR.h"  // The local file
#include <rRadialSet.h> // needed for any RadialSet objects you might send include
#include <rError.h>     // Logging information uses this header
#include <rConstants.h> // Constant::MissingData
#include <cmath>        // for pow() and min
// this is always a good idea so that the compiler knows you are
// part of the rapio environment.
namespace rapio {
namespace {
// This anonymous namespace is a location for values only
//  used by this algorithm itself.
//
//  The maximum allowed value of CC. Because data >= 1.0 causes the equation
//  to become non-finite we limit cc to this maximum value.
const float ccMax = 0.9999;
//
// This anonymous namespace is good place to keep other algorithmic threshold values.
//
} // end of anonymous namespace

// Note how the science part is sperate from the data acquisition and
// even the walking of arrays. This allows others to use the science
// and computeDR elsewhere if needed. We try to keep the science
// as seperate from the data flow as possible.
//
float
computeDR(float cc, float zdr, float MissingFlag)
{
  // convert Zdr into linear units
  auto zdrlin = pow(10.0, zdr / 10.0);

  if (!finite(zdrlin)) {
    return MissingFlag;
  }

  auto drVal = (1.0 + zdrlin - 2.0 * fmin(ccMax, cc) * (pow(zdrlin, 1.0 / 2.0)))
    / (1.0 + zdrlin + 2.0 * fmin(ccMax, cc) * (pow(zdrlin, 1.0 / 2.0)));

  if (!finite(drVal)) {
    return MissingFlag;
  } else {
    return 10.0 * log10(drVal);
  }
}

// The function that handles the problem as a RadialSet, uses the above
// function for the science. This is just data handling for RadialSet.
std::shared_ptr<rapio::RadialSet>
computeDR(std::shared_ptr<rapio::RadialSet> CC, std::shared_ptr<rapio::RadialSet> Zdr)
{
  //  Identify what the output should look like (Zdr)
  std::shared_ptr<rapio::RadialSet> DR = Zdr->Clone();

  // Assume the data has azimuthal alignment
  // each Az for [a] is the same azimuth in both RadialSets 
  // each Gate for [g] is the same in both RadialSets 
  size_t numRadials = Zdr->getNumRadials();
  size_t numGates   = Zdr->getNumGates();

  // Fetch the 2D arrays by reference for high-speed modification/access
  auto& zdrData = Zdr->getFloat2DRef();
  auto& ccData  = CC->getFloat2DRef();

  auto& drData = DR->getFloat2DRef();

  for (size_t a = 0; a < numRadials; ++a) {
    for (size_t g = 0; g < numGates; ++g) {
      float ccVal  = ccData[a][g];
      float zdrVal = zdrData[a][g];

      // Test for valid values:
      if (Constants::isGood(ccVal) && Constants::isGood(zdrVal) ) {
        drData[a][g] = computeDR(ccVal, zdrVal, Constants::MissingData);
      } else {
        if ( (ccData[a][g] == Constants::RangeFolded) ||
          (zdrData[a][g] == Constants::RangeFolded) )
        {
          drData[a][g] = Constants::RangeFolded;
        } else {
          drData[a][g] = Constants::MissingData;
        }
      }
    }
  }

  return DR;
} // computeDR

std::shared_ptr<rapio::RadialSet>
computeQCmask(std::shared_ptr<rapio::RadialSet> DR,
  float                                         DR_threshold)
{
  //  Identify what the output should look like (Zdr)
  std::shared_ptr<rapio::RadialSet> QCmask = DR->Clone();

  // Assume the data has azimuthal alignment
  // each Az for [a] is the same azimuth in both RadialSets 
  // each Gate for [g] is the same in both RadialSets 
  size_t numRadials = DR->getNumRadials();
  size_t numGates   = DR->getNumGates();

  // Fetch the 2D arrays by reference for high-speed modification/access
  auto& drData = DR->getFloat2DRef();
  auto& qcData = QCmask->getFloat2DRef();

  for (size_t a = 0; a < numRadials; ++a) {
    for (size_t g = 0; g < numGates; ++g) {
      qcData[a][g] = 0;
      if (drData[a][g] < DR_threshold) {
        qcData[a][g] = 1.0;
      }
    }
  }

  return QCmask;
}

void
applyQCmask(std::shared_ptr<rapio::RadialSet> myData, std::shared_ptr<rapio::RadialSet> QCmask)
{
  size_t numRadials = myData->getNumRadials();
  size_t numGates   = myData->getNumGates();

  // Check the data for azimuthal alignment
  if (QCmask->getNumRadials() != numRadials) {
    fLogSevere("applyQCmask, numRadials mismatch, abort data: {} qc: {} ", numRadials, QCmask->getNumRadials());
    return;
  }

  // Fetch the 2D arrays by reference for high-speed modification/access
  auto& Data   = myData->getFloat2DRef();
  auto& qcData = QCmask->getFloat2DRef();

  for (size_t a = 0; a < numRadials; ++a) {
    for (size_t g = 0; g < numGates; ++g) {
      //Often in WSR-88D data Reflectivity can be longer than Dualpol moments. 
      if (QCmask->getNumGates() < numGates) {
        if (qcData[a][g] < 1) {
          Data[a][g] = Constants::MissingData;
        }
      } else {
          Data[a][g] = Constants::MissingData;
      }
    }
  }
}

}// end of namespace rapio
