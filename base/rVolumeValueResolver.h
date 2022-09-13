#pragma once

#include <rUtility.h>
#include <rDataType.h>

namespace rapio {
/** Organizer of outputs for a layer, typically upper tilt or lower tilt
 * around the location of interest.
 */
class LayerValue : public Utility
{
public:
  double value;         ///< Value of data at layer
  float terrainPercent; ///< Terrain blockage, if available otherwise 0
  LengthKMs heightKMs;  ///< Height in kilometers at the point of interest
  LengthKMs rangeKMs;   ///< Rangein kilometers along the beampath
  int gate;             ///< Gate number at the point of interest, or -1
  int radial;           ///< Radial number at the point of interest, or -1
};

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
  DataType * upper; ///< DataType above, if any
  DataType * lower; ///< DataType below, if any

  // Odd value out at moment, probably for calculation speed
  LengthKMs cHeight; ///< Radar location height in Kilometers (constant)

  // INPUTS for this value location
  // This is the location of interest between layers
  LengthKMs layerHeightKMs;  ///< Height of the virtual layer we're calculating, the merger layer
  AngleDegs azDegs;          ///< Virtual Azimuth degrees at this location
  AngleDegs virtualElevDegs; ///< Virtual elevation degrees at this location
  double sinGcdIR;           ///< Cached sin GCD IR ratio
  double cosGcdIR;           ///< Cached cos GCD IR ratio

  // OUTPUTS ---------------------
  double dataValue; ///< Final output calculated data value by resolver

  LayerValue uLayer; ///< Output from above
  LayerValue lLayer; ///< Output from below
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

  /** Query info on a RadialSet from cached location information given by a VolumeValue.
   * This basically 'fast' calculates from LLH to RadialSet information.
   * FIXME: group the outputs I think into another structure */
  bool
  queryLayer(VolumeValue& vv, DataType * set, LayerValue& l);
};
}