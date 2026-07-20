#include "computeDRmask.h" //The local file
#include "computeFilter.h" //The local file
#include <rRadialSet.h> //needed for any RadialSet objects you might send include
#include <rError.h> //Logging information uses this header
#include <rConstants.h> //Constant::MissingData 
#include <algorithm> //fill_n
#include <deque> //fill_n
//#include <cmath> //for pow() and min
//this is always a good idea so that the compiler knows you are 
// part of the rapio environment.
namespace rapio {

namespace {
// This anonymous namespace is a location for values only 
//  used by this algorithm itself.
//
//  At some point in the life of PhiDP there is so much attenuation
//  that the dualpol moments CC and Zdr are trash (not useful). Where
//  this point is is hard to know.  
// 
//  These are just initial guess right now.
//  --JMK 7/20/2026 
    const float StdPhiDP_threshold = 30;
    const float PhiDPsm_threshold = 100;
    const float Refl_threshold = 40.0; 
//
// This anonymous namespace is good place to keep other algorithmic threshold values.
//
} //end of anonymous namespace
//

/* Krause note's that a lot of the "leftover" bloom contains these 
 * very high CC values at very low SNR. We don't need SNR to know that CC values > 1.0
 * at not likely to be meteorological.
 */

std::shared_ptr<rapio::RadialSet> NBFdetection( int filter_length_meters, 
                                                std::shared_ptr<rapio::RadialSet> & Refsm,
                                                std::shared_ptr<rapio::RadialSet> & PhiDPsm,
                                                std::shared_ptr<rapio::RadialSet> & LTAR_mask,
                                                std::shared_ptr<rapio::RadialSet> & DR_mask,
                                                std::shared_ptr<rapio::RadialSet> & StdPhiDP 
                                              ) 
{

  size_t numGates =   PhiDPsm->getNumGates();
  size_t numRadials = PhiDPsm->getNumRadials();

  auto NBF_fill = PhiDPsm->Clone();
  auto& PhiDPsm_data = PhiDPsm->getFloat2DRef(); //Ref = Reference (not Reflectivity)
  auto& Refsm_data = Refsm->getFloat2DRef(); //Ref = Reference (not Reflectivity)
  auto& NBFdata = NBF_fill->getFloat2DRef(); //Ref = Reference (not Reflectivity)
  auto& LTAR_data = LTAR_mask->getFloat2DRef(); //Ref = Reference (not Reflectivity)
  auto& DR_data = DR_mask->getFloat2DRef(); //Ref = Reference (not Reflectivity)
  auto& StdPhiDP_data = StdPhiDP->getFloat2DRef(); //Ref = Reference (not Reflectivity)

  if (Refsm->getNumGates() < numGates) {
      numGates = Refsm->getNumGates();
      fLogSevere("DQ check strange, numGates, Reflectivity has too few gates: ");
  }

  //compute the number of gates in the filter length from meters
  double gateWidthMeters = PhiDPsm->getGateWidthKMs() * 1000.0;

  if (gateWidthMeters <= 0.0) {
    fLogSevere("NBFdetection: missing gateWidthKMs. using 250m override");
    gateWidthMeters = 250.0; // Failsafe
  }
 
  // Compute filter lengths based on bin spacing
  int filter_length_gates = std::round( filter_length_meters / gateWidthMeters);

  std::deque <float> Phi_field;
  for (size_t a = 0; a < numRadials; ++a) {
        //need to fill
        std::fill_n(NBFdata[a].begin(), numGates, 0.0f);
        //we start with false, it's rare
        bool NBF_detected = false;
        //When we get close to the end value of PhiDP the data
        //isn't NBF anymore. It's empty
        float EndPhiDP_value = PhiDPsm_data[a][numGates-1] * 0.97;
        Phi_field.clear();
 
        for (size_t g = 0; g < numGates; ++g) {
            //We want to linear average of StdPhiDP. AP has very high StdPhiDp over an entire 
            // areaa, but Significant storms have much lower stdPhiDP
            Phi_field.push_back(StdPhiDP_data[a][g]);
            if ((int) Phi_field.size() > filter_length_gates) {
            //This should keep the sample at a particular size
                Phi_field.pop_front();
            }
            float NBF_fill_flag = 0.0; //Again NBF is rare, so as little work as possible
            if (Constants::isGood(Refsm_data[a][g])) {
                if (!NBF_detected && PhiDPsm_data[a][g] < EndPhiDP_value ) {
                   float Phi_sum = std::accumulate(Phi_field.begin(), Phi_field.end(), 0.0); 
                   float StdPhiDP_field_ave = Phi_sum/(float) Phi_field.size();
                   //determine if the current filterlength is an NBF detection
                    if (StdPhiDP_field_ave < StdPhiDP_threshold && 
                        StdPhiDP_data[a][g] != 0.0 && //non-valid data has 0.0 StdPhiDP
                        PhiDPsm_data[a][g] > PhiDPsm_threshold && 
                        Refsm_data[a][g] > Refl_threshold && 
                        LTAR_data[a][g] != 0.0 && //Don't detect in LTAR
                        DR_data[a][g] == 0.0) //detect only where DR says non-meteorological
                    {
                        NBF_fill_flag = 1.0;
                        NBF_detected = true; 
                        //fLogInfo("NBF detected at azimuth {} gate {} ", a, g);
                    }
                }
                //already found NBF. The only thing we can do is test Reflectivity
                if (NBF_detected) {
                    if( Refsm_data[a][g] > 15.0) {
                        NBF_fill_flag = 1.0;
                    }
                }  
            } 
            NBFdata[a][g] = NBF_fill_flag;
        } //for g
  }//for a

  return NBF_fill;
}

}//end of namespace rapio
