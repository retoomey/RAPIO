#include "computeDRmask.h" //The local file
#include "computeFilter.h" //The local file
#include <rRadialSet.h> //needed for any RadialSet objects you might send include
#include <rError.h> //Logging information uses this header
#include <rConstants.h> //Constant::MissingData 
#include <algorithm> //fill_n
#include <deque> //
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
    const float minValueCC = 0.9999; //can be 1.0 or below
    const float CC_percent_high = 0.5; //can be between 0.0 and 1.0
//
// This anonymous namespace is good place to keep other algorithmic threshold values.
//
} //end of anonymous namespace
//

/* Krause note's that a lot of the "leftover" bloom contains these 
 * very high CC values at very low SNR. We don't need SNR to know that CC values > 1.0
 * at not likely to be meteorological.
 */

std::shared_ptr<rapio::RadialSet> computeCCmask( int filter_length_meters, 
                                                 std::shared_ptr<rapio::RadialSet> & CC) {

  size_t numGates =   CC->getNumGates();
  size_t numRadials = CC->getNumRadials();

  auto QCmask = CC->Clone();
  auto& CC_data = CC->getFloat2DRef(); //Ref = Reference (not Reflectivity)
  auto& QCdata = QCmask->getFloat2DRef(); //Ref = Reference (not Reflectivity)

  //compute the number of gates in the filter length from meters
  double gateWidthMeters = CC->getGateWidthKMs() * 1000.0;

  if (gateWidthMeters <= 0.0) {
    fLogSevere("compute_std_PhiDP: missing gateWidthKMs. using 250m override");
    gateWidthMeters = 250.0; // Failsafe
  }
 
  // Compute filter lengths based on bin spacing
  int filter_length_gates = std::round( filter_length_meters / gateWidthMeters);

  std::deque <float> cc_field;
  for (size_t a = 0; a < numRadials; ++a) {
        //need to fill
        std::fill_n(QCdata[a].begin(), numGates, 1.0f);
        cc_field.clear(); //between radials we want a clean start
        for (size_t g = 0; g < numGates; ++g) {
            float CCVal = CC_data[a][g];
            float qc_val = 1.0;

            cc_field.push_back(CCVal);
            if ( (int) cc_field.size() > filter_length_gates) {
            //This should keep the sample at a particular size
                cc_field.pop_front();
            } 

            if (Constants::isGood(CCVal) && (int) cc_field.size() == filter_length_gates ) {
            //run test.
            // Compute the number of valid values that are > threshold
            // non-valid CC values are very low numbers like -9900
            // so testing for valid high values of CC works.
            //
               float high_value_count = 0;  
               for(auto v: cc_field) {
                  if (v > minValueCC ) {
                      high_value_count += 1.0;
                  }
               } 
               float percent_high = high_value_count/(float) cc_field.size();
               if ( percent_high > CC_percent_high ) {
                   qc_val = 0.0;
               }
            } 
            QCdata[a][g] = qc_val;
        } //for g
  }//for a

  return QCmask;
}

}//end of namespace rapio
