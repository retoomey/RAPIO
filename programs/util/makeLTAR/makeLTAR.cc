#include <boost/log/trivial.hpp>
#include <iostream>
#include "makeLTAR.h"
#include <rConstants.h>

using namespace rapio;

namespace {
int getNearestRadialIndex(rapio::RadialSet & radialSet, float targetAzimuthDegs) {
    // References cannot be null, so we only need to check the radial count
    if (radialSet.getNumRadials() == 0) {
        return -1;
    }

    // RadialSetProjection creates a 1000-bin-per-degree lookup map
    // We use &radialSet to pass a pointer to the constructor
    rapio::RadialSetProjection projection("primary", &radialSet);

    int radialNo = -1;
    int gateNo = -1;
    
    // Test a dummy range (e.g. 10.0 km) to retrieve the radial index for the target azimuth
    if (projection.AzRangeToRadialGate(targetAzimuthDegs, 10.0, radialNo, gateNo)) {
        return radialNo;
    }

    return -1;
}

}

void
LTAR::declareOptions(RAPIOOptions& o)
{
  // NOTE: the i, I, l, O and r options are already defined in RAPIOAlgorithm.
  // i --> input index/url support
  // I --> input filter support default matches.
  // l --> notifier support (FAM files basically) //NOTE: this alg only runs on FAM
  // r --> realtime flag support
  // o.setDescription("WDSS2"); // Default for WDSS2/MRMS algorithms that you intend to copyright as part of WDSS2
  o.setDescription("makeLTAR creates the LTAR output for a radar, use makeLTAR_driver.py to launch");
  o.setAuthors("John Krause, NSSL 2026, John.Krause@noaa.gov ");

  // Radar name is a required parameter (algorithm won't run without it).  
  o.require("R", "radar_name", "The 4 letter (or 5) id of the radar");
}

/** RAPIOAlgorithms process options on start up */
void
LTAR::processOptions(RAPIOOptions& o)
{
   //Note: the algortihm really only runs on a fam file and dynamic generation using makeLTAR_driver.py
   radar_name = o.getString("R");
   fLogInfo(" ************************R IS {}", radar_name);
}

void
LTAR::processLTAR(std::map<std::string, std::shared_ptr<rapio::RadialSet>> & DataMap)
{
// Access the pointer from the map
    std::shared_ptr<rapio::RadialSet> Z = myDataMap["Reflectivity"];
//
//  Identify the output, to look like Z
    if (!Refl_sum ) {
        Refl_sum = Z->Clone();
        fLogInfo("Refl_sum init complete.");
        return;
    }
     
//Check the data for azimuthal alignment
// Set the num_az and num_gates
    size_t numRadials = Refl_sum->getNumRadials(); 
    auto azZ = Z->getAzimuthRef();
    auto azRefl_sum = Refl_sum->getAzimuthRef();
 
    //find the minimum number of gates in the radial set and use that as our numGates
    //Note we can test all the moments, but it's not neccessary, 
    //  the data are collected as R,V,SPW and then CC,Zdr,PhiDP on the WSR-88D
    size_t numGates = Refl_sum->getNumGates();
    
    if ( Z->getNumGates() < numGates ) {
        numGates = Z->getNumGates();
        fLogSevere("DQ check strange, numGates, incoming Z  has too few gates: ");
    }
    //
    //We have checked out data and now we can start the processing
    //  using Z data as our starting point we want to sum the Reflectivity 
    //  value for every point in the 2d array

    
    // Fetch the 2D arrays by reference for high-speed modification/access
    auto& z_Data = Z->getFloat2DRef();
    auto& rs_Data = Refl_sum->getFloat2DRef();

    for (size_t a = 0; a < numRadials; ++a) {
        auto az_index = getNearestRadialIndex(*Z, azRefl_sum[a]);
        if (az_index == -1) {
                fLogSevere("DQ az check failed{} returned {} AzCheck: z: {} refl_sum: {} ", a, az_index, (float) azZ[a], azRefl_sum[a] );
                return; 
        }
        for (size_t g = 0; g < numGates; ++g) {
            //Find the Z azimuth that matches the kept Refl_sum data
             
            float zVal = z_Data[az_index][g]; 
            //Test for valid values:
            if (Constants::isGood(zVal) ) {
                rs_Data[a][g] += zVal; 
            } 
        }
    }
    //increment the number of elevations processed 
    num_elevs += 1;     
    fLogInfo("Number of elevations: {}", num_elevs);
    //compute the LTAR output for this time step
    std::shared_ptr<rapio::RadialSet> LTAR = Refl_sum->Clone();

    auto & ltar_data = LTAR->getFloat2DRef();

    //the Clone process keeps the summation data we only need
    // to divide each gate by the number of elevations
    for (size_t a = 0; a < numRadials; ++a) {
        for (size_t g = 0; g < numGates; ++g) {
            float rsVal = rs_Data[a][g]; 
            //Test for valid values:
            if (rsVal > 0.0 ) {
                ltar_data[a][g] = rsVal/(float) num_elevs; 
            } else {
                //safe:
                ltar_data[a][g] = 0.0; 
            }
        }
    }
    
    // Set the TypeName for your new product
    LTAR->setTypeName("LTAR");
    LTAR->setUnits("dBz");
    LTAR->setDataAttributeValue("ColorMap", "Reflectivity");
    LTAR->setTime(Z->getTime());
    //add this to the DataMap
    myDataMap["LTAR"] = LTAR;
    //Note the myDataMap variable boes back to processNewData()
}

void
LTAR::processNewData(rapio::RAPIOData& d) {

  std::string s = "Data received: ";
  auto sel      = d.record().getSelections();
  //This looks like 0 = time; 1=name, 2=elev

  for (auto& s1:sel) {
    s = s + s1 + " ";
  }
  //fLogInfo("{} from index {}", s, d.matchedIndexNumber());

  //For sanity let's go ahead and name the thing
  const std::vector<std::string>& data_record = d.record().getSelections(); 

  // Look for any data the system knows how to read
  // convert this data to a RadialSet which we can handle
  auto r = d.datatype<rapio::RadialSet>(); 
  
  if (r != nullptr) {
    // Example for processing groups of subtypes.

    // Say you have data incoming where you
    // require N moments to all exist in order to process them.  For example, you need
    // 01.80 Reflectivity and 01.80 Velocity together to process your algorithm.

    // First save to a collection of radial sets for each subtype:

    // The types we must have...
    std::string rapio_ref = "Reflectivity";
    const std::vector<std::string> types = {rapio_ref}; 
    const std::string current = data_record[1];//the current data type like, "Zdr"         

    //Test if the type we have is one that we want. 
    bool type_found = false;
    if (std::find(types.begin(), types.end(), current) != types.end()) {
        type_found = true;
    }
    //
    //NOTE; we only want data from the LTAR elevation that we are interested in.
    //  We use 0.5 degrees, but you can also try < 0.5 degrees
    //  You could compute and keep LTAR data from all elevation angles or just a few
    //  LTAR can be applied on any elevation less than the collected elevation. With
    //  some error because LTAR will probably higher at lower elevations.
    // 
    fLogDebug("---> {} type_found {} :", type_found, current);
    if ( type_found ) {
          //fLogDebug("---> type_found {} elev {}:", current, current_elevation);
      if ( fabs(0.5 - r->getElevationDegs()) < 0.1 ) { 
          //Add this input to the collected data:
          myDataMap.clear(); 
          myDataMap[current] = r; 
      } 

    }

    fLogDebug("---> {} DataMap.size {} Types.size {} :", current, myDataMap.size(), types.size());
    if (myDataMap.size() == types.size() ) { 
      fLogInfo("---> Full DataMap Collected: size:{} ", myDataMap.size());
      //We have all the moments we want, now compute the result
      //The output is adding moments (RadialSet) to the map with the prepro prefix like this:
      processLTAR( myDataMap ); 

      //   We need to output a file for each "output_*" subtype in the Datamap 
      //   use the list processing
      for( auto & ppm : myDataMap) {
          // only output the added "DR" radialsets 
          if ((ppm.first).find("LTAR") != std::string::npos ) {
              //create output
              auto o = ppm.second; //The back half of the map 
               
              // Standard echo of data to output.  Note it's the same data out as in here
              fLogDebug("--->Echoing {} {} product to output", o->getTypeName(), o->getElevationDegs() );

              std::map<std::string, std::string> myOverrides;
              writeOutputProduct(o->getTypeName(), o, myOverrides); // Typename will be replaced by -O filters
              fLogInfo("--->Finished {} product to output", o->getTypeName());
             
          }
      }

      // If processed now you can clearn that Map if you don't want to chance processing this timeset again
      fLogDebug("---> Data Cleared!! ");
      myDataMap.clear(); 

    } //We Found all the data we need 

  }// if (r != nullptr)

} // LTAR::processNewData

void
LTAR::processHeartbeat(const Time& n, const Time& p)
{
  fLogInfo("Simple alg got a heartbeat...what do you want me to do?");
  // FIXME: longer example here maybe..
  // Some RadialSet I'm holding onto/modifying over time...now I write it every N time:
  // writeOutputProduct(r->getTypeName(), r); // Typename will be replaced by -O filters
}

int
main(int argc, char * argv[])
{
  // Create your algorithm instance and run it.  Isn't this API better?
  LTAR alg = LTAR();

  // Run this thing standalone.
  alg.executeFromArgs(argc, argv);
}
