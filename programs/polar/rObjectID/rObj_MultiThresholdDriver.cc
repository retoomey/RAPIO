#include "rObj_MultiThresholdDriver.h"
#include "rRadialObjectIdentifier.h"
#include <boost/log/trivial.hpp>
#include <iostream>
#include <fmt/ranges.h>

using namespace rapio;

void Obj_MultiThresholdDriver::declareOptions(RAPIOOptions& o) {
  o.setDescription("2D Driver for Reflectivity Object Identification. Uses multiple Thresholds ");
  o.setAuthors("John.Krause@noaa.gov");
  
  o.optional("thresholds", "50.0,45.0,35.0", "Comma separated threshold values (e.g. reflectivity, 50,45,35)."); 
  declareProduct("MultiObj", "The generated radial set of mutli-threshold objects");
}

void Obj_MultiThresholdDriver::processOptions(RAPIOOptions& o) {
  myThresholds.clear();

  // 1. Get the raw string
  std::string threshStr = o.getString("thresholds");
  
  // 2. Split the string by commas using RAPIO's utility
  std::vector<std::string> parts;
  rapio::Strings::splitWithoutEnds(threshStr, ',', &parts);
  
  // 3. Convert to floats safely
  for (const auto& p : parts) {
      try {
          //cout << "Thresh: " << p << "\n";
          myThresholds.push_back(std::stof(p));
      } catch (...) {
          fLogSevere("Failed to parse threshold value: {}", p);
      }
  }

  // Fallback just in case the user passed an empty/invalid string
  if ( myThresholds.size() == 0 ) {
      myThresholds.push_back(50.0f);
      myThresholds.push_back(45.0f); 
      myThresholds.push_back(35.0f);
      fLogSevere("Failed to parse threshold value using reflectivty values");
  } 
}

void Obj_MultiThresholdDriver::processNewData(RAPIOData& d) {
  // Extract the radial set from the incoming data payload
  auto rsIn = d.datatype<RadialSet>();
  if (!rsIn) return;

  //Note you can use any input variable CC, Zdr, AzShear, Reflectivity, etc...
  // Restrict processing specifically to Reflectivity
  //if (rsIn->getTypeName() != "PreProReflectivity") {
  //    return; 
  //}

  // Note: This is just for my convienence, remove elevation restrictions if you want.
  // 2. Restrict processing to the 0.5 degree elevation sweep
  // We use a tolerance of 0.1 to catch actual elevations like 0.48 or 0.52
  float currentElev = rsIn->getElevationDegs();
  if (std::abs(currentElev - 0.5f) > 0.1f) {
      return; // Skip this tilt if it is not the 0.5 sweep
  }

  fLogInfo("Processing tilt at elevation: {}", rsIn->getElevationDegs());

  // Pass data to the decoupled science class
  //
  auto ObjectsRS = RadialObjectIdentifier::ObjectID_Multi_Threshold(rsIn, myThresholds);
  // That's it! Now you have a RadialSet of objects

  // Write the output to disk/memory/next algorithm
  if (ObjectsRS) {
    IOConfig overrides;
    // Tell the RAPIO writer to drop the fractional seconds from the filename
    overrides.set("FractionalTime", "false");
    writeOutputProduct("Objects", ObjectsRS, overrides);
  }
}

// Standard entry point
int main(int argc, char * argv[]) {
  Obj_MultiThresholdDriver alg;
  alg.executeFromArgs(argc, argv);
}
