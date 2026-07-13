#include "computeLTARmask.h" //The local file
#include <rIODataType.h>
#include <rOS.h>
#include <rRadialSet.h> //needed for any RadialSet objects you might send include
#include <rRadialSetProjection.h> //needed to match LTAR to Reflectivity
#include <rError.h> //Logging information uses this header
#include <rConstants.h> //Constant::MissingData 
//#include <cmath> //for pow() and min
//this is always a good idea so that the compiler knows you are 
// part of the rapio environment.
namespace rapio {

namespace {
// This anonymous namespace is a location for values only 
//  used by this algorithm itself.
//
// Large LTAR values are evidence of ground clutter. There are times 
// when the reflectivity data overcomes the LTAR value as when severe
// thunderstorms pass over areas with light to moderate clutter signals 
// 
//
float Ref_LTAR_combination( float ref_dbz, float ltar_dbz) {

    if (ltar_dbz > 35.0 ) {
        return 0.0;
    } else if (ltar_dbz > 0.0) {
       // Calculate the dynamic threshold using the power law
       // replaceing the non-dynamic hard thresholds.....
       double threshold = 0.312 * pow(ltar_dbz, 1.287);
    
        if ((ref_dbz - ltar_dbz) < threshold) {
            return 0.0;
        } else {
            return 1.0;
        }
   }
   return 1.0;
}

} //end of anonymous namespace

std::shared_ptr<rapio::RadialSet> readLTARData(const std::string& filepath) {
    // 1. Verify the file actually exists using RAPIO's OS wrapper
    if (!rapio::OS::isRegularFile(filepath)) {
        fLogSevere("LTAR file does not exist or is not a regular file: {}", filepath);
        return nullptr;
    }

    // 2. Read the file into a RadialSet.
    // The second parameter "netcdf" explicitly tells the factory to use the NetCDF builder.
    auto LTAR= rapio::IODataType::read<rapio::RadialSet>(filepath, "netcdf");

    // 3. Verify the read was successful
    if (LTAR != nullptr) {
        fLogInfo("Successfully read reference data: {} ({} radials, {} gates)", 
                 LTAR->getTypeName(),
                 LTAR->getNumRadials(), 
                 LTAR->getNumGates());
                 
        // You can now access the underlying data array:
        // like this....
        // auto& data = LTAR->getFloat2DRef();
        //
        
    } else {
        fLogSevere("Failed to parse RadialSet from NetCDF file: {}", filepath);
    }
    return LTAR;
}
//
std::shared_ptr<rapio::RadialSet> computeLTARmask( std::shared_ptr<rapio::RadialSet>  & Ref, 
                                                   std::shared_ptr<rapio::RadialSet>  & LTAR)  {
//
//read in the LTAR file, assume that the LTAR has already been hit by the 2DDilation Filter
//
  size_t numRadials = Ref->getNumRadials();
  size_t numGates = Ref->getNumGates();
  auto azRef        = Ref->getAzimuthRef();

  // 2. Extract the projection from the LTAR data
  auto LTARProj = std::dynamic_pointer_cast<rapio::RadialSetProjection>(LTAR->getProjection());
  if (!LTARProj) return nullptr;

  auto QCmask = Ref->Clone();
  auto& Z_data = Ref->getFloat2DRef();
  auto& QCdata = QCmask->getFloat2DRef();
  
  auto& azDegs   = Ref->getAzimuthRef();
  auto& bwDegs   = Ref->getBeamWidthRef();
    
  // Grab the starting geometry for the Reflectivity gates
  float startKm     = Ref->getDistanceToFirstGateM() / 1000.0f;
  float gateWidthKm = Ref->getGateWidthKMs();

  for (size_t a = 0; a < numRadials; ++a) {
        float centerAz = azDegs[a] + (bwDegs[a] * 0.5f);
        for (size_t g = 0; g < numGates; ++g) {
            //LTAR may have been collected on difference azimuths or 
            //with different gate spacing. Adjust
            float centerRangeKm = startKm + (g * gateWidthKm) + (gateWidthKm * 0.5f);
            
            float qc_value = 0.0;

            // Query the LTAR projection for this exact location
            double ltarValue;
            int ltarRadialNo, ltarGateNo;
            
            if (LTARProj->getValueAtAzRange(centerAz, centerRangeKm, ltarValue, ltarRadialNo, ltarGateNo)) {
                
                // We found a spatial match!
                float reflValue = Z_data[a][g];
                
                if (rapio::Constants::isGood(reflValue) && rapio::Constants::isGood(ltarValue)) {
                    qc_value =  Ref_LTAR_combination(reflValue, ltarValue); 
                } else {
                    qc_value = rapio::Constants::MissingData;
                }
            } else {
                // The Reflectivity gate falls outside the physical bounds of the LTAR data
                qc_value = rapio::Constants::DataUnavailable;
            }
            QCdata[a][g] = qc_value;
        } //for g
  }//for a

  return QCmask;
}

}//end of namespace rapio
