#include <rPreProAI.h>
#include "fastMedian.h"
#include "calc_kdp.h"
#include "phase_computations.h"
#include "corr_attenuation.h"
#include "computeDR.h"
#include <iostream>

using namespace rapio;

/** PreProAI
 * @author John Krause
 **/
void
rPreProAI::declareOptions(RAPIOOptions& o)
{
  // NOTE: the i, I, l, O and r options are already defined in RAPIOAlgorithm.
  // i --> input index/url support
  // I --> input filter support default matches.
  // l --> notifier support (FAM files basically)
  // r --> realtime flag support
  // o.setDescription("WDSS2"); // Default for WDSS2/MRMS algorithms that you intend to copyright as part of WDSS2
  o.setDescription("rPreProAI computes Kdp, DR, outputs QC'd data with -Q ");
  o.setAuthors("John Krause, NSSL 2026, John.Krause@noaa.gov ");

  // An optional string param...default is "Test" if not set by user
  // o.optional("T", "Test", "Test option flag");

  // An optional boolean param, since boolean defaults are always false.
  // o.boolean("x", "Turn x on and off for some reason.  Basically add a -x to turn on a boolean flag that is false by default");

  // A required parameter (algorithm won't run without it).  Here there is no default since it's required, instead you can provide an example of the setting
  // o.require("Z", "method1", "Set this to anything, it's just an example");
  o.boolean("Q", "Output moments will be QC'd with a DR threshold -11");
}

/** RAPIOAlgorithms process options on start up */
void
rPreProAI::processOptions(RAPIOOptions& o)
{
  // This is an example of how to get your algorithm parameters
  // Stick them in instance variables you can use them later in processing.

  qc_option = o.getBoolean("Q");

  /*
   * myTest = o.getString("T");
   * myX = o.getBoolean("x");
   * myZ = o.getString("Z");
   * std::string xAsString = o.getString("x");
   * fLogInfo(" ************************x IS {} (as string {}", myX, xAsString);
   * fLogInfo(" ************************T IS {}", myTest);
   * fLogInfo(" ************************Z IS {}", myZ);
   */
  fLogInfo(" ************************QZ IS {}", qc_option);
}

void
rPreProAI::processPreProAI(std::map<std::string, std::shared_ptr<RadialSet> > & DataMap)
{
  //
  // Check the myDataMap for azimuthal alignment
  // Access the pointer from the map
  std::shared_ptr<RadialSet> Ref = myDataMap["Reflectivity"];
  std::shared_ptr<RadialSet> CC    = myDataMap["RhoHV"];
  std::shared_ptr<RadialSet> Zdr   = myDataMap["Zdr"];
  std::shared_ptr<RadialSet> PhiDP = myDataMap["PhiDP"];

  size_t numRadials = Ref->getNumRadials();
  auto azRef        = Ref->getAzimuthRef();
  auto azCC         = CC->getAzimuthRef();

  bool DQ_test = true;
  bool abort   = false;

  if (DQ_test) {
    if (!Ref) {
      fLogSevere("DQ test Reflectivity missing, abort");
      abort = true;
    }

    if (!CC) {
      fLogSevere("DQ test RhoHV missing, abort");
      abort = true;
    }

    if (!Zdr) {
      fLogSevere("DQ test Zdr missing, abort");
      abort = true;
    }
    if (!PhiDP) {
      fLogSevere("DQ test PhiDP missing, abort");
      abort = true;
    }

    if (CC->getNumRadials() != numRadials) {
      fLogSevere("DQ test numRadials {} test failed on cc {}, abort", numRadials, CC->getNumRadials());
      abort = true;
    }
    if (Zdr->getNumRadials() != numRadials) {
      fLogSevere("DQ test numRadials {} test failed on cc {}, abort", numRadials, Zdr->getNumRadials());
      abort = true;
    }
    if (PhiDP->getNumRadials() != numRadials) {
      fLogSevere("DQ test numRadials {} test failed on cc {}, abort", numRadials, PhiDP->getNumRadials());
      abort = true;
    }
    //Test actual retrieved azimuths
    for (int a = 0; a < numRadials; ++a) {
      if (fabs(azRef[a] - azCC[a]) > 0.1) {
        fLogSevere("DQ az check failed{} AzCheck: ref: {} cc: {} ", a, (float) azRef[a], azCC[a]);
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

  fLogInfo("---> starting processPreProAI(), past DQ step:");
  // find the minimum number of gates in the radial set and use that as our numGates
  //
  // Note we can test all the moments, but it's not neccessary,
  //  the data are collected as R,V,SPW and then CC,Zdr,PhiDP on the WSR-88D
  //
  // Reflectivity data is often collected out to 1832 gates while CC is at 1192 gates.
  // 
  size_t numGates = CC->getNumGates();

  if (Ref->getNumGates() < numGates) {
    numGates = Ref->getNumGates();
    fLogSevere("DQ check strange, numGates, Reflectivity has too few gates: ");
  }
  // ---------
  // Past DQ test at this point
  // Set the min_system_phase to the value that tells the code
  // to compute the min_system_phase and to return the result
  float min_system_phase = -999.0; // use -999.0 value for "unknown"

  //Both methods remove the inital system phase from the PhiDP. Meaning the inital
  //system phase is effectively zero
  //
  // correct_system_phase(PhiDP, CC, min_system_phase); //older method prone to "streaks"

  correct_system_phase_radial_by_radial(PhiDP, CC, min_system_phase); // newer method

  fLogDebug("min_system_phase {}", min_system_phase);

  //Compute KDP here:
  // 9 gate on WSR-88D
  std::shared_ptr<RadialSet> short_PhiDP = PhiDP->Clone();
  std::shared_ptr<RadialSet> short_Kdp   = compute_triple_median_Kdp(short_PhiDP, CC, 2250.);

  // 25 gate on WSR-88D
  std::shared_ptr<RadialSet> long_PhiDP = PhiDP->Clone();
  std::shared_ptr<RadialSet> long_Kdp   = compute_triple_median_Kdp(long_PhiDP, CC, 6250.);

  // combine the Kdp based on a Z threshold. Use short Kdp where Z > 40;
  // improve this by melding the data around the threshold rather than a simple step jump
  std::shared_ptr<RadialSet> prepro_Kdp = combine_Kdp(short_Kdp, long_Kdp, Ref, 40.0);

  fLogInfo("-------> processPreProAI(), Kdp finished:");

  std::shared_ptr<RadialSet> prepro_Ref = Ref->Clone();

  // Correct Zh for Horrizontal attenuation, using long_PhiDP
  correct_Zh_for_attenuation(prepro_Ref, long_PhiDP);

  std::shared_ptr<RadialSet> prepro_Zdr = Zdr->Clone();

  // Correct Zh for Horrizontal attenuation, using long_PhiDP
  correct_Zdr_for_attenuation(prepro_Zdr, long_PhiDP);

  // create the QC map from DR and a threshold, (Kilambi et. al. 2018)
  //  https://doi.org/10.1175/JTECH-D-17-0175.1
  std::shared_ptr<RadialSet> DR = computeDR(CC, Zdr);

  // this 2d median is very helpful in cleaning up the mask and eliminating
  // random speckling
  applyFast2DMedian(DR, 3, 3, 0.33);

  // The QC map is simply DR > threshold = non-meteorological Kilambi suggests -12
  // I think -11 works better for thunderstorms, mostly in the core where CC is low and
  // a ZdrColumn has formed. You can make your own masks using Reflectivity or CC or 
  // whatever.
  std::shared_ptr<RadialSet> QCmask = computeQCmask(DR, -11.0);

  // Smoothing to reduce variability
  applyFast2DMedian(prepro_Ref, 3, 3, 0.33);
  applyFast2DMedian(prepro_Zdr, 3, 3, 0.33);
  applyFast2DMedian(prepro_Kdp, 3, 3, 0.33);

  std::shared_ptr<RadialSet> prepro_CC = CC->Clone();

  applyFast2DMedian(prepro_CC, 3, 3, 0.33);
  fLogInfo("-------> processPreProAI(), 2D medians complete:");

  // Set the TypeName for your new product
  prepro_Ref->setTypeName("PrePro" + Ref->getTypeName());
  prepro_Ref->setDataAttributeValue("ColorMap", "Reflectivity");
  prepro_Zdr->setTypeName("PrePro" + Zdr->getTypeName());
  prepro_Zdr->setDataAttributeValue("ColorMap", "Zdr");
  prepro_CC->setTypeName("PrePro" + CC->getTypeName());
  prepro_CC->setDataAttributeValue("ColorMap", "RhoHV");

  prepro_Kdp->setTypeName("Kdp");
  prepro_Kdp->setUnits("deg/km");
  prepro_Kdp->setDataAttributeValue("ColorMap", "Kdp");

  long_PhiDP->setDataAttributeValue("ColorMap", "PhiDP");

  DR->setTypeName("DR");
  DR->setUnits("dB");
  DR->setDataAttributeValue("ColorMap", "DR");

  QCmask->setTypeName("QCmask");
  QCmask->setUnits("none");
  QCmask->setDataAttributeValue("ColorMap", "QCMask");

  auto preproRefQC = prepro_Ref->Clone();

  // check the options to decide if we apply the QCmask to the data
  // before we output it to disk.
  fLogInfo("-------> processPreProAI(), QC option {}", qc_option);
  if (qc_option) {
    fLogInfo("-------> processPreProAI(), QC started:");
    applyQCmask(preproRefQC, QCmask);
    preproRefQC->setTypeName("PrePro" + Ref->getTypeName() + "QC");
    myDataMap["prepro_RefQC"] = preproRefQC;
  }

  // add this to the DataMap
  myDataMap["prepro_Ref"]   = prepro_Ref;
  myDataMap["prepro_Zdr"]   = prepro_Zdr;
  myDataMap["prepro_CC"]    = prepro_CC;
  myDataMap["prepro_PhiDP"] = long_PhiDP;
  myDataMap["prepro_Kdp"]   = prepro_Kdp;
  myDataMap["prepro_DR"]    = DR;
  myDataMap["prepro_QC"]    = QCmask;
} // rPreProAI::processPreProAI

void
rPreProAI::processNewData(RAPIOData& d)
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
  auto r = d.datatype<RadialSet>();

  if (r != nullptr) {
    // Example for processing groups of subtypes.

    // Say you have data incoming where you
    // require N moments to all exist in order to process them.  For example, you need
    // 01.80 Reflectivity and 01.80 Velocity together to process your algorithm.

    // First save to a collection of radial sets for each subtype:

    // The types we must have... we want 4
    const std::vector<std::string> types = { "Reflectivity", "RhoHV", "PhiDP", "Zdr" };
    const std::string current = data_record[1];// the current data, "Zdr"

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
          fLogSevere("---> type_found mismatched elevations expected: {} found elev {}:", current, current_elevation);
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
        fLogSevere("---> type_found mismatched elevations expected: {} found elev {}:", current, current_elevation);
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
      processPreProAI(myDataMap);

      //   We need to output a file for each "prepro_*" subtype in the Datamap
      //   use the list processing
      //   This loop gives each myDataMap entry as a pair(string, <RadialSet>)
      for (auto & ppm : myDataMap) {
        // only output the added "prepro" radialsets
        //
        //The map key is a string, we eval it and determine if it needs
        // to be output
        if ((ppm.first).find("prepro") != std::string::npos) {
          // create output
          auto o = ppm.second; // The value of the map is a RadialSet

          // Standard echo of data to output.  Note it's the same data out as in here
          fLogDebug("--->Echoing {} {} product to output", o->getTypeName(), o->getElevationDegs() );

          std::map<std::string, std::string> myOverrides;
          // myOverrides["postwrite"] = "ldm";            // Do a standard pqinsert of final data file
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
} // rPreProAI::processNewData

void
rPreProAI::processHeartbeat(const Time& n, const Time& p)
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
  rPreProAI alg = rPreProAI();

  // Run this thing standalone.
  alg.executeFromArgs(argc, argv);
}
