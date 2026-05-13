#include <rDRradial.h>
#include <iostream>
#include "computeDR.h"
#include <rConstants.h>

using namespace rapio;

/** DRradial Algorithm
 *
 * @author John Krause
 **/
void
rDRradial::declareOptions(RAPIOOptions& o)
{
  // NOTE: the i, I, l, O and r options are already defined in RAPIOAlgorithm.
  // i --> input index/url support
  // I --> input filter support default matches.
  // l --> notifier support (FAM files basically)
  // r --> realtime flag support
  // o.setDescription("WDSS2"); // Default for WDSS2/MRMS algorithms that you intend to copyright as part of WDSS2
  // o.setDescription("rDRradial does something");
  // o.setAuthors("Robert Toomey");

  // An optional string param...default is "Test" if not set by user
  // o.optional("T", "Test", "Test option flag");

  // An optional boolean param, since boolean defaults are always false.
  // o.boolean("x", "Turn x on and off for some reason.  Basically add a -x to turn on a boolean flag that is false by default");

  // A required parameter (algorithm won't run without it).  Here there is no default since it's required, instead you can provide an example of the setting
  // o.require("Z", "method1", "Set this to anything, it's just an example");
}

/** RAPIOAlgorithms process options on start up */
void
rDRradial::processOptions(RAPIOOptions& o)
{
  // This is an example of how to get your algorithm parameters
  // Stick them in instance variables you can use them later in processing.

  /*
   * myTest = o.getString("T");
   * myX = o.getBoolean("x");
   * myZ = o.getString("Z");
   * std::string xAsString = o.getString("x");
   * fLogInfo(" ************************x IS {} (as string {}", myX, xAsString);
   * fLogInfo(" ************************T IS {}", myTest);
   * fLogInfo(" ************************Z IS {}", myZ);
   */
}

void
rDRradial::processDRradial(std::map<std::string, std::shared_ptr<RadialSet>> & DataMap)
{
// Access the pointer from the map
    std::shared_ptr<RadialSet> CC = myDataMap["RhoHV"];
    std::shared_ptr<RadialSet> Zdr = myDataMap["Zdr"];
//
//  Identify the output, to look like Zdr
    std::shared_ptr<RadialSet> DR = Zdr->Clone();
     
//Check the data for azimuthal alignment
// Set the num_az and num_gates
    size_t numRadials = Zdr->getNumRadials(); 
    auto azZdr = Zdr->getAzimuthRef();
    auto azCC = CC->getAzimuthRef();
 
    bool DQ_test = true;
    bool abort = false;
    if (DQ_test) {
        if ( !CC) {
            fLogSevere("DQ test RhoHV missing, abort");
            abort = true;
        }
        if ( !Zdr) {
            fLogSevere("DQ test Zdr missing, abort");
            abort = true;
        }
        if (CC->getNumRadials() != numRadials ) {
            fLogSevere("DQ test numRadials {} test failed on cc {}, abort",  numRadials, CC->getNumRadials());
            abort = true;
        } 
        for(int a=0; a< numRadials; ++a){
            if ( fabs(azZdr[a]-azCC[a]) > 0.1) {
                fLogSevere("DQ az check failed{} AzCheck: zdr: {} cc: {} ", a, (float) azZdr[a], azCC[a] );
                abort = true;
            }
        }
    }

    if ( DQ_test) {
        if ( !abort ) {
            fLogDebug("DQ check passed: ");
        } else {
            fLogSevere("DQ check failed, return without processing: ");
            return;
        }
    } 
  
    fLogInfo("---> starting processDRradial(), past DQ step:");
    //find the minimum number of gates in the radial set and use that as our numGates
    //Note we can test all the moments, but it's not neccessary, 
    //  the data are collected as R,V,SPW and then CC,Zdr,PhiDP on the WSR-88D
    size_t numGates = Zdr->getNumGates();
    if (DQ_test) {
        if ( CC->getNumGates() < numGates ) {
            numGates = CC->getNumGates();
            fLogSevere("DQ check strange, numGates, CC has too few gates: ");
        }
    }
    //
    //We have checked out data and now we can start the processing
    //  using Zdr data as our starting point we want to compute the DR
    //  value for every point in the 2d array

    
    // Fetch the 2D arrays by reference for high-speed modification/access
    auto& zdrData = Zdr->getFloat2DRef();
    auto& ccData = CC->getFloat2DRef();

    auto& drData = DR->getFloat2DRef();

    for (size_t a = 0; a < numRadials; ++a) {
        for (size_t g = 0; g < numGates; ++g) {

            float ccVal = ccData[a][g];
            float zdrVal = zdrData[a][g];

            //Test for valid values:
            if (Constants::isGood(ccVal) && Constants::isGood(zdrVal) ) {
                drData[a][g] = computeDR(ccVal, zdrVal, Constants::MissingData);
            } else {
                if ( ccData[a][g] == Constants::RangeFolded ||
                     zdrData[a][g] == Constants::RangeFolded ) {
                    drData[a][g] = Constants::RangeFolded;
                } else {
                    drData[a][g] = Constants::MissingData;
                }
            }
        }
    }
    //FIXME:
    // Probably want to hit this with a 2D Median Filter

    fLogInfo("-------> processDRradial(), computations complete:");

    // Set the TypeName for your new product
    DR->setTypeName("DR");
    DR->setUnits("dB");
    DR->setDataAttributeValue("ColorMap", "DR");

    //add this to the DataMap
    myDataMap["DR"] = DR;
    //Note the myDataMap variable boes back to processNewData()
}

void
rDRradial::processNewData(RAPIOData& d)
{
  // Use d.record() and d.datatype() to get your wdssii information...
  // We'll probably add more helper features in the API as it grows.
  // Note: You don't have print this in your algorithm, it's just
  // an example
  //If I declare a volume here does it go output of scope?
  //Main data collection object
  // Example: "Reflectivity", <shared_ptr set of Reflectivity from RadialSet>
  // in protected area: std::map<std::string, std::shared_ptr<RadialSet>>  myDataMap;

  std::string s = "Data received: ";
  auto sel      = d.record().getSelections();
  //This looks like 0 = time; 1=name, 2=elev

  for (auto& s1:sel) {
    s = s + s1 + " ";
  }
  fLogInfo("{} from index {}", s, d.matchedIndexNumber());

  //For sanity let's go ahead and name the thing
  const std::vector<std::string>& data_record = d.record().getSelections(); 

  // Look for any data the system knows how to read
  // convert this data to a RadialSet which we can handle
  auto r = d.datatype<RadialSet>(); 
  
  if (r != nullptr) {
    // Example for processing groups of subtypes.

    // Say you have data incoming where you
    // require N moments to all exist in order to process them.  For example, you need
    // 01.80 Reflectivity and 01.80 Velocity together to process your algorithm.

    // First save to a collection of radial sets for each subtype:

    // The types we must have...
    const std::vector<std::string> types = {"RhoHV", "Zdr"}; 
    const std::string current = data_record[1];//the current data type like, "Zdr"         

    //Test if the type we have is one that we want. 
    bool type_found = false;
    if (std::find(types.begin(), types.end(), current) != types.end()) {
        type_found = true;
    }
    //
    //note we want all the types from the same elevation.
    // all data from 0.5 or all data from 1.5 deg elev
    // 
    fLogDebug("---> {} type_found {} :", type_found, current);
    if ( type_found ) {
          fLogDebug("---> type_found {} elev {}:", current, current_elevation);
      if (myDataMap.size() == 0 || current_elevation == MISSING_ELEV ) {
          myDataMap[current] = r; 
          //Init the current elevation for dq checks
          current_elevation = r->getElevationDegs();
          fLogDebug("--->in add {} DataMap.size {} Types.size {} :", current, myDataMap.size(), types.size());
      } else if ( fabs(current_elevation - r->getElevationDegs()) < 0.1 ) { 
          //Add this input to the collected volume:
          myDataMap[current] = r; 
      } else {
          //Warn that data might not be comming? 
          // identify when to abort processing and reset the volume and current elevation.
          if ( current_elevation != MISSING_ELEV && fabs(current_elevation - r->getElevationDegs()) > 0.1 ) {
            fLogSevere("---> type_found mismatched elevations expected: {} found elev {}:", current, current_elevation);
            fLogSevere("---> Data Reset. Elevation {} will not be run", current_elevation);
            myDataMap.clear(); 
            myDataMap[current] = r; 
            current_elevation = r->getElevationDegs();
          }

      }
    } else {
       if ( current_elevation != MISSING_ELEV && fabs(current_elevation - r->getElevationDegs()) > 0.1 ) {
       //Warn that data might not be comming? 
       // identify when to abort processing and reset the volume and current elevation.
           fLogSevere("---> type_found mismatched elevations expected: {} found elev {}:", current, current_elevation);
           fLogSevere("---> Data Reset. Elevation {} will not be run", current_elevation);
           myDataMap.clear(); 
           myDataMap[current] = r; 
           current_elevation = r->getElevationDegs();
       }
    }

    fLogDebug("---> {} DataMap.size {} Types.size {} :", current, myDataMap.size(), types.size());
    if (myDataMap.size() == types.size() ) { 
      fLogInfo("---> Full DataMap Collected: size:{} ", myDataMap.size());
      //We have all the moments we want, now compute the result
      //The output is adding moments (RadialSet) to the map with the prepro prefix like this:
      //myDataMap["prepro_Ref"] = prepro_Ref; where prepro_Ref is the modified RadialSet
      processDRradial( myDataMap ); 

      //   We need to output a file for each "prepro_*" subtype in the Datamap 
      //   use the list processing
      for( auto & ppm : myDataMap) {
          // only output the added "DR" radialsets 
          if ((ppm.first).find("DR") != std::string::npos ) {
              //create output
              auto o = ppm.second; //The back half of the map 
               
              // Standard echo of data to output.  Note it's the same data out as in here
              fLogDebug("--->Echoing {} {} product to output", o->getTypeName(), o->getElevationDegs() );
              
              std::map<std::string, std::string> myOverrides;
              //myOverrides["postwrite"] = "ldm";            // Do a standard pqinsert of final data file
              writeOutputProduct(o->getTypeName(), o, myOverrides); // Typename will be replaced by -O filters
              fLogInfo("--->Finished {} product to output", o->getTypeName());
             
          }
      }

      // If processed now you can clearn that Map if you don't want to chance processing this timeset again
      fLogDebug("---> Data Cleared!! ");
      myDataMap.clear(); 
      current_elevation = MISSING_ELEV ;

    } //We Found all the data we need 

  }// if (r != nullptr)

} // rDRradial::processNewData

void
rDRradial::processHeartbeat(const Time& n, const Time& p)
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
  rDRradial alg = rDRradial();

  // Run this thing standalone.
  alg.executeFromArgs(argc, argv);
}
