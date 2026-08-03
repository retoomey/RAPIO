#include <RAPIO.h>

namespace rapio {

float LWC_formula( float dbz ) {
    if (Constants::isGood(dbz)) {
        if ( dbz > 56.0 ) {
            dbz = 56.0;
        }
        float db = pow(10.0, 0.1*dbz);
        return 3.44E-06 *pow( db, (4.0/7.0)) ;
    } else {
        fLogSevere("--->LWC_formula:  Bad dbz Data dbz={}", dbz);
        return 0;
    }
}

/*
float VIL_formula( float dh, float dbz ) {
    if (Constants::isGood(dbz)) {
        if ( dbz > 56.0 ) {
            dbz = 56.0;
        }
        float db = pow(10.0, 0.1*dbz);
        if ( dh <= 0 ) {
            fLogSevere("--->VIL_formula: Bad Height Data dh={}", dh);
            return 0;
        }
        return 3.44E-06 *pow( db, (4.0/7.0) ) * dh;
    } else {
        fLogSevere("--->VIL_formula:  Bad dbz Data dbz={}", dbz);
        return 0;
    }
}
*/

class PolarVIL : public rapio::PolarAlgorithm {
public:
  PolarVIL() {}

  virtual void processNewData(rapio::RAPIOData& d) override {
    auto r = d.datatype<rapio::RadialSet>();
    if (r != nullptr) {
      // This safely adds the tilt to our virtual volume, but DOES NOT 
      // trigger the output generation.
      //
      //FIXME:
      //  Add radar name and reflectivity tag here?
      //
      processRadialSet(r); 
    }
  }

  virtual void processHeartbeat(const rapio::Time& n, const rapio::Time& p) override {
  
      // Grab the current tilts in the volume
      auto& tilts = myElevationVolume->getVolume();
      
      // Only generate a product if we actually collected data
      if (tilts.size() > 0) {
        
        // 1. Initialize the latest time tracker with the first tilt's time
        rapio::Time latestTime = tilts[0]->getTime();
        
        // 2. Iterate through the volume to find the absolute newest tilt
        for (const auto& tilt : tilts) {
          if (tilt->getTime() > latestTime) {
            latestTime = tilt->getTime();
          }
        }
    
        fLogInfo("Heartbeat fired! Generating VIL stamped at actual data time: {}", latestTime.getString());
        
        // 3. Trigger your VIL integration using the real data time instead of the heartbeat time 'p'
        processVolume(latestTime, 0.0f, "AzRan");
      }
  } 
  
  virtual void declareOptions(rapio::RAPIOOptions& o) override {
      o.setDescription("PolarVIL creates a single-radar polar VIL product.");
      // Force the default Input filter to ONLY accept Reflectivity
      // VIL is a reflectivity only algorithm. Nothing else is possible. 
      o.setDefaultValue("I", "Reflectivity");
  }

  // This is called automatically by PolarAlgorithm whenever a new tilt arrives (if -everytilt is set) 
  // or when the heartbeat fires.
  virtual void processVolume(const Time& useTime, float useElevDegs, const std::string& useSubtype) override {
    
    // Define our custom callback to handle the Az/Range iteration
    class VILCallback : public ElevationVolumeCallback {
    public:
      // Pre-allocate our 2D array: myHeights[tilt_index][gate_index]
      std::vector<std::vector<float>> myHeights;

      // 1. OVERRIDE handleBeginLoop to pre-compute our heights
      virtual void handleBeginLoop(RadialSetIterator* it, const RadialSet& radial) override {
        // Always call the base class first so it can pre-compute getRanges()
        ElevationVolumeCallback::handleBeginLoop(it, radial);

        auto& pc = getPointerCache();
        auto const numgates = radial.getNumGates();
        myHeights.resize(pc.size());
        
        // Loop through the tilts to calculate heights for every gate
        for (size_t i = 0; i < pc.size(); ++i) {
          RadialSetPointerCache* p = static_cast<RadialSetPointerCache*>(pc[i]);
          auto* rs = static_cast<RadialSet*>(p->dt);
          
          auto elevDegs = rs->getElevationDegs();
          auto stationHeightKMs = rs->getLocation().getHeightKM();
          
          myHeights[i].resize(numgates);
          for (size_t g = 0; g < numgates; ++g) {
            auto atRangeKM = getRanges()[i][g]; // Pre-calculated slant range
            
            // Calculate and cache the physical height for this specific tilt and gate
            myHeights[i][g] = Project::attenuationHeightKMs(stationHeightKMs, atRangeKM, elevDegs);
            //FIXME:
            //Test that the height computed is above the Terrain Height for this location or set to 
            // zero ? (or the Terrain Heigh)t. 
          }
        }
      }

      virtual void handleGate(RadialSetIterator* it) override {
        const auto atAzDegs = it->getCenterAzimuthDegs();
        const auto g = it->getCurrentGate();
        
        // Inside handleGate...
        
        float vil = 0.0f;
        bool weatherfound = false; //Was there a weather echo at this location?
        bool missingMask = false; //Did we look at this location or not?
        int radialNo, gateNo;
        double value;
        
        float lastHeightKM = -10000.0f;
        float lastLWC = 0.0f; // Track LWC to integrate between sweeps
        
        auto& pc = getPointerCache();
        
        // Iterate vertically through the tilts from top to bottom
        for (int i = pc.size() - 1; i >= 0; --i) {
          RadialSetPointerCache* p = static_cast<RadialSetPointerCache*>(pc[i]);
          RadialSetProjection* proj = static_cast<RadialSetProjection*>(p->project);
          
          auto atRangeKM = getRanges()[i][g];
        
          if (proj->getValueAtAzRange(atAzDegs, atRangeKM, value, radialNo, gateNo)) {
            missingMask = true; 
            
            if (Constants::isGood(value)) {
              float currentHeightKM = myHeights[i][g];
              float currentLWC = LWC_formula(static_cast<float>(value));
              
              if (lastHeightKM > -1000.0f) {
                // We have a top and bottom for this layer, integrate between them
                float hdiff = lastHeightKM - currentHeightKM;
                //extra check for Terrain when it's added.
                if (hdiff < 0 ) {
                    hdiff = 0;
                }
             
                //MRMS-WDSSII in w2img_PolarVIL does not average the values of Z between the layers
                //  w2Vil uses a "layer below" method. Not wrong but different. 
                //   
                // Average the LWC of the top and bottom of the layer
                float avgLWC = (lastLWC + currentLWC) / 2.0f; 
                vil += avgLWC * (hdiff * 1000.0f); 
              }
              
              // Store current values to act as the "top" for the next layer down
              lastHeightKM = currentHeightKM;
              lastLWC = currentLWC;
              weatherfound = true;
            }
          }
        }
        
        // --- NEW: Integrate the lowest beam down to the ground ---
        if (weatherfound) {
          // Get the radar's height MSL to represent the ground floor
          // (If you have a terrain model loaded, you could use terrain height instead)
          auto* lowest_rs = static_cast<RadialSet*>(static_cast<RadialSetPointerCache*>(pc[0])->dt);
          float groundHeightKM = lowest_rs->getLocation().getHeightKM(); 
          
          if (lastHeightKM > groundHeightKM) {
            float hdiff = lastHeightKM - groundHeightKM;
            // Assume the LWC of the lowest valid beam extends uniformly down to the ground
            vil += lastLWC * (hdiff * 1000.0f); 
          }
        }
        // Set the final computed value for this specific Az/Range bin
        if (weatherfound) {
          it->setValue(vil);
        } else {
          it->setValue(missingMask ? Constants::MissingData : Constants::DataUnavailable);
        }
      }
    };

    // Grab the current virtual volume
    auto& tilts = myElevationVolume->getVolume();
    if (tilts.size() < 1) { return; }

    // Use the lowest tilt as a template to construct our output 2D polar grid
    auto base = std::dynamic_pointer_cast<rapio::RadialSet>(tilts[0]);
    auto vil = createOutputRadialSet(useTime, useElevDegs, "VIL", useSubtype);
    if (vil == nullptr) { return; }

    // (Optional) Update attributes for the new product
    vil->setDataAttributeValue("ColorMap", "VIL");
    vil->setUnits("kg/m^2");

    // Execute the callback across the entire RadialSet
    VILCallback myCallback;
    myCallback.addVolume(*myElevationVolume);
    RadialSetIterator iter(*vil);
    iter.iterateRadialGates(myCallback);

    // Write the resulting product to disk (or the next pipeline step)
    writeOutputProduct(vil->getTypeName(), vil);
  }
};

} // namespace rapio

// Suggested command line for output evey 5 minutes:
//
// rPolarVIL -i /path/to/data/code_index.xml -o /path/to/output/location -sync "0 */5 * * * *"
//
// Standard RAPIO program entry point
int main(int argc, char* argv[]) {
  rapio::PolarVIL alg = rapio::PolarVIL();
  alg.executeFromArgs(argc, argv);
}
