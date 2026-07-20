#include <rPreProQC.h>
#include "computeFilter.h" 
#include <computeLTARmask.h>
#include <computeDRmask.h>
#include <computeStdPhiDPmask.h>
#include <computeCCmask.h>
#include <NBFdetection.h>
#include <boost/log/trivial.hpp>
#include <iostream>
#include <algorithm>
#include <fmt/core.h>
#include <fmt/ranges.h>

using namespace rapio;

// Your group/project/area for all of YOUR code.
// Here we're part of wdssii or hmet or anc, etc.
using namespace wdssii;

/** PreProQC: computes the QCmask for the input data
 * where 0 = non-meteorological
 * where 1 = meteorological 
 *
 * Design uses independant masks that are combined
 *
 * Current masks are:
 *    LTAR mask removes data (0) where LTAR overpowers reflectivity
 *    DR mask removes data (0) where DR / reflectivity suggest non-meteorological scatters
 *    
 *    Future Mask:
 *    Interference mask remove data in regions where CC > 1.0, suggests interference
 *    NBF mask, to be determined
 *    Terrain mask, to be determined, but may be covered by LTAR
 *
 * @author John Krause, John.Krause@noaa.gov 
 **/
void
rPreProQC::declareOptions(RAPIOOptions& o)
{
  // NOTE: the i, I, l, O and r options are already defined in RAPIOAlgorithm.
  // i --> input index/url support
  // I --> input filter support default matches.
  // l --> notifier support (FAM files basically)
  // r --> realtime flag support
  // R --> radar_name 
  // L --> LTAR reference directory
  // T --> Terrain reference directory (unused) 
  // o.setDescription("WDSS2"); // Default for WDSS2/MRMS algorithms that you intend to copyright as part of WDSS2
  o.setDescription("rPreProQC computes QC mask for meteorological/non-meteorological data identification");
  o.setAuthors("John Krause, NSSL 2026, John.Krause@noaa.gov ");

  // An optional string param...default is "Test" if not set by user
  // o.optional("T", "Test", "Test option flag");

  // An optional boolean param, since boolean defaults are always false.
  // o.boolean("x", "Turn x on and off for some reason.  Basically add a -x to turn on a boolean flag that is false by default");

  // A required parameter (algorithm won't run without it).  Here there is no default since it's required, instead you can provide an example of the setting
  // o.require("Z", "method1", "Set this to anything, it's just an example");
  o.optional("L", "", "Location of the LTAR reference data as XXXX.nc, a netcdf file with dBZ values named by radarID");
//  o.optional("T", "", "Location of the Terrrain reference data as XXXX.nc, a netcdf file with ? values named by radarID");
//  o.boolean("a", "Apply the QC to the Reflectivity (and whatever else....) ");
  o.require("R", "radar_name", "4 letter character ID for the radar, ex. KTLX ");
}

/** RAPIOAlgorithms process options on start up */
void
rPreProQC::processOptions(RAPIOOptions& o)
{
  // This is an example of how to get your algorithm parameters
  // Stick them in instance variables you can use them later in processing.

  ltar_dir = o.getString("L");
  //terrain_dir = o.getString("T");
  //apply_QC = o.getBoolean("a");
  //radar_name is required because we are using the PreProAI rapio output as 
  // our input
  radar_name = o.getString("R");
  /*
   * myTest = o.getString("T");
   * myX = o.getBoolean("x");
   * myZ = o.getString("Z");
   * std::string xAsString = o.getString("x");
   * fLogInfo(" ************************x IS {} (as string {}", myX, xAsString);
   */
  //fLogInfo(" ************************T IS {}", terrain_dir); //not yet implemented
  fLogInfo(" ************************L IS {}", ltar_dir);
  //fLogInfo(" ************************a IS {}", apply_QC);
  fLogInfo(" ************************R IS {}", radar_name);

}

void
rPreProQC::processPreProQC()
{
  //We can build the RAPIO name ourselves
  std::shared_ptr<rapio::RadialSet> inRef = myDataMap[radar_name + "_PreProReflectivity"];
  std::shared_ptr<rapio::RadialSet> CC = myDataMap[radar_name + "_PreProRhoHV"];
  std::shared_ptr<rapio::RadialSet> DR  = myDataMap[radar_name + "_DR"];
  std::shared_ptr<rapio::RadialSet> stdPhiDP  = myDataMap[radar_name + "_StdDiffPhase"];
  std::shared_ptr<rapio::RadialSet> PhiDPsm  = myDataMap[radar_name + "_SmoothedDifferentialPhase"];
 
  auto Ref = inRef->Clone(); 

  size_t numRadials = Ref->getNumRadials();
  auto azRef        = Ref->getAzimuthRef();
  auto azDR         = DR->getAzimuthRef();

  bool DQ_test = true;
  bool abort   = false;

  if (DQ_test) {
    if (!Ref) {
      fLogSevere("DQ test Reflectivity missing, abort");
      abort = true;
    }

    if (!DR) {
      fLogSevere("DQ test DR missing, abort");
      abort = true;
    }

    if (DR->getNumRadials() != numRadials) {
      fLogSevere("DQ test numRadials {} test failed on DR {}, abort", numRadials, DR->getNumRadials());
      abort = true;
    }
    //Test actual retrieved azimuths
    for (size_t a = 0; a < numRadials; ++a) {
      if (fabs(azRef[a] - azDR[a]) > 0.1) {
        fLogSevere("DQ az check failed{} AzCheck: ref: {} DR: {} ", a, (float) azRef[a], azDR[a]);
        abort = true;
      }
    }
  }

  if (DQ_test) {
    if (!abort) {
      fLogDebug("DQ check passed: ");
    } else {
      fLogSevere("DQ check failed, return without processing: ");
      return;
    }
  }

  fLogInfo("---> starting processPreProQC(), past DQ step:");
  // find the minimum number of gates in the radial set and use that as our numGates
  // Reflectivity data is often collected out to 1832 gates while DR is at 1192 gates.
  // 
  size_t numGates = DR->getNumGates();

  if (Ref->getNumGates() < numGates) {
    numGates = Ref->getNumGates();
    fLogSevere("DQ check strange, numGates, Reflectivity has too few gates: ");
  }
  // ---------
  // The QC mask is a combination of LTAR, DR, and (soon) Terrain.
  // If any of the QC methods identify bad data, then that mask is set to "0"
  // meteorological data is identified by "1". We assume that all locations with 
  // a valid Refelctivity value are valid unless identified as invalid. 
  // ---------
  // In general the technique moves away from single gate logic. We want to compare
  // non-local neighborhood values (max 9 gate) against the single gate values.
 
  auto LTAR_QCmask = Ref->Clone();
  //init the mask data properly then clone the init
  auto& LTAR_data = LTAR_QCmask->getFloat2DRef();
  auto& refData = Ref->getFloat2DRef();
  for (size_t a = 0; a < numRadials; ++a) {
        for (size_t g = 0; g < numGates; ++g) {
            float refVal = refData[a][g];
            if ( Constants::isGood(refVal) ) {
                LTAR_data[a][g] = 1;
            } else {
                LTAR_data[a][g] = 0;
            }
        }//for g
  }//for a
  //Now clone this properly initailized QCmask
  auto StdPhiDP_QCmask = LTAR_QCmask->Clone();
  auto DR_QCmask = LTAR_QCmask->Clone();
  auto CC_QCmask = LTAR_QCmask->Clone();
  auto NBF_detect = LTAR_QCmask->Clone();

  //See if we already have LTAR
  if (!LTAR) {
      //We don't have a valid LTAR field
      fLogInfo("Loading LTAR: ");
      std::string ltar_file = ltar_dir;
      
      // Ensure trailing slash
      if (!ltar_file.empty() && ltar_file.back() != '/') {
          ltar_file += "/";
      }
      ltar_file += Ref->getRadarName() + ".nc.gz";

      //LTAR is a private variable in the rPreProQC class see rPreProQC.h
      LTAR = readLTARData(ltar_file); 

      //Apply the dilation filter once.....
      LTAR = apply2DDilationFilter(LTAR, 3, 3, 0.33);
  }
 
  //added a little flex to the elevation field. LTAR is collected at 0.5 you can use
  //it below that level with some issues. 
  if (LTAR && Ref->getElevationDegs() <= 0.55) { 
      LTAR_QCmask = computeLTARmask(Ref, LTAR);
  }

  //should be handled by LTAR, but if you do not have LTAR you might need to use Terrain
  //if (terrain_directory != nullptr ) {
  //std::shared_ptr<rapio::RadialSet> Terrain_QCmask = computeTerrainmask(Ref, Terrain_file);
  //}
  
  // We want to use the Refsm field in both computeDRmask and computeStdPhiDPmask so 
  // compute it once outside and send it in.
  //
  //Blur the data so that the average value of the 3x3 box is 
  //used in the DR Threshold computation. We want to compare a local average 
  //reflectivity threshold to DR rather than a gate by gate value. The local average
  //of reflectivity tells us if the gate value is vaild, because it samples an
  //area around the gate rather than  a point target.
  // Note: try 5x5, but speed of 3x3 is faster? Either probably works. 
  std::shared_ptr<rapio::RadialSet> Refsm = apply2DBlurFilter(Ref, 5, 5, 0.33);

  // (circular) Depolarization Ratio (DR): Identifies non-meteorological targets. Based on Kilambi et al. 2018 (JTECH)
  // https://doi.org/10.1175/JTECH-D-17-0175.1 We introduce a dependance on Reflectivity, assuming that the LTAR
  // applied to the data has identified the ground returns we can focus on modifying Kilambi's thresholds to 
  // allow for detection of meteorological data inside of hail cores, where Zdr is high and CC is low.
  // 
 
  DR_QCmask = computeDRmask(Refsm, DR);

  //
  //Following the Kdp procedure, we noticed that the
  //standard deviation of differential phase was a great indicator of ground clutter and clutter
  //in general. We match it with reflectivity minimums to make sure we don't remove too much.
  // For lighter weight installations use just this QCmask
  //
  StdPhiDP_QCmask = computeStdPhiDPmask(Refsm, stdPhiDP);

  //
  //CC is unusually high in regions where there is low SNR and in interference. This continously
  //high CC is a good indicator of bad/low signal data. 
  float cc_filter_length = 2250.0; //meters
  CC_QCmask = computeCCmask(cc_filter_length, CC);

  //NBF is a problem where data is removed that should not be. All the dualpol moments become
  // unstable when there is excessive attenuation. This algorihtm tries to idenfity
  // locations where the NBF is occuring, so that we can limit the removal of reflectivity data
  // from these regions. The dualpol data is still bad in these regions, but often QPE wants
  // R(z) here rather than anything else (ex. R(A), R(Z,Zdr)) 
  float NBF_filter_length = 2250.0; //meters
  NBF_detect = NBFdetection(NBF_filter_length, Refsm, PhiDPsm, LTAR_QCmask, DR_QCmask, stdPhiDP);

  //Now combine the different QCmasks into a single QCmask
  auto QCmask = Ref->Clone();
  // init to good data, then knock off bad data. 
  QCmask->getFloat2D()->fill(1.0f);

  //
  auto& QC_data = QCmask->getFloat2DRef();
  auto& LTAR_QCdata = LTAR_QCmask->getFloat2DRef();
  auto& DR_QCdata = DR_QCmask->getFloat2DRef();
  auto& CC_QCdata = CC_QCmask->getFloat2DRef();
  auto& StdPhiDP_QCdata = StdPhiDP_QCmask->getFloat2DRef();
  auto& NBF_data = NBF_detect->getFloat2DRef();
  //combine all the indifividual masks into a single mask
  for (size_t a = 0; a < numRadials; ++a) {
        for (size_t g = 0; g < numGates; ++g) {
            bool NBF_flag = false;
            float refVal = refData[a][g]; //Allows missing data and range folded data flags
            if ( Constants::isGood(refVal) ) {
                //Combine the data in a way that allows the QMask to tell you 
                //which mask was applied for removeal LTAR == -1; DR == -2; both LTAR and DR == -3;
                //  The mask is ranked by agressiveness (subjective) where the least agressive
                //  Treatment LTAR is followed by DR and then finally stdPhiDP. This allows the user
                //  to select the "amount" of filtering they want. 
                //  
                //  Severe Weather Algs might only want LTAR and/or DR
                //
                //  Hydro Algs might want all of it. We can apply more filters this way in different
                //  ways to customize the usage.
                //
                if (StdPhiDP_QCdata[a][g] == 0 || 
                    LTAR_QCdata[a][g] == 0 || 
                    DR_QCdata[a][g] == 0 || 
                    CC_QCdata[a][g] == 0 ) 
                {
                    if( NBF_flag ) {
                    //once NBF is detected in the radial any 
                    //DQ issue is due to NBF
                        QC_data[a][g] = -6;
                    } else {
                        if ( LTAR_QCdata[a][g] == 0 ) {
                            if( LTAR_QCdata[a][g] == 0 && DR_QCdata[a][g] == 0 ) {
                                QC_data[a][g] = -2;
                            } else {
                                QC_data[a][g] = -1;
                            }
                        } else if ( DR_QCdata[a][g] == 0 ) {
                            QC_data[a][g] = -3;
                        } else if (StdPhiDP_QCdata[a][g] == 0 ) {
                            QC_data[a][g] = -4;
                        } else if (CC_QCdata[a][g] == 0 ) {
                            QC_data[a][g] = -5;
                        } else {
                            QC_data[a][g] = 0;
                        }
                    }
                } else {
                    QC_data[a][g] = 1;
                }
                //QC is all well and good. NBF overides it.
                if (NBF_data[a][g] == 1.0 ) {
                    NBF_flag = true; //once we find NBF it's all NBF
                    QC_data[a][g] = -6;
                }
            } else {
                QC_data[a][g] = refVal;
            }
        }//for g
  }//for a
  
  // add this to the DataMap
  QCmask->setTypeName("QCmask");
  QCmask->setDataAttributeValue("ColorMap", "QCmask");
  myDataMap["output_QCmask"]    = QCmask;
  // Create the ReflectivtyQC field here:
  //    you can also create CC or Zdr or ? QC field if you send in the data
  //    You can add the entire QC method to the PreProAI as an all-in-one
  //    option; (ToDo: Create PreProAI_QC)
  // 
      for (size_t a = 0; a < numRadials; ++a) {
            for (size_t g = 0; g < numGates; ++g) {
                float QCVal = QC_data[a][g];
                if ( Constants::isGood(refData[a][g]) ) {
                    if (QCVal <= 0 && QCVal != -6 ) {
                        refData[a][g] = Constants::MissingData;
                    }
                }
            }
      }
   Ref->setTypeName("PreProReflectivityQC");
   myDataMap["output_PreProReflectivityQC"] = Ref;

   //non-standard outputs for development
   bool dev_output = true;
   if (dev_output) {
       LTAR_QCmask->setTypeName("LTARQCmask");
       LTAR_QCmask->setDataAttributeValue("ColorMap", "QCmask");
       myDataMap["output_LTARQCmask"]    = LTAR_QCmask;

       DR_QCmask->setTypeName("DRQCmask");
       DR_QCmask->setDataAttributeValue("ColorMap", "QCmask");
       myDataMap["output_DRQCmask"]    = DR_QCmask;

       Refsm->setTypeName("RefsmQC");
       myDataMap["output_RefsmQC"] = Refsm;

       NBF_detect->setTypeName("NBFdetect");
       NBF_detect->setDataAttributeValue("ColorMap", "KMeans");
       myDataMap["output_NBFdetect"]    = NBF_detect;


   }

} // rPreProQC::processPreProQC

void
rPreProQC::processNewData(rapio::RAPIOData& d)
{
  // Main data collection object where we collect data we want
  // to process.
  //
  // Example: "Reflectivity"
  // in protected area: std::map<std::string, std::shared_ptr<RadialSet>>  myDataMap;

  std::string s = "Data received: ";
  auto sel      = d.record().getSelections();

  // This looks like 0=time; 1=name, 2=elev

  for (auto& s1:sel) {
    s = s + s1 + " ";
  }
  fLogInfo("{} from index {}", s, d.matchedIndexNumber());

  // For sanity let's go ahead and name the thing
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

    // The types we must have... we want 2
    //
    //Build rapio types with the radar_name out front
    //
    string rapio_reflectivity = radar_name + "_PreProReflectivity";
    string rapio_DR = radar_name + "_DR";
    string rapio_stdPhiDP = radar_name + "_StdDiffPhase";
    string rapio_CC = radar_name + "_PreProRhoHV";
    string rapio_PhiDP = radar_name + "_SmoothedDifferentialPhase";
    const std::vector<std::string> types = { rapio_reflectivity, rapio_DR, rapio_stdPhiDP, rapio_CC, rapio_PhiDP};
    const std::string current = data_record[1];// the current data, ex. "Zdr"

    // Test if the type we have is one that we want.
    bool type_found = false;
    if (std::find(types.begin(), types.end(), current) != types.end()) {
        type_found = true;
    }
    //
    // note we want all the types from the same elevation.
    // all data from 0.5 or all data from 1.5 deg elev don't mix elevations
    //
    fLogDebug("---> {} type_found {} :", type_found, current);
    if (type_found) {
      fLogDebug("---> type_found {} elev {}:", current, current_elevation);
      if ((myDataMap.size() == 0) || (current_elevation == MISSING_ELEV)) {
        //store the radialSet in our private class variable myDataMap
        myDataMap[current] = r;
        // Init the current elevation for dq checks
        current_elevation = r->getElevationDegs();
        fLogDebug("--->in add {} DataMap.size {} Types.size {} :", current, myDataMap.size(), types.size());
      } else if (fabs(current_elevation - r->getElevationDegs()) < 0.1) {
        // Add this input to the volume:
        myDataMap[current] = r;
      } else {
        // Warn that data might not be comming?
        // identify when to abort processing and reset the volume and current elevation.
        if ( (current_elevation != MISSING_ELEV) && (fabs(current_elevation - r->getElevationDegs()) > 0.1) ) {
          fLogSevere("---> type_found unexpected elevation: expected: {} found elev {}:", current, current_elevation);
          // 
          std::string map_keys = "";
          for (const auto& [key, _] : myDataMap) {
            map_keys += key + " ";
          }
          fLogDebug("---> DataMap.size {} Types.size {} | Available Keys: [ {} ]", myDataMap.size(), types.size(), map_keys);
          fLogSevere("---> Data Reset. Elevation {} will not be run", current_elevation);
          myDataMap.clear();
          myDataMap[current] = r;
          current_elevation  = r->getElevationDegs();
        }
      }
    } else {
      if ( (current_elevation != MISSING_ELEV) && (fabs(current_elevation - r->getElevationDegs()) > 0.1) ) {
        // Warn that data might not be comming?
        // identify when to abort processing and reset the volume and current elevation.
        fLogSevere("---> type_found unexpected elevations expected: {} found elev {}:", current, current_elevation);
        //fLogDebug("---> DataMap.size {} Types.size {} :", current, myDataMap.size(), types.size());
        // 
        std::string map_keys = "";
        for (const auto& [key, _] : myDataMap) {
            map_keys += key + " ";
        }
        fLogDebug("---> DataMap.size {} Types.size {} | Available Keys: [ {} ]", myDataMap.size(), types.size(), map_keys);
        fLogSevere("---> Data Reset. Elevation {} will not be run", current_elevation);
        myDataMap.clear();
        myDataMap[current] = r;
        current_elevation  = r->getElevationDegs();
      }
    }

    fLogDebug("---> {} DataMap.size {} Types.size {} :", current, myDataMap.size(), types.size());
    //This is the test we use to determine if we have all of our data.
    if (myDataMap.size() == types.size() ) {
      fLogInfo("---> Full DataMap Collected: size:{} ", myDataMap.size());
      // We have all the moments we want, now compute the result
      // The output is adding moments (RadialSet) to the map with the "prepro" prefix:
      processPreProQC();

      //   We need to output a file for each "output_*" subtype in the Datamap
      //   use the list processing
      //   This loop gives each myDataMap entry as a pair(string, <RadialSet>)
      for (auto & ppm : myDataMap) {
        // only output the added "output" radialsets
        //
        //The map key is a string, we eval it and determine if it needs
        // to be output
        if ((ppm.first).find("output") != std::string::npos) {
          // create output
          auto o = ppm.second; // The value of the map is a RadialSet

          // Standard echo of data to output.  Note it's the same data out as in here
          fLogDebug("--->Echoing {} {} product to output", o->getTypeName(), o->getElevationDegs() );

          std::map<std::string, std::string> myOverrides;
          writeOutputProduct(o->getTypeName(), o, myOverrides); // Typename will be replaced by -O filters
          fLogInfo("--->Finished {} product to output", o->getTypeName());
        }
      }

      //If processed now you can clean that Map. You don't want to chance processing this data again
      fLogDebug("---> Data Cleared!! ");
      myDataMap.clear();
      current_elevation = MISSING_ELEV;
    } // We Found all the data we need
  }// if (r != nullptr)
} // rPreProQC::processNewData

/*
void
rPreProQC::processHeartbeat(const Time& n, const Time& p)
{
  fLogInfo("Simple alg got a heartbeat...what do you want me to do?");
  // FIXME: longer example here maybe..
  // Some RadialSet I'm holding onto/modifying over time...now I write it every N time:
  // writeOutputProduct(r->getTypeName(), r); // Typename will be replaced by -O filters
}
*/

int
main(int argc, char * argv[])
{
  // Create your algorithm instance and run it.  Isn't this API better?
  rPreProQC alg = rPreProQC();

  // Run this thing standalone.
  alg.executeFromArgs(argc, argv);
}
