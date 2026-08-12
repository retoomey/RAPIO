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
*/

    // ---------------------------------------------------------
    // DYNAMIC GRID CALCULATION (ALIGNED TO CONUS)
    // ---------------------------------------------------------
    
    
    std::shared_ptr<rapio::LatLonGrid> createAlignedSubGrid( float radiusKMs, std::shared_ptr<rapio::RadialSet> rs) {
        rapio::LLH radarLoc = rs->getLocation();
        float radarLat = radarLoc.getLatitudeDeg();
        float radarLon = radarLoc.getLongitudeDeg();
    
        // 2. Define our target domain limits AND the Master CONUS anchor
        float latSpacing = 0.01f;
        float lonSpacing = 0.01f;
       
        // DEFAULT snap to grid 
        // CONUS nw(50.0, -130.0) 
        float masterNorth = 50.0f;
        float masterWest = -130.0f;
    
        // 3. Convert 300 km to Lat/Lon degrees using RAPIO constants
        float latOffset = (radiusKMs / rapio::Constants::EarthRadiusKM) * rapio::Constants::DegreesPerRadian;
        float lonOffset = latOffset / std::cos(radarLat * rapio::Constants::RadiansPerDegree);
    
        // 4. Calculate the unaligned (raw) bounding box
        float rawNorth = radarLat + latOffset;
        float rawSouth = radarLat - latOffset;
        float rawWest  = radarLon - lonOffset;
        float rawEast  = radarLon + lonOffset;
    
        // 5. SNAP the boundaries to the master CONUS grid
        // We calculate how many 0.01 steps we are from the master anchor, round it, and multiply back.
        float snappedNorth = masterNorth - std::round((masterNorth - rawNorth) / latSpacing) * latSpacing;
        float snappedSouth = masterNorth - std::round((masterNorth - rawSouth) / latSpacing) * latSpacing;
        
        float snappedWest  = masterWest  + std::round((rawWest - masterWest) / lonSpacing) * lonSpacing;
        float snappedEast  = masterWest  + std::round((rawEast - masterWest) / lonSpacing) * lonSpacing;
    
        // Calculate dimensions using the snapped boundaries
        size_t numLats = static_cast<size_t>(std::round((snappedNorth - snappedSouth) / latSpacing));
        size_t numLons = static_cast<size_t>(std::round((snappedEast - snappedWest) / lonSpacing));
    
        // Create the perfectly aligned North-West anchor point
        rapio::LLH nwAnchor(snappedNorth, snappedWest, radarLoc.getHeightKM());
    
        // 6. Initialize the LatLonGrid 
        return rapio::LatLonGrid::Create(
            rs->getTypeName(),
            rs->getUnits(),
            nwAnchor,
            rs->getTime(),
            latSpacing, 
            lonSpacing,
            numLats, 
            numLons
        );
    }
//Note: Inaccurate nearest neighbor:
    //NOTE This uses a simple "nearest neighbor" projection. 
    class NearestNeighborCallback : public rapio::LatLonGridCallback {
    public:
        std::shared_ptr<rapio::DataProjection> proj;
        NearestNeighborCallback(std::shared_ptr<rapio::DataProjection> p) : proj(p) {}

        void handlePixel(rapio::LatLonGridIterator* iterator) override {
            // Project the Lat/Lon pixel into Az/Range space to sample the value
            double val = proj->getValueAtLL(
                iterator->getCurrentLatDegs(), 
                iterator->getCurrentLonDegs()  
            );
            iterator->setValue(static_cast<float>(val)); 
        }
    }; 

//Note: Fast
    class FiveGateCrossCallback : public rapio::LatLonGridCallback {
    public:
        std::shared_ptr<rapio::RadialSetProjection> rsProj;
        float radarLat, radarLon;
        
        // Grab the raw 2D array for ultra-fast memory access
        rapio::ArrayFloat2DPtr rawDataPtr; 
        int numRadials;
        int numGates;

        // Pass the RadialSet into the constructor to extract the raw array
        FiveGateCrossCallback(std::shared_ptr<rapio::RadialSetProjection> p, float rLat, float rLon, 
                             rapio::RadialSet& radialSet) 
            : rsProj(p), radarLat(rLat), radarLon(rLon),
              rawDataPtr(radialSet.getFloat2DPtr(rapio::Constants::PrimaryDataName)),
              numRadials(radialSet.getNumRadials()),
              numGates(radialSet.getNumGates()) {}

        void handlePixel(rapio::LatLonGridIterator* iterator) override {
            if (!rsProj) return;

            // 1. Get Lat/Lon
            float pixelLat = iterator->getCurrentLatDegs();
            float pixelLon = iterator->getCurrentLonDegs();

            // 2. Convert to Az/Range
            float azDegs;
            float rangeMeters;
            rapio::Project::LatLonToAzRange(
                radarLat, radarLon, pixelLat, pixelLon, azDegs, rangeMeters
            );

            // 3. Find the exact array indices for this Az/Range
            int r, g;
            if (!rsProj->AzRangeToRadialGate(azDegs, rangeMeters / 1000.0, r, g)) {
                iterator->setValue(rapio::Constants::DataUnavailable);
                return;
            }

            // 4. Perform the ultra-fast 5-gate cross average
            double sum = 0.0;
            int count = 0;

            // Helper lambda for bounds checking and accumulation
            auto addGate = [&](int testR, int testG) {
                // Ensure we don't read past the end of the radials range
                if (testG >= 0 && testG < numGates) {
                    // Handle 360-degree wrap around for radials (e.g., Azimuth 359 wraps to 0)
                    if (testR < 0) testR = numRadials - 1;
                    if (testR >= numRadials) testR = 0;

                    float val = (*rawDataPtr)[testR][testG];
                    
                    // Only average valid weather echoes
                    if (rapio::Constants::isGood(val)) {
                        sum += val;
                        count++;
                    }
                }
            };

            // Sample the "Cross" pattern (Center, Left, Right, Up, Down)
            addGate(r, g);     // Center
            addGate(r+1, g);   // Right (Azimuth +)
            addGate(r-1, g);   // Left  (Azimuth -)
            addGate(r, g+1);   // Up    (Range +)
            addGate(r, g-1);   // Down  (Range -)

            // 5. Assign the smoothed value
            if (count > 0) {
                iterator->setValue(static_cast<float>(sum / count));
            } else {
                iterator->setValue(rapio::Constants::DataUnavailable);
            }
        }
    };

    //Note: Slow
    class CressmanGridCallback : public rapio::LatLonGridCallback {
    public:
        std::shared_ptr<rapio::RadialSetProjection> rsProj;
        float radarLat, radarLon;

        // Pass the projection and radar coordinates into the callback
        CressmanGridCallback(std::shared_ptr<rapio::DataProjection> p, float rLat, float rLon) 
            : radarLat(rLat), radarLon(rLon) {
            // Cast down to RadialSetProjection to access polar-specific methods
            rsProj = std::dynamic_pointer_cast<rapio::RadialSetProjection>(p);
        }

        void handlePixel(rapio::LatLonGridIterator* iterator) override {
            if (!rsProj) return;

            // 1. Get the Lat/Lon of the current grid pixel
            float pixelLat = iterator->getCurrentLatDegs();
            float pixelLon = iterator->getCurrentLonDegs();

            // 2. Convert Lat/Lon to Azimuth/Range
            float centerAzDegs;
            float centerRangeMeters;
            rapio::Project::LatLonToAzRange(
                radarLat, radarLon, 
                pixelLat, pixelLon, 
                centerAzDegs, centerRangeMeters
            );

            double centerRangeKMs = centerRangeMeters / 1000.0;

            // 3. Define our 1km search radius
            double radiusKMs = 1.0;
            
            // Azimuth radius expands as you get closer to the radar
            double azRadius = (centerRangeKMs > 0) ? 
                (radiusKMs / centerRangeKMs) * rapio::Constants::DegreesPerRadian : 0;
            
            double sumValue = 0.0;
            double sumWeight = 0.0;
            int validCount = 0;

            // 4. Sample a grid patch around the center to approximate the 1km area
            for (double rOffset = -radiusKMs; rOffset <= radiusKMs; rOffset += (radiusKMs / 2.0)) {
                for (double aOffset = -azRadius; aOffset <= azRadius; aOffset += (azRadius / 2.0)) {
                    
                    double sampleAz = centerAzDegs + aOffset;
                    double sampleRange = centerRangeKMs + rOffset;
                    
                    // Wrap azimuth across North
                    if (sampleAz >= 360.0) sampleAz -= 360.0;
                    if (sampleAz < 0.0) sampleAz += 360.0;

                    double val;
                    int rIdx, gIdx;
                    
                    // Check if there is valid data at this specific offset
                    if (rsProj->getValueAtAzRange(sampleAz, sampleRange, val, rIdx, gIdx)) {
                        if (rapio::Constants::isGood(val)) {
                            
                            // Calculate physical distance between the center and this sample point
                            // Arc length ~ aOffset(radians) * centerRangeKMs
                            double arcDist = (aOffset * rapio::Constants::RadiansPerDegree) * centerRangeKMs;
                            double dist = std::sqrt((rOffset * rOffset) + (arcDist * arcDist));
                            
                            // Direct hit, perfectly centered
                            if (dist < std::numeric_limits<double>::epsilon()) { 
                                iterator->setValue(static_cast<float>(val));
                                return;
                            }

                            // Apply Cressman weight (1/D)
                            double weight = 1.0 / dist;
                            sumValue += val * weight;
                            sumWeight += weight;
                            validCount++;
                        }
                    }
                }
            }

            // 5. Calculate final interpolated value
            if (validCount > 0) {
                iterator->setValue(static_cast<float>(sumValue / sumWeight));
            } else {
                iterator->setValue(rapio::Constants::DataUnavailable);
            }
        }
    };

class PolarVIL : public rapio::PolarAlgorithm {
protected:
    bool polar_only = false;
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
      //VIL is a reflectivity only algorithm. Nothing else is possible. 
      o.setDefaultValue("I", "Reflectivity");
      //Call this program with -I NAME_Reflectivity, where NAME is the 4 (or 5) letter id of the radar
      //o.require("R", "radar_name", "4 letter character ID for the radar, ex. KTLX ");
      o.boolean("p", "Limits the rPolarVIL to polar only output");

  }

  virtual void processOptions(RAPIOOptions& o) override {
      polar_only = o.getBoolean("p");
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
    auto vil = createOutputRadialSet(useTime, 0.0, "VIL", useSubtype);
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

    if (!polar_only) {
        //An additional output.........LatLonVil
        //Added compute the gridded VIL product that is snapped to the
        //standard COUNUS wide output at a 400km  radius cutoff.
        //
        // 1. Get the radar's location from the computed RadialSet
        rapio::LLH radarLoc = vil->getLocation();
        float radarLat = radarLoc.getLatitudeDeg();
        float radarLon = radarLoc.getLongitudeDeg();
    
        // Create the subgrid snapped to the standard CONUS setup
        auto targetGrid = createAlignedSubGrid(400.0, vil); 
    
        // 6. Get the projection from the polar VIL RadialSet
        auto projection = vil->getProjection(rapio::Constants::PrimaryDataName);
    
        // 7. Iterate over the new grid and sample the polar data
        //   hardcoded sample of 5 gate "cross" pattern average
        rapio::LatLonGridIterator gridIt(*targetGrid); 
    
        // Instantiate the callback, passing in the projection, radar coordinates, and the RadialSet itself
        //  pick the type of data combination you want. 5gate is fast and reasonably accurate
        //
        FiveGateCrossCallback gridCb(
            std::dynamic_pointer_cast<rapio::RadialSetProjection>(projection), 
            radarLat, radarLon, 
            *vil
        );
    
        gridIt.iterate(gridCb); 
    
        // 8. Write the gridded single radar product to disk
        std::string radarName;
        vil->getString("radarName-value", radarName);
        targetGrid->setDataAttributeValue("radarName", radarName);
        targetGrid->setDataAttributeValue("ColorMap", "VIL");    
        // Set the Datatype to "VIL" and the Subtype to "LatLon"
        targetGrid->setTypeName("VIL");
        targetGrid->setSubType("LatLon");

    // Update your fileprefix to include the {subtype} token
        IOConfig myOverride;
        myOverride.set("fileprefix","{source}/{datatype}/{subtype}/00.00/{time}");
        writeOutputProduct(targetGrid->getTypeName(), targetGrid, myOverride); 
    }
  }
};

} // namespace rapio

// Suggested command line for output evey 5 minutes:
//
// rPolarVIL -i /path/to/data/code_index.xml -o /path/to/output/location -sync "0 */5 * * * *" -I RadarID_Reflectivity
// rPolarVIL -i /path/to/index/rapio_index.xml -sync "0 */5 * * * *" -o /path/to/output/KCYS20170612/ -I KCYS_Reflectivity
//
//NOTE: Call with -I NAME_Reflectivity where NAME is the 4 letter id of the radar, example: "KTLX_Reflectivity"
//  This is a single radar algorithm. There is a different and seperate CONUS VIL algorithm. The LatLonGrid
//  output for this algorithm is "snapped" to the standard wdssii grid so that derived single radar outputs can
//  easily be joined into a COUNUS output. 
//
// Standard RAPIO program entry point
int main(int argc, char* argv[]) {
  rapio::PolarVIL alg = rapio::PolarVIL();
  alg.executeFromArgs(argc, argv);
}
