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
    const float minValueDR = -17;
//
// This anonymous namespace is good place to keep other algorithmic threshold values.
//
float DR_threshold( float ref_dBZ) {
    //Linear relationship. Someone might study this more and deeper. 
    // Kilambi suggested 12.7 but this removes hailcores. We adjust with this linear
    //   relationship to Reflectivity.
    //   https://doi.org/10.1175/JTECH-D-17-0175.1
    //
    // Ref -- DR_thresh
    // 50     -7
    // 40     -9.5 
    // 30     -12
    // 10     -17 (minValueDR -17) 
    //
    float dr_thresh = 0.25*ref_dBZ - 19.5;
    if (dr_thresh < minValueDR ) {
        dr_thresh = minValueDR;
    }
    return dr_thresh;
}
} //end of anonymous namespace
//
std::shared_ptr<rapio::RadialSet> computeDRmask( std::shared_ptr<rapio::RadialSet>  & Ref, 
                                                 std::shared_ptr<rapio::RadialSet> & DR) {

  size_t numGates =   DR->getNumGates();
  size_t numRadials = Ref->getNumRadials();
  auto azRef        = Ref->getAzimuthRef();
  auto azDR         = DR->getAzimuthRef();

  if (DR->getNumRadials() != numRadials){
      fLogSevere("computeDRmask: ABORT Reflectivity radials : {} and DR radials: {} do not match. ", numRadials, DR->getNumRadials());
      return nullptr;
  }

  if (Ref->getNumGates() < numGates) {
      numGates = Ref->getNumGates();
      fLogSevere("DQ check strange, numGates, Reflectivity has too few gates: ");
  }
  
  //Blur the data so that the average value of the 3x3 box is 
  //used in the DR Threshold computation. We want to compare a local average 
  //reflectivity threshold to DR rather than a gate by gate value. The local average
  //of reflectivity tells us if the gate value is vaild.
  // Note: try 5x5, but speed of 3x3 is faster? Either probably works. 
  std::shared_ptr<rapio::RadialSet> Refsm = apply2DBlurFilter(Ref, 5, 5, 0.33);

  auto QCmask = Refsm->Clone();
  auto& DR_data = DR->getFloat2DRef();
  auto& Z_data = Refsm->getFloat2DRef();
  auto& QCdata = QCmask->getFloat2DRef();

  for (size_t a = 0; a < numRadials; ++a) {
        //need to fill because of te Ref clone you get Ref values
        std::fill_n(QCdata[a].begin(), numGates, 1.0f);
        for (size_t g = 0; g < numGates; ++g) {
            float refVal = Z_data[a][g];
            float DRval = DR_data[a][g];
            float qc_val = 1.0f;
            if (Constants::isGood(refVal) ) {
                float dr_thresh = DR_threshold(refVal);
                if (DRval > dr_thresh) {
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
