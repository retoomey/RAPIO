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

  /** Attempt to quickly project from azimuth/range to radial/gate */
  inline bool
  AzRangeToRadialGate(const double& azDegs, const double& rangeKMs, int& radialNo, int& gateNo)
  {
    const double rnMeters = rangeKMs * 1000.0; // FIXME: if we stored a KM variable could save math

    // ---------------------------------------
    // Range check. The point would be in the cone of silence or outside scan
    if ((rnMeters < myDistToFirstGateM) || (rnMeters >= myDistToLastGateM)) {
      return false;
    }

    // ---------------------------------------
    // Azimuth check
    const int azNo = static_cast<int>(azDegs * myAccuracy) % myAccuracy360;

    if (( azNo < 0) || ( azNo >= int(myAzToRadialNum.size())) ) {
      return false;
    }
    radialNo = myAzToRadialNum[ azNo ];
    if (radialNo < 0) {
      return false;
    }

    const double r = (rnMeters - myDistToFirstGateM) / myGateWidthM;

    gateNo = static_cast<int>(r);
    if ((gateNo < 0) || (gateNo >= myNumGates) ) {
      return false;
    }

    return true;
  }

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
    radialNo = myAzToRadialNum[ azNo ];
    const bool validRadial = (radialNo >= 0);

    // Calculate range and gate numbers finally
    // rnMeters must be < myDistToLastGateM here
    if (myGateWidthM > 0) { // which 'should' always be true so what's up here.
      gateNo = static_cast<int>( (rnMeters - myDistToFirstGateM) / myGateWidthM);
    } else {
      return false;
    }
    if ((gateNo < 0) || (gateNo >= myNumGates) ) {
      return false;
    }

    out = validRadial ? (*my2DLayer)[radialNo][gateNo] : Constants::DataUnavailable;
    return validRadial;
  } // getRadialGate

  /** Experimental linear gate value fetcher (This dampens even with just 3 gates) */
  inline bool
  getValueAtAzRangeL(const double& azDegs, const double& rangeKMs, double& out, int& radialNo, int&  gateNo)
  {
    gateNo   = -1;
    radialNo = -1;
    const double rnMeters = rangeKMs * 1000.0;

    // ---------------------------------------
    // Range check. The point would be in the cone of silence or outside scan
    if ((rnMeters < myDistToFirstGateM) || (rnMeters >= myDistToLastGateM)) {
      return false;
    }

    // ---------------------------------------
    // Azimuth check
    const int azNo = static_cast<int>(azDegs * myAccuracy) % myAccuracy360;

    if (( azNo < 0) || ( azNo >= int(myAzToRadialNum.size())) ) {
      return false;
    }
    radialNo = myAzToRadialNum[ azNo ];
    if (radialNo < 0) {
      radialNo = -1;
      return false;
    }

    // ---------------------------------------
    // Percentage of gate coverage and gate number
    const double r = (rnMeters - myDistToFirstGateM) / myGateWidthM;

    gateNo = static_cast<int>(r); // Rounded floor example 15.4 --> 15

    // ---------------------------------------
    // Three gate linear interpolation idea.  May be too slow
    out = (*my2DLayer)[radialNo][gateNo];
    if (!Constants::isGood(out)) { // Can't linear with others if not a 'true' value
      return true;
    }

    const double p = r - gateNo; // Left over example 15.4-15 = .4

    // Lower/upper values if any...
    const double L = (gateNo > 0) ? (*my2DLayer)[radialNo][gateNo - 1] : Constants::DataUnavailable;
    const double U = (gateNo < myNumGates) ? (*my2DLayer)[radialNo][gateNo + 1] : Constants::DataUnavailable;

    // Original out is center in formula, for linear gate interpolation
    // ---------------------
    // |  L  |  out  |  U  |
    // ---------------------
    // p =   0  .5   1
    // ==> L +(p+.5)(out-L)   ; // out-L is distance
    // ==> out + (p-.5)(U-out); // U-out is distance
    (p <= .50) ? (Constants::isGood(L) ? out = L + (p + .5) * (out - L) : out) :
    (Constants::isGood(U) ? out = out + (p - .5) * (U - out) : out);

    return true;
  } // getValueAtAzRangeL

  /** Get a value at a lat lon for a given layer name (SpaceTime datatype) */
  virtual double
  getValueAtLL(double latDegs, double lonDegs) override;

  /** Calculate Lat Lon coverage marching grid from spatial center */
  virtual bool
  LLCoverageCenterDegree(const float degreeOut, const size_t numRows, const size_t numCols,
    float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs) override;

  /** Calculate a 'full' coverage based on the data type */
  virtual bool
  LLCoverageFull(size_t& numRows, size_t& numCols,
    float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs) override;

  /** Can be quicker to get gates from the projection */
  inline int getNumGates(){ return myNumGates; }

  /** Can be quicker to get radial from the projection */
  inline int getNumRadials(){ return myNumRadials; }

protected:

  /*** Initialize to a RadialSet */
  void
  initToRadialSet(RadialSet&);

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

  /** The number of radials */
  int myNumRadials;

  // -----------------------------------------------------
  // Caching of radial set values for speed

  /** Cache center degrees */
  float myCenterLatDegs;

  /** Cache center degrees */
  float myCenterLonDegs;
};
}
