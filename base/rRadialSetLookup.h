#pragma once

#include <rUtility.h>
#include <rRadialSet.h>

#include <vector>

namespace rapio
{
/** Bin lookup for O(1) lookup of azimuth/range to range/gate of a RadialSet.
 * Tried a few ways of doing this, this trades a bit of memory/setup for speed.
 * Lak's smart technique avoids doing OlogN sorting/searching of azimuths.
 *
 * @author Valliappa Lakshman
 * @author Robert Toomey */
class RadialSetLookup : public Utility
{
public:

  /** Will pre-compute the radial number within this radial set
   *  that corresponds to every azimuth and provide access to
   *  this information.
   *  @param myAccuracy How accurate do you want the conversion to be?
   *  Larger numbers denote greater myAccuracy. Use at least 5. This
   *  is the myAccuracy of the radial lookup.
   */
  RadialSetLookup(RadialSet&, int myAccuracy = 10);

  /*** Initialize an existing lookup to a given RadialSet. This can
   * be faster by reusing memory */
  void
  initToRadialSet(RadialSet&, bool blank = true);

  /** Returns the azimuth number in our lookup */
  inline int
  getAzimuthNumber(const double& azDeg) const
  {
    // find azimuth number
    const int fullCircle = 360 * myAccuracy;
    int azNo = int(azDeg * myAccuracy);

    if (azNo < 0) {
      azNo += fullCircle;
    } else if (azNo >= fullCircle) {
      azNo -= fullCircle;
    }
    return azNo;
  };

  /** Fills in the radial number and gatenumber that correspond
   *  to the azimuth and range passed in.
   *  The returned information is valid only for the last initialized
   *  radial set, either from constructor or initToRadialSet
   *
   *  @return true if the returned values are valid. The returned
   *  values will be invalid if the input az-ran is outside the
   *  range of the radial set.
   */
  inline bool
  getRadialGate(const double& azDegs, const double rnMeters,
    int * radialNo, int * gateNo) const
  {
    // Calculate Azimuth bin
    // Check azimuth bounds first to avoid range calculation...
    int azNo = getAzimuthNumber(azDegs);

    if (( azNo < 0) || ( azNo >= int(myAzToRadialNum.size())) ) {
      return false;
    }
    // Get Azimuth Index
    *radialNo = myAzToRadialNum[ azNo ];
    if (*radialNo < 0) {
      return false;
    }
    // Calculate Gate Index (last to avoid math if possible)
    *gateNo = int( (rnMeters - myDistToFirstGateM) / myGateWidthM);
    if (( *gateNo < 0) || ( *gateNo >= myNumGates) ) {
      return false;
    }
    return true;
  }

protected:
  /** The precision or number of bins per radial we use for lookup */
  int myAccuracy;

  /** Lookup from azimuth to index of radial for this azimuth */
  std::vector<int> myAzToRadialNum;

  // Range calculation variables

  /** The distance of a gate used for calculating range */
  double myGateWidthM;
  /** The distance to first gate of the RadialSet we're linked to */
  double myDistToFirstGateM;
  /** The number of gates assumed for calculating range */
  int myNumGates;
};
}
