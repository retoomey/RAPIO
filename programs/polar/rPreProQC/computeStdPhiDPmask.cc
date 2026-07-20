#include "computeDRmask.h" //The local file
#include "computeFilter.h" //The local file
#include <rRadialSet.h> //needed for any RadialSet objects you might send include
#include <rError.h> //Logging information uses this header
#include <rConstants.h> //Constant::MissingData 
#include <algorithm> //fill_n
//#include <cmath> //for pow() and min
//this is always a good idea so that the compiler knows you are 
// part of the rapio environment.
namespace rapio {

namespace {
// This anonymous namespace is a location for values only 
//  used by this algorithm itself.
//
//  DR values below the minimum value are considered 
//  meteorological. This sets the global min value. 
//  
    const float minValueStdPhiDP = 10;
//
// This anonymous namespace is good place to keep other algorithmic threshold values.
//
float stdPhiDP_threshold( float ref_dBZ) {
    //
    // Ref -- std_thresh
    // 50     25 
    // 40     20 
    // 30     15 
    // 20     10 
    //
    float thresh = 0 + (ref_dBZ/10.0)*5 ;
    if (thresh < minValueStdPhiDP ) {
        thresh = minValueStdPhiDP;
    }
    return thresh;
}
} //end of anonymous namespace
//
  /* Move this code outside of algorithm for speed. 
   * we want to use the Refsm field with another mask.
   * if you want to recreate it. Here you go.
   *
  //Blur the data so that the average value of the 3x3 box is 
  //used in the DR Threshold computation. We want to compare a local average 
  //reflectivity threshold to DR rather than a gate by gate value. The local average
  //of reflectivity tells us if the gate value is vaild.
  // Note: try 5x5, but speed of 3x3 is faster? Either probably works. 
  //std::shared_ptr<rapio::RadialSet> Refsm = apply2DBlurFilter(Ref, 5, 5, 0.33);
  */
std::shared_ptr<rapio::RadialSet> computeStdPhiDPmask( std::shared_ptr<rapio::RadialSet>  & Refsm, 
                                                       std::shared_ptr<rapio::RadialSet> & stdPhiDP) {

  size_t numGates =   Refsm->getNumGates();
  size_t numRadials = Refsm->getNumRadials();
  auto azRefsm        = Refsm->getAzimuthRef();
  auto azstdPhiDP         = stdPhiDP->getAzimuthRef();

  if (stdPhiDP->getNumRadials() != numRadials){
      fLogSevere("computestdPhiDPmask: ABORT Reflectivity radials : {} and stdPhiDP radials: {} do not match. ", numRadials, stdPhiDP->getNumRadials());
      return nullptr;
  }

  if (Refsm->getNumGates() < numGates) {
      numGates = Refsm->getNumGates();
      fLogSevere("DQ check strange, numGates, Reflectivity has too few gates: ");
  }
  

  auto QCmask = Refsm->Clone();
  auto& stdPhiDP_data = stdPhiDP->getFloat2DRef();
  auto& Z_data = Refsm->getFloat2DRef();
  auto& QCdata = QCmask->getFloat2DRef();

  for (size_t a = 0; a < numRadials; ++a) {
        //need to fill because of the Refsm clone you get Refsm values
        std::fill_n(QCdata[a].begin(), numGates, 1.0f);
        for (size_t g = 0; g < numGates; ++g) {
            float refVal = Z_data[a][g];
            float stdPhiDPval = stdPhiDP_data[a][g];
            float qc_val = 1.0f;
            if (Constants::isGood(refVal) ) {
                float thresh = stdPhiDP_threshold(refVal);
                if (stdPhiDPval > thresh) {
                  qc_val = 0.0f;
                }
            } else {
                  qc_val = 0.0f;
            }
            QCdata[a][g] = qc_val;
        } //for g
  }//for a

  return QCmask;
}

}//end of namespace rapio
