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
  // Traditional(useTime, useElevDegs, useSubtype);
  Interpolated(useTime, useElevDegs, useSubtype);
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
      const auto atRangeKM = it->getCenterRangeMeters() / 1000.0;

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

        // ...see if we hit it...
        if (proj->getValueAtAzRange(atAzDegs, atRangeKM, Zb, radialNo, gateNo)) {
          missingMask = true; // Because we 'hit' radar coverage

          // ...and we also hit a good dbz value
          if (Constants::isGood(Zb) && (Zb >= DBZ_THRESH)) {
            // (i) find the maximum elevation angle over threshold.
            auto Tb = rs->getElevationDegs();

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
