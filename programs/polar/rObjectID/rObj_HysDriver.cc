#include "rObj_HysDriver.h"
#include "rRadialObjectIdentifier.h"
#include <boost/log/trivial.hpp>
#include <iostream>

using namespace rapio;

void Obj_HysDriver::declareOptions(RAPIOOptions& o) {
  o.setDescription("RadialSet (polar) Driver for Object Identification. Uses Lak Hysteresis threshold and size ");
  o.setAuthors("John.Krause@noaa.gov");
  
  o.optional("thresholds", "50.0,45.0,35.0", "Comma separated threshold values (e.g., 50,45,35)."); 
  o.optional("sizes", "3,7,15", "Comma separated minimum sizes for detection in number of gates"); 
  declareProduct("HysObj", "The generated radial set of Lak Hysteresis objects");
}

void Obj_HysDriver::processOptions(RAPIOOptions& o) {
  myThresholds.clear();

  // 1. Get the raw string
  std::string threshStr = o.getString("thresholds");
  
  // 2. Split the string by commas using RAPIO's utility
  std::vector<std::string> parts;
  rapio::Strings::splitWithoutEnds(threshStr, ',', &parts);
  
  // 3. Convert to floats safely
  for (const auto& p : parts) {
      try {
          myThresholds.push_back(std::stof(p));
      } catch (...) {
          fLogSevere("Failed to parse threshold value: {}", p);
      }
  }

  // 1. Get the raw string
  std::string sizesStr = o.getString("sizes");
  
  // 2. Split the string by commas using RAPIO's utility
  parts.clear();
  myMinSizes.clear(); 
  rapio::Strings::splitWithoutEnds(sizesStr, ',', &parts);
  
  // 3. Convert to floats safely
  for (const auto& p : parts) {
      try {
          // Parse as unsigned long long, then safely cast to size_t
          myMinSizes.push_back(static_cast<size_t>(std::stoull(p)));
      } catch (...) {
          fLogSevere("Failed to parse threshold value: {}", p);
      }
  }

  if (myThresholds.size() == 0 ) {
  // Fallback just in case the user passed an empty/invalid string
      myThresholds.push_back(50.0f);
      myThresholds.push_back(45.0f); 
      myThresholds.push_back(35.0f); 
      fLogSevere("No Threshold value provided, fallback to Reflectivity: {}", myThresholds);
  }

  if (myMinSizes.size() == 0 ) {
      myMinSizes.push_back(3);
      myMinSizes.push_back(7);
      myMinSizes.push_back(15);
      fLogSevere("No minSizes provided, fallback to Reflectivity: {}", myMinSizes);
  }

}

void Obj_HysDriver::processNewData(RAPIOData& d) {
  // Extract the radial set from the incoming data payload
  auto rsIn = d.datatype<RadialSet>();
  if (!rsIn) return;

  //FIXME: modfied to handle any input data
  // Use -I "Velocity" or -I "Reflectivity"
  // Predict processing name from -I  
  //if (rsIn->getTypeName() != "PreProReflectivity") {
  //    return; 
  //}

  // 2. Restrict processing to the 0.5 degree elevation sweep
  // Note: You can process objects at any elevation angle. We
  // restrict processing because I dont want to look at all the data
  // from all the elevations
  //
  // We use a tolerance of 0.1 to catch actual elevations like 0.48 or 0.52
  float currentElev = rsIn->getElevationDegs();
  if (std::abs(currentElev - 0.5f) > 0.1f) {
      return; // Skip this tilt if it is not the 0.5 sweep
  }

  fLogInfo("Processing tilt at elevation: {}", rsIn->getElevationDegs());

  // Pass data to the decoupled science class
  auto ObjectRS = RadialObjectIdentifier::ObjectID_Hysteresis(rsIn, myThresholds, myMinSizes);
  //  That's it! Now you have a radial set of objects

  // Write the output to disk/memory/next algorithm
  if (ObjectsRS) {
    std::map<std::string, std::string> overrides;
    // Tell the RAPIO writer to drop the fractional seconds from the filename
    overrides["FractionalTime"] = "false";
    writeOutputProduct("Objects", ObjectsRS, overrides);
  }
}

// Standard entry point
int main(int argc, char * argv[]) {
  Obj_HysDriver alg;
  alg.executeFromArgs(argc, argv);
}
