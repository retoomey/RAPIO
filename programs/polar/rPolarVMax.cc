#include <rPolarVMax.h>

#include <iostream>

using namespace rapio;

void
PolarVMax::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "PolarVMax creates a polar maximum product.");
}

void
PolarVMax::processVolume(const Time& useTime, float useElevDegs, const std::string& useSubtype)
{
  /** Callback to do a vertical max in polar */
  class VerticalMax : public ElevationVolumeCallback {
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

      float currentMaxAbs = 0.0;
      bool foundOne = false;
      bool missingMask = false;
      int radialNo, gateNo;
      double value;
      auto& pc = getPointerCache();

      // Go through the vertical column
      for (size_t i = 0; i < pc.size(); ++i) {
        // Static casts are cheap.  Add volume should have prechecked it all
        RadialSetPointerCache * p  = static_cast<RadialSetPointerCache *>(pc[i]);
        RadialSetProjection * proj = static_cast<RadialSetProjection *>(p->project);

        // ...see if we hit it...
        if (proj->getValueAtAzRange(atAzDegs, atRangeKM, value, radialNo, gateNo)) {
          missingMask = true; // Because we 'hit' radar coverage

          // ...and if a good value, keep the maximum
          if (Constants::isGood(value)) {
            value = std::fabs(value);

            if (!foundOne) {
              currentMaxAbs = value;
              foundOne      = true;
            } else {
              if (value >= currentMaxAbs) { currentMaxAbs = value; }
            }
          }
        }
      }

      if (foundOne) {
        it->setValue(currentMaxAbs);
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
    base->getTypeName() + "_2DMax",
    useSubtype);

  if (set == nullptr) { return; }
  // set->setDataAttributeValue("ColorMap", "Max");
  LogInfo(*myElevationVolume << "\n");

  VerticalMax myCallback;

  myCallback.addVolume(*myElevationVolume);
  RadialSetIterator iter(*set);

  iter.iterateRadialGates(myCallback);

  // Write product
  std::map<std::string, std::string> myOverride;

  writeOutputProduct(set->getTypeName(), set, myOverride);
} // PolarVMax::processVolume

int
main(int argc, char * argv[])
{
  // Create your algorithm instance and run it.  Isn't this API better?
  PolarVMax alg = PolarVMax();

  // Run this thing standalone.
  alg.executeFromArgs(argc, argv);
}
