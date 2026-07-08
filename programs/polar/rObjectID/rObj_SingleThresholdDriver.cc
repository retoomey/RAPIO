#include "rObj_SingleThresholdDriver.h"
#include "rRadialObjectIdentifier.h"
#include <boost/log/trivial.hpp>
#include <iostream>

using namespace rapio;

void Obj_SingleThresholdDriver::declareOptions(RAPIOOptions& o) {
  o.setDescription("RadialSet (polar) Driver for Single Treshold Object Identification.");
  o.setAuthors("John.Krause@noaa.gov");
  
  o.optional("threshold", "45.0", "Reflectivity Threshold value for object generation. Use different thresholds for different input data. ");
  
  declareProduct("SingleObj", "The generated radial set of single threshold objects > ");
}

void Obj_SingleThresholdDriver::processOptions(RAPIOOptions& o) {
  myThreshold = o.getFloat("threshold");
}

void Obj_SingleThresholdDriver::processNewData(RAPIOData& d) {
  // Extract the radial set from the incoming data payload
  auto rsIn = d.datatype<RadialSet>();
  if (!rsIn) return;

  // Restrict processing specifically to Reflectivity
  //if (rsIn->getTypeName() != "PreProReflectivity") {
  //    return; 
  //}

  // Note this is just for convience. You can remove the elevation
  // restrictions or enlarge them.
  //
  // 2. Restrict processing to the 0.5 degree elevation sweep
  // We use a tolerance of 0.1 to catch actual elevations like 0.48 or 0.52
  float currentElev = rsIn->getElevationDegs();
  if (std::abs(currentElev - 0.5f) > 0.1f) {
      return; // Skip this tilt if it is not the 0.5 sweep
  }

  fLogInfo("Processing tilt at elevation: {}", rsIn->getElevationDegs());

  // Pass data to the decoupled science class
  auto rsOut = RadialObjectIdentifier::ObjectID_Single_Threshold(rsIn, myThreshold);

  // Write the output to disk/memory/next algorithm
  if (rsOut) {
    std::map<std::string, std::string> overrides;
    // Tell the RAPIO writer to drop the fractional seconds from the filename
    overrides["FractionalTime"] = "false";
    writeOutputProduct("Objects", rsOut, overrides);
  }
}

// Standard entry point
int main(int argc, char * argv[]) {
  Obj_SingleThresholdDriver alg;
  alg.executeFromArgs(argc, argv);
}
