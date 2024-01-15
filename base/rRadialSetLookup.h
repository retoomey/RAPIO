#pragma once

#include <rDataProjection.h>
#include <rUtility.h>
#include <rConstants.h>
#include <rArray.h>

#include <vector>
#include <memory>

namespace rapio
{
class RadialSet;

/** Bin lookup for O(1) lookup of azimuth/range to range/gate of a RadialSet.
 * Tried a few ways of doing this, this trades a bit of memory/setup for speed.
 * Lak's smart technique avoids doing OlogN sorting/searching of azimuths.
 *
 * @author Valliappa Lakshman
 * @author Robert Toomey */
class RadialSetProjection : public DataProjection
{
public:

  /**
   * Create a radial set projection
   * Will pre-compute the radial number within this radial set
   *  that corresponds to every azimuth and provide access to
   *  this information.
   *  @param rs RadialSet we are generating a look up for.
   *  @param ac How accurate do you want the conversion to be?
   *  Larger numbers denote greater accuracy. Use at least 5. This
   *  is the accuracy of the radial lookup.
   */
  RadialSetProjection(const std::string& layer, RadialSet * owner, int acc = 1000);

  /** Fills in the radial number and gatenumber that correspond
   *  to the azimuth and range passed in.
   *
   *  @return true if the returned values are valid. The returned
   *  values will be invalid if the input az-ran is outside the
   *  range of the radial set.
   *
   * */
  inline bool
  getValueAtAzRange(const double& azDegs, const double& rangeKMs, double& out, int& radialNo, int&  gateNo)
  {
    gateNo   = -1;
    radialNo = -1;
    const double rnMeters = rangeKMs * 1000.0;

    // If out of range can break early
    // The point would be in the cone of silence or outside scan
    if ((rnMeters < myDistToFirstGateM) || (rnMeters >= myDistToLastGateM)) {
      return false;
      // Say firstGate: 5 + (numGates: 10 * GateWidth:3)  = 35 last gate.
      // Pass in rnMeters = 35.  35-5 = 30.  30/3 = 10 which is > 0-9 index.
      // so should be >= in the f above
    }

    // Range 'should' be in the tilt, so check azimuth next
    // const int azNo = int(azDegs * myAccuracy) % (360 * myAccuracy);
    const int azNo = static_cast<int>(azDegs * myAccuracy) % myAccuracy360;

    if (( azNo < 0) || ( azNo >= int(myAzToRadialNum.size())) ) {
      return false;
    }

    // Calculate range and gate numbers finally
    // rnMeters must be < myDistToLastGateM here
    gateNo = static_cast<int>( (rnMeters - myDistToFirstGateM) / myGateWidthM);
    // if ( *gateNo >= myNumGates ) { Shouldn't be possible if LastGate is correct.
    //  return false;
    // }

    radialNo = myAzToRadialNum[ azNo ];
    const bool validRadial = (radialNo >= 0);

    out = validRadial ? (*my2DLayer)[radialNo][gateNo] : Constants::DataUnavailable;
    return validRadial;
  } // getRadialGate

  /** Get a value at a lat lon for a given layer name (SpaceTime datatype) */
  virtual double
  getValueAtLL(double latDegs, double lonDegs) override;

  /** Calculate Lat Lon coverage marching grid from spatial center */
  virtual bool
  LLCoverageCenterDegree(const float degreeOut, const size_t numRows, const size_t numCols,
    float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs) override;

  virtual bool
  LLCoverageFull(size_t& numRows, size_t& numCols,
    float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs) override;

protected:

  /*** Initialize to a RadialSet */
  void
  initToRadialSet(RadialSet&, bool blank = true);

  /** Cache current set 2D layer */
  ArrayFloat2DPtr my2DLayer;

  // -----------------------------------------------------
  // Lookup overlay variables

  /** The precision or number of bins per radial we use for lookup */
  int myAccuracy;

  /** Cached myAccuracy times 360 for lookup speed (360*myAccuracy) */
  int myAccuracy360;

  /** Lookup from azimuth to index of radial for this azimuth */
  std::vector<int> myAzToRadialNum;

  // -----------------------------------------------------
  // Range calculation variables

  /** The distance of a gate used for calculating range */
  double myGateWidthM;

  /** The distance to first gate of the RadialSet we're linked to */
  double myDistToFirstGateM;

  /** The distance at end of last gate myDistToFirstGateM+(myNumGates*myGateWidthM)*/
  double myDistToLastGateM;

  /** The number of gates assumed for calculating range */
  int myNumGates;

  // -----------------------------------------------------
  // Caching of radial set values for speed

  /** Cache center degrees */
  float myCenterLatDegs;

  /** Cache center degrees */
  float myCenterLonDegs;
};
}
