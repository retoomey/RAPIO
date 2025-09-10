#include <rPolarVMax.h>

#include <iostream>

using namespace rapio;

void
PolarVMax::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "PolarVMax create a polar maximum product.");
}

void
PolarVMax::processVolume(const Time& useTime, float useElevDegs, const std::string& useSubtype)
{
  // FIXME: Will be too much code duplication here currently,
  // we're still updating the API.  All this is for the max.  A min might
  // be only slightly different.  Either we avoid the iterator class or we
  // improve its api to allow various use cases

  /** New way using the callback iterator API.  This will allow us to hide
   * the 'normal' ways of iterating over DataTypes.  In particular,
   * the LatLonGrid is complicated. Typically one marches over lat/lon and
   * has to do angle calculations, etc.  We can hide all that from users. */
  class myRadialSetCallback : public RadialSetCallback {
public:

    /** Add a projection for a RadialSet.  We'll project into
     * each tilt to get the value there */
    void
    addProjection(RadialSetProjection * p)
    {
      projectors.push_back(p);
    }

    /** For each gate, we've gonna take the absolute max of the other gates
     * in the volume vertically */
    virtual void
    handleGate(RadialSetIterator * it)
    {
      // We want the current center azimuth/range of our RadialSet gate,
      // this will be used to project into the other RadialSets.
      const auto atAzDegs  = it->getCenterAzimuthDegs();
      const auto atRangeKM = it->getCenterRangeMeters() / 1000.0; // FIXME: units?

      float currentMaxAbs = 0.0;
      bool foundOne = false;
      bool missingMask = false;
      int radialNo, gateNo;
      double value;

      // For each RadialSet in the volume...
      for (size_t i = 0; i < projectors.size(); ++i) {
        // ...see if we hit it...
        if (projectors[i]->getValueAtAzRange(atAzDegs, atRangeKM, value, radialNo, gateNo)) {
          missingMask = true; // Because we 'hit' radar coverage

          // ...and if a good value, keep the maximum
          if (Constants::isGood(value)) {
            value = std::fabs(value);
            if (value >= currentMaxAbs) { currentMaxAbs = value; }
            foundOne = true;
          }
        }

        // Maybe a group method for this?  This is probably a common thing
        // it->setValueMask(foundOne, missingMask, value);

        if (foundOne) {
          it->setValue(currentMaxAbs);
          //  outData[r][g] = currentMaxAbs;
        } else {
          it->setValue(missingMask ? Constants::MissingData : Constants::DataUnavailable);
          // outData[r][g] = missingMask ? Constants::MissingData : Constants::DataUnavailable;
        }
      }
    } // handleGate

private:
    // Gather the radial set projections for speed
    std::vector<RadialSetProjection *> projectors;
  };

  // Do the work of processing the virtual volume.  For our 'start' we're gonna try
  // doing an absolute maximum.  This will expand to be plugins that we could also call
  // directly from fusion I think.

  // -------------------------------------------------------------
  // Find the largest gates/radials in the volume and clone that.
  // FIXME: We could also possibly specify the output gates/radials
  // as well. That might work better.
  //
  auto& tilts = myElevationVolume->getVolume();

  if (tilts.size() < 1) { return; }

  size_t maxGates   = 1;
  size_t maxRadials = 1;

  for (size_t i = 0; i < tilts.size(); ++i) {
    // It should be a radial set.
    if (auto r = std::dynamic_pointer_cast<rapio::RadialSet>(tilts[i])) {
      const size_t numGates   = r->getNumGates();
      const size_t numRadials = r->getNumRadials();
      if (numGates > maxGates) {
        maxGates = numGates;
      }
      if (numRadials > maxRadials) {
        maxRadials = numRadials;
      }
    }
  }
  // if (maxRadials > 360){ maxRadials = 360; }
  // if (maxRadials > 720){ maxRadials = 720; }

  // Create the output based on one of the inputs
  auto base = std::dynamic_pointer_cast<rapio::RadialSet>(tilts[0]);

  myMergedSet = RadialSet::Create(base->getTypeName() + "_2DMax",
      base->getUnits(),
      base->getLocation(),
      useTime, // Could see current time or tilt time
      useElevDegs,
      base->getDistanceToFirstGateM(),
      base->getGateWidthKMs() * 1000.0,
      maxRadials,
      maxGates);
  myMergedSet->setSubType(useSubtype); // Mark special
  // Debating forcing RadarName in Create, but make sure we
  // have one since many algs require this field.
  myMergedSet->setRadarName(base->getRadarName());

  // -------------------------------------------------------------
  // Iterate over the volume, projection from largest to the other
  // tilts
  //
  LogInfo(*myElevationVolume << "\n");

  // Do the algorithm.  Here we're going to do an absolute max as POC

  myRadialSetCallback myCallback;

  // Gather the radial set projections for speed
  for (size_t i = 0; i < tilts.size(); ++i) {
    auto pd = tilts[i]->getProjection().get();
    RadialSetProjection * p = static_cast<RadialSetProjection *>(pd);
    if (p != nullptr) {
      myCallback.addProjection(p);
    }
  }

  RadialSetIterator iter(*myMergedSet);

  iter.iterateRadialGates(myCallback);

  std::map<std::string, std::string> myOverride;

  writeOutputProduct(myMergedSet->getTypeName(), myMergedSet, myOverride); // Typename will be replaced by -O filters
} // PolarVMax::processVolume

int
main(int argc, char * argv[])
{
  // Create your algorithm instance and run it.  Isn't this API better?
  PolarVMax alg = PolarVMax();

  // Run this thing standalone.
  alg.executeFromArgs(argc, argv);
}
