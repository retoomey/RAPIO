#pragma once

#include <rUtility.h>
#include <rDataType.h>

namespace rapio {
/** Organizer for in/out of a VolumeValueResolver.
 * These are available data values for a volume resolver to use to interpolate
 * or extrapolate data values involving tilts and volumes.
 *
 * @author Robert Toomey
 */
class VolumeValue : public Utility
{
public:
  // INPUTS ---------------------
  DataType * upper;  ///< DataType above, if any
  DataType * lower;  ///< DataType below, if any
  LengthKMs cHeight; ///< Radar location height in Kilometers (constant)

  // INPUTS for this value location
  LengthKMs layerHeightKMs;  ///< Height of the virtual layer we're calculating, the merger layer
  AngleDegs azDegs;          ///< Virtual Azimuth degrees at this location
  AngleDegs virtualElevDegs; ///< Virtual elevation degrees at this location
  double sinGcdIR;           ///< Cached sin GCD IR ratio
  double cosGcdIR;           ///< Cached cos GCD IR ratio

  // OUTPUTS ---------------------
  double dataValue; ///< Output calculated data value
  // FIXME: weight probably here too
};

/** Volume value resolver */
class VolumeValueResolver : public Utility
{
public:
  /** Calculate/interpret values between layers */
  virtual void calc(VolumeValue& vv){ vv.dataValue = Constants::DataUnavailable; }

  /** Calculate vertical height due to a degree shift (for inside beamwidth checking) */
  void
  heightForDegreeShift(VolumeValue& vv, DataType * set, AngleDegs delta, LengthKMs& heightKMs);

  /** Get value and height in a RadialSet at a given range */
  bool
  valueAndHeight(VolumeValue& vv, DataType * set, LengthKMs& atHeightKMs, float& out);
};
}
