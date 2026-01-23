#include <rEchoTop.h>

#include <rPolarAlgorithm.h>

#include <iostream>

using namespace rapio;
static LengthKMs MAXKMS = 0;

/** Callback for vertical column coverage */
class VCC : public PolarAlgorithm::ElevationVolumeCallback {
public:

  /** For each gate, we've gonna take the absolute max of the other gates
   * in the volume vertically */
  virtual void
  handleGate(RadialSetIterator * it)
  {
    // We want the current center azimuth/range of our RadialSet gate,
    // this will be used to project into the other RadialSets.
    const auto atAzDegs = it->getCenterAzimuthDegs();
    const auto g        = it->getCurrentGate();

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
      auto atRangeKM = getRanges()[i][g]; // ground to slant range cache

      // ...see if we hit it...
      if (proj->getValueAtAzRange(atAzDegs, atRangeKM, value, radialNo, gateNo)) {
        missingMask = true; // Because we 'hit' radar coverage

        // We only care about height at top and bottom of our beam.  Note, we
        // assume the tilts don't overlap?  Or maybe we don't care? In theory,
        // overlapping might be 'stronger' confidence so for now we'll just join
        // and not union.
        auto * rs             = static_cast<RadialSet *>(p->dt);
        auto elevDegs         = rs->getElevationDegs();
        auto stationHeightKMs = rs->getLocation().getHeightKM(); // could optimize out

        AngleDegs bw = (*p->bw)[radialNo];
        // Jitters with beamwidht but slightly more accurate. Without this each radial
        // is the exact same.
        auto upElevDegs = elevDegs + (0.5 * bw);
        // auto upElevDegs       = elevDegs + (0.5);
        auto topKMs      = Project::attenuationHeightKMs(stationHeightKMs, atRangeKM, upElevDegs);
        auto botElevDegs = elevDegs - (0.5 * bw);
        // auto botElevDegs    = elevDegs - (0.5);
        auto botKMs = Project::attenuationHeightKMs(stationHeightKMs, atRangeKM, botElevDegs);

        // This gets rid of overlapping heights.  Debating if we double count beam overlaying
        // or not.  Could be an option.
        if (noOverLap && (topKMs > prevBotKMs)) {
          topKMs = prevBotKMs;
        }

        // A range of hit in the vertical column;
        if (topKMs > botKMs) { // should always be, right?
          auto rangeKMs = topKMs - botKMs;
          totalKMs += rangeKMs;
        }
        prevBotKMs = botKMs;
        foundOne   = true;
      }
    }

    // Set final output value
    if (foundOne) {
      if (totalKMs > MAXKMS) { MAXKMS = totalKMs; }

      // Temp we're gonna do a weight.  Scale at around 22 kilometers for now
      float weight = totalKMs / 22.0;

      #if 0
      // Currently letting 2D resolver just multiple the range in.  Also
      // it has a parameter for changing range weight.
      // Research question would be if it's always the case.

      // FIXME: probably will be parameter?  Should all polar have a sigma option?
      constexpr float sigma    = 50;
      constexpr float variance = 1.0 / (25.0 * (sigma * sigma));
      const auto rangeKMs      = it->getCenterRangeMeters() * 0.001;
      float range = VolumeValueResolver::rangeToWeight(rangeKMs, variance);

      // So use our new metric * range weight drop off;
      // This is all experimentation
      weight = range * weight;
      #endif // if 0

      it->setValue(weight);
    } else {
      it->setValue(missingMask ? Constants::MissingData : Constants::DataUnavailable);
    }
  } // handleGate
};

/** Callback for traditional echotop algorithm */
class TraditionalET : public PolarAlgorithm::ElevationVolumeCallback {
public:

  /** For each gate, we've gonna take the absolute max of the other gates
   * in the volume vertically */
  virtual void
  handleGate(RadialSetIterator * it)
  {
    // We want the current center azimuth/range of our RadialSet gate,
    // this will be used to project into the other RadialSets.
    const auto atAzDegs = it->getCenterAzimuthDegs();
    const auto g        = it->getCurrentGate();

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
      auto atRangeKM = getRanges()[i][g]; // ground to slant range cache

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

/** Callback for Lak/Kurt's interpolated 2014 paper echotop algorithm */
class InterpolatedET : public PolarAlgorithm::ElevationVolumeCallback {
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
    const auto atAzDegs = it->getCenterAzimuthDegs();
    const auto g        = it->getCurrentGate();

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
      auto * rs      = static_cast<RadialSet *>(p->dt);
      auto Tb        = rs->getElevationDegs();
      auto atRangeKM = getRanges()[i][g]; // ground to slant range cache

      if (proj->getValueAtAzRange(atAzDegs, atRangeKM, Zb, radialNo, gateNo)) {
        missingMask = true; // Because we 'hit' radar coverage

        // ...and we also hit a good dbz value
        if (Constants::isGood(Zb) && (Zb >= DBZ_THRESH)) {
          // (i) find the maximum elevation angle over threshold.
          // auto Tb = rs->getElevationDegs();

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

void
EchoTop::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "EchoTop polar algorithm.");
}

void
EchoTop::processVolume(const Time& useTime, float useElevDegs, const std::string& useSubtype)
{
  // Traditional(useTime, useElevDegs, useSubtype);
  Interpolated(useTime, 0.0, useSubtype); // Seems MRMS uses 0 degrees?
  // VerticalColumnCoverage(useTime, useElevDegs, useSubtype);
}

void
EchoTop::VerticalColumnCoverage(const Time& useTime, float useElevDegs, const std::string& useSubtype)
{
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
  fLogInfo("{}", *myElevationVolume);

  VCC myCallback;

  myCallback.addVolume(*myElevationVolume);
  RadialSetIterator iter(*set);

  iter.iterateRadialGates(myCallback);

  // Write product
  std::map<std::string, std::string> myOverride;

  writeOutputProduct(set->getTypeName(), set, myOverride);
} // EchoTop::VerticalColumnCoverage

void
EchoTop::Traditional(const Time& useTime, float useElevDegs, const std::string& useSubtype)
{
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
  fLogInfo("{}", *myElevationVolume);

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
  fLogInfo("{}", *myElevationVolume);

  // -------------------------------------
  // Echotop value
  //
  InterpolatedET myCallback;

  myCallback.addVolume(*myElevationVolume);
  RadialSetIterator iter(*set);

  iter.iterateRadialGates(myCallback);
  // -------------------------------------
  // VCC value
  //
  // Some duplication.
  // We won't to be able to separate logically,
  // but some algs will belong together and we
  // don't want to duplicate.  So what's the best
  // design?

  // Metric ideas (brainstorm):
  // 1. VCC -- km coverage.  Normalize with some max value?
  //        --If there are two equal coverage areas, the one higher up
  //          in the sky is probably more reliable.
  //          Almost a 'max' problem vs averaging.
  // probably.  testing needed.
  // 3. Number of tilts.
  //     -- Numbers can be high near radar but cover a low area of vertical space,
  //        meaning that echotops from larger spreads over us are more accurate.
  //        Basically the cone of silence thing.
  //     -- Numbers can be high far away and cover a large area of vertical space
  //        due to the beam spread.
  // 2. 'Density' of tilts.  Tilts per km?
  // 4. Beamwidth top
  // 5. Range (already do)
  //
  //
  // Problems:
  //    A. Data too close to radar has cone of silence above.  The storm is usually
  //    not in the scan.  Need another radar's coverage to fill in.
  //       Range metric fails here IMO.
  //    B. Data too far the beam spread makes the values less reliable
  //       Range metric covers this somewhat.
  //
  VCC myCallback2;

  myCallback2.addVolume(*myElevationVolume);
  RadialSetIterator iter2(*set);

  // Will work witwh fusion to use these weights.
  set->addFloat2D("Weights", "dimensionless", { 0, 1 });
  iter2.setOutputArray("Weights");

  iter2.iterateRadialGates(myCallback2);
  // -------------------------------------

  fLogSevere("-------------------->>MAX {}", MAXKMS);
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
