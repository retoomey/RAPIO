#include <rEchoTop.h>

#include <iostream>

using namespace rapio;

void
EchoTop::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "EchoTop creates a polar maximum product.");
}

void
EchoTop::processVolume(const Time& useTime, float useElevDegs, const std::string& useSubtype)
{
  //Traditional(useTime, useElevDegs, useSubtype);
  Interpolated(useTime, 0.0, useSubtype); // Seems MRMS uses 0 degrees?
  //VerticalColumnCoverage(useTime, useElevDegs, useSubtype);
}

void
EchoTop::VerticalColumnCoverage(const Time& useTime, float useElevDegs, const std::string& useSubtype)
{
  /** Callback for vertical column coverage */
  class VCC : public ElevationVolumeCallback {
public:

    /** For each gate, we've gonna take the absolute max of the other gates
     * in the volume vertically */
    virtual void
    handleGate(RadialSetIterator * it)
    {
      // We want the current center azimuth/range of our RadialSet gate,
      // this will be used to project into the other RadialSets.
      const auto atAzDegs  = it->getCenterAzimuthDegs();
      const auto atRangeKM = it->getCenterRangeMeters() / 1000.0;

      LengthKMs totalKMs = 0;
      LengthKMs prevBotKMs = 20000;
      bool foundOne = false;
      bool missingMask = false;
      int radialNo, gateNo;
      double value;

      // Allow overlapping beam spread in the metric calculation.
      // No means union of the beam spread vs double counting.
      bool noOverLap = true; 

      // Go through the vertical column
      auto& pc = getPointerCache();

      for (int i = pc.size() - 1; i >= 0; --i) {
        // Static casts are cheap.  Add volume should have prechecked it all
        RadialSetPointerCache * p  = static_cast<RadialSetPointerCache *>(pc[i]);
        RadialSetProjection * proj = static_cast<RadialSetProjection *>(p->project);

        // ...see if we hit it...
        if (proj->getValueAtAzRange(atAzDegs, atRangeKM, value, radialNo, gateNo)) {
          missingMask = true; // Because we 'hit' radar coverage

          // We only care about height at top and bottom of our beam.  Note, we
          // assume the tilts don't overlap?  Or maybe we don't care? In theory,
          // overlapping might be 'stronger' confidence so for now we'll just join
          // and not union.
          auto * rs = static_cast<RadialSet *>(p->dt);
          auto elevDegs = rs->getElevationDegs();
          auto stationHeightKMs = rs->getLocation().getHeightKM(); // could optimize out

          AngleDegs bw          = (*p->bw)[radialNo];
          // Jitters with beamwidht but slightly more accurate. Without this each radial
          // is the exact same.
          auto upElevDegs       = elevDegs + (0.5 * bw);
          //auto upElevDegs       = elevDegs + (0.5);
          auto topKMs = Project::attenuationHeightKMs(stationHeightKMs, atRangeKM, upElevDegs);
          auto botElevDegs    = elevDegs - (0.5 * bw);
          //auto botElevDegs    = elevDegs - (0.5);
          auto botKMs = Project::attenuationHeightKMs(stationHeightKMs, atRangeKM, botElevDegs);

          // This gets rid of overlapping heights.  Debating if we double count beam overlaying
          // or not.  Could be an option.
          if (noOverLap && (topKMs > prevBotKMs)){
             topKMs = prevBotKMs;
          }

          // A range of hit in the vertical column;
          if (topKMs > botKMs){ // should always be, right?
            auto rangeKMs = topKMs-botKMs;
            totalKMs += rangeKMs;
          }
          prevBotKMs = botKMs;
          foundOne = true;
        }
      }

      // Set final output value
      if (foundOne) {
        it->setValue(totalKMs);
      } else {
        it->setValue(missingMask ? Constants::MissingData : Constants::DataUnavailable);
      }
    } // handleGate
  };

  // FIXME:
  // Ahh what if we're trying to do multi array single output?
  // We could create RadialSets then merge them later.  Or maybe
  // the API could handle direct arrays?  Basically different
  // data array pointers
  auto& tilts = myElevationVolume->getVolume();

  if (tilts.size() < 1) { return; }

  auto base = std::dynamic_pointer_cast<rapio::RadialSet>(tilts[0]);
  auto set  = createOutputRadialSet(
    useTime,
    useElevDegs,
    base->getTypeName() + "_VCC",
    useSubtype);

  if (set == nullptr) { return; }
  set->setDataAttributeValue("ColorMap", "EchoTop");
  set->setUnits("Km");
  LogInfo(*myElevationVolume << "\n");

  VCC myCallback;

  myCallback.addVolume(*myElevationVolume);
  RadialSetIterator iter(*set);

  iter.iterateRadialGates(myCallback);

  // Write product
  std::map<std::string, std::string> myOverride;

  writeOutputProduct(set->getTypeName(), set, myOverride);
}

void
EchoTop::Traditional(const Time& useTime, float useElevDegs, const std::string& useSubtype)
{
  /** Callback for traditional echotop algorithm */
  class TraditionalET : public ElevationVolumeCallback {
public:

    /** For each gate, we've gonna take the absolute max of the other gates
     * in the volume vertically */
    virtual void
    handleGate(RadialSetIterator * it)
    {
      // We want the current center azimuth/range of our RadialSet gate,
      // this will be used to project into the other RadialSets.
      const auto atAzDegs  = it->getCenterAzimuthDegs();
      const auto atRangeKM = it->getCenterRangeMeters() / 1000.0;

      float echoTop = 0.0;
      bool foundOne = false;
      bool missingMask = false;
      int radialNo, gateNo;
      double value;

      // Go through the vertical column
      auto& pc = getPointerCache();

      for (int i = pc.size() - 1; i >= 0; --i) { // Echo top is top down, volume is sorted
        // Static casts are cheap.  Add volume should have prechecked it all
        RadialSetPointerCache * p  = static_cast<RadialSetPointerCache *>(pc[i]);
        RadialSetProjection * proj = static_cast<RadialSetProjection *>(p->project);

        // ...see if we hit it...
        if (proj->getValueAtAzRange(atAzDegs, atRangeKM, value, radialNo, gateNo)) {
          missingMask = true; // Because we 'hit' radar coverage

          // Hit a good dbz value
          if (Constants::isGood(value) && (value >= 18)) {
            auto * rs = static_cast<RadialSet *>(p->dt);
            // top of 3dB beam
            AngleDegs bw          = (*p->bw)[radialNo];
            auto elevDegs         = rs->getElevationDegs() + (0.5 * bw);
            auto stationHeightKMs = rs->getLocation().getHeightKM();
            echoTop  = Project::attenuationHeightKMs(stationHeightKMs, atRangeKM, elevDegs);
            foundOne = true;
            break;
          }
        }
      }

      // Set final output value
      if (foundOne) {
        it->setValue(echoTop);
      } else {
        it->setValue(missingMask ? Constants::MissingData : Constants::DataUnavailable);
      }
    } // handleGate
  };

  auto& tilts = myElevationVolume->getVolume();

  if (tilts.size() < 1) { return; }

  auto base = std::dynamic_pointer_cast<rapio::RadialSet>(tilts[0]);
  auto set  = createOutputRadialSet(
    useTime,
    useElevDegs,
    base->getTypeName() + "_EchoTop",
    useSubtype);

  if (set == nullptr) { return; }
  set->setDataAttributeValue("ColorMap", "EchoTop");
  set->setUnits("km");
  LogInfo(*myElevationVolume << "\n");

  TraditionalET myCallback;

  myCallback.addVolume(*myElevationVolume);
  RadialSetIterator iter(*set);

  iter.iterateRadialGates(myCallback);

  // Write product
  std::map<std::string, std::string> myOverride;

  writeOutputProduct(set->getTypeName(), set, myOverride);
} // EchoTop::processVolume

void
EchoTop::Interpolated(const Time& useTime, float useElevDegs, const std::string& useSubtype)
{
  /** Callback for traditional echotop algorithm */
  class InterpolatedET : public ElevationVolumeCallback {
public:

    /** For each gate, handle vertical echotop using Lak's interpolated algorithm */
    virtual void
    handleGate(RadialSetIterator * it)
    {
      // Hardcoded for the moment.  FIXME: Params maybe?
      constexpr float DBZ_THRESH  = 18;
      constexpr float DBZ_MISSING = -14; // 88d.  How about other radars?

      // We want the current center azimuth/range of our RadialSet gate,
      // this will be used to project into the other RadialSets.
      const auto atAzDegs  = it->getCenterAzimuthDegs();
      const auto atGroundKM = it->getCenterRangeMeters() / 1000.0;

      float echoTop = 0.0;
      bool foundOne = false;
      bool missingMask = false;
      int radialNo, gateNo;
      double Zb;
      double Za;
      bool topTilt = true;

      // Go down the vertical column
      auto& pc = getPointerCache();

      for (int i = pc.size() - 1; i >= 0; --i) { // Echo top is top down, volume is sorted
        // Static casts are cheap.  Add volume should have prechecked it all
        RadialSetPointerCache * p  = static_cast<RadialSetPointerCache *>(pc[i]);
        RadialSetProjection * proj = static_cast<RadialSetProjection *>(p->project);
        auto * rs = static_cast<RadialSet *>(p->dt);
        auto Tb = rs->getElevationDegs();

        // ...see if we hit it...
        // Humm forgot to project the 'ground' of the polar output to the tilt
        // We could cache this per radial/tilt.  Also MRMS does this per RadialSet,
        // which trades memory for speed.  Currently giving up speed for memory
        //auto atRangeKM = atGroundKM;
        // This is SLOW.  Cos is slow.  We should add api to cache this per RadialSet.
        // Should just need one radial
        // FIXME: Trying to get things correct, then I'll speed them up/improve
        // iterator API.
        auto atRangeKM = Project::groundToSlantRangeKMs(atGroundKM, Tb);
        if (proj->getValueAtAzRange(atAzDegs, atRangeKM, Zb, radialNo, gateNo)) {
          missingMask = true; // Because we 'hit' radar coverage

          // ...and we also hit a good dbz value
          if (Constants::isGood(Zb) && (Zb >= DBZ_THRESH)) {
            // (i) find the maximum elevation angle over threshold.
            //auto Tb = rs->getElevationDegs();

            // (ii) If we're not the highest elevation scan in the virtual volume
            float elevDegs;
            if (!topTilt) {
              // Look above to the next elevation then.  On valid hit use the value,
              // otherwise use the DBZ_MISSING
              // FIXME: Could make a method for this?  We're grabbing the pointer cache
              // of the tilt above us and checking value
              RadialSetPointerCache * p2  = static_cast<RadialSetPointerCache *>(pc[i + 1]);
              RadialSetProjection * proj2 = static_cast<RadialSetProjection *>(p2->project);
              auto * rs2 = static_cast<RadialSet *>(p2->dt);
              auto Ta    = rs2->getElevationDegs();

              if (proj2->getValueAtAzRange(atAzDegs, atRangeKM, Za, radialNo, gateNo)) {
                if (!(Constants::isGood(Za) && (Za >= DBZ_MISSING))) {
                  Za = DBZ_MISSING;
                }
              }
              // Lak's formula (1)
              elevDegs = (DBZ_THRESH - Za) * (Tb - Ta) / (Zb - Za) + Tb;
            } else {
              // (iii) If we're the highest elevation, then et = elev + beamwidth/2.0
              AngleDegs bw = (*p->bw)[radialNo];
              elevDegs = rs->getElevationDegs() + (0.5 * bw);
            }

            // Actual echo top value finally
            auto stationHeightKMs = rs->getLocation().getHeightKM();
            echoTop = Project::attenuationHeightKMs(stationHeightKMs, atRangeKM, elevDegs);

            foundOne = true;
            break;
          }
        }
        topTilt = false;
      }

      // Set final output value
      if (foundOne) {
        it->setValue(echoTop);
      } else {
        it->setValue(missingMask ? Constants::MissingData : Constants::DataUnavailable);
      }
    } // handleGate
  };

  auto& tilts = myElevationVolume->getVolume();

  if (tilts.size() < 1) { return; }

  auto base = std::dynamic_pointer_cast<rapio::RadialSet>(tilts[0]);
  auto set  = createOutputRadialSet(
    useTime,
    useElevDegs,
    base->getTypeName() + "_EchoTop",
    useSubtype);

  if (set == nullptr) { return; }
  set->setDataAttributeValue("ColorMap", "EchoTop");
  set->setUnits("km");
  LogInfo(*myElevationVolume << "\n");

  InterpolatedET myCallback;

  myCallback.addVolume(*myElevationVolume);
  RadialSetIterator iter(*set);

  iter.iterateRadialGates(myCallback);

  // Write product
  std::map<std::string, std::string> myOverride;

  writeOutputProduct(set->getTypeName(), set, myOverride);
} // EchoTop::Interpolated

int
main(int argc, char * argv[])
{
  // Create your algorithm instance and run it.  Isn't this API better?
  EchoTop alg = EchoTop();

  // Run this thing standalone.
  alg.executeFromArgs(argc, argv);
}
