#pragma once

#include <rUtility.h>

namespace rapio {
class RAPIOAlgorithm;

/** Organizer of outputs for a layer, typically upper tilt or lower tilt
 * around the location of interest.
 * These are optional data that is created/stored in a VolumeValue
 * depending upon demand.  You can think of it as a database for a particular
 * tilt that is related to the grid point being sampled.
 * There's cost/speed associated with calculating the math for a tilt or
 * layer.
 *
 * FIXME: finish get/set stuff to make the API cleaner here.
 *
 * @author Robert Toomey
 */
class LayerValue : public Utility
{
public:
  double value;            ///< Value of data at layer
  bool haveTerrain;        ///< Do we have terrain information?
  float terrainCBBPercent; ///< Terrain cumulative beam blockage, if available otherwise 0
  float terrainPBBPercent; ///< Terrain partial beam blockage, if available otherwise 0
  bool beamHitBottom;      ///< Terrain blockage, did beam bottom hit terrain?
  LengthKMs heightKMs;     ///< Height in kilometers at the point of interest
  LengthKMs rangeKMs;      ///< Range in kilometers along the beampath
  int gate;                ///< Gate number at the point of interest, or -1
  int radial;              ///< Radial number at the point of interest, or -1
  AngleDegs elevation;     ///< 'True' elevation angle used for this layer value
  AngleDegs beamWidth;     ///< Beamwidth at the point of interest

  /** Clear all the values to default unset state */
  void
  clear()
  {
    value             = Constants::DataUnavailable;
    haveTerrain       = false;
    terrainCBBPercent = 0;
    terrainPBBPercent = 0;
    beamHitBottom     = false;
    heightKMs         = 0;
    rangeKMs          = 0;
    gate      = -1;
    radial    = -1;
    elevation = Constants::MissingData;
    beamWidth = 1;
  }
};

/** Organizer for in/out of a VolumeValueResolver for resolving the output of
 * a single grid cell.
 *
 * These are available data values for a volume resolver to use to interpolate
 * or extrapolate data values involving tilts and volumes.
 *
 * @author Robert Toomey
 */
class VolumeValue : public Utility
{
public:

  /** Ensure cleared out on construction..it should be */
  VolumeValue() : dataValue(0.0) // Default to 1.0 since final = v/w
  {
    layer[0]   = layer[1] = layer[2] = layer[3] = nullptr;
    project[0] = project[1] = project[2] = project[3] = nullptr;
    cbb[0]     = cbb[1] = cbb[2] = cbb[3] = nullptr;
    pbb[0]     = pbb[1] = pbb[2] = pbb[3] = nullptr;
    bottom[0]  = bottom[1] = bottom[2] = bottom[3] = nullptr;
  }

  // Layers/tilts we handle around the sample location
  // FIXME: Feels messy. Maybe enum class and avoid the 'special methods'

  /** Get the input layer/tilt directly below sample */
  inline DataType *& getLower(){ return layer[0]; }

  /** Get the input projection for layer/tilt below sample */
  inline DataProjection *& getLowerP(){ return project[0]; }

  /** Get the query results below sample */
  inline LayerValue& getLowerValue(){ return layerout[0]; }

  /** Get the input layer/tilt directly above sample */
  inline DataType *& getUpper(){ return layer[1]; }

  /** Get the input projection for layer/tilt above sample */
  inline DataProjection *& getUpperP(){ return project[1]; }

  /** Get the query results above sample */
  inline LayerValue& getUpperValue(){ return layerout[1]; }

  /** Get the layer/tilt two levels below the sample */
  inline DataType *& get2ndLower(){ return layer[2]; }

  /** Get the input projection for two layers below sample */
  inline DataProjection *& get2ndLowerP(){ return project[2]; }

  /** Get the query results two levels below sample */
  inline LayerValue& get2ndLowerValue(){ return layerout[2]; }

  /** Get the layer/tilt two levels above the sample */
  inline DataType *& get2ndUpper(){ return layer[3]; }

  /** Get the input projection for two layers above sample */
  inline DataProjection *& get2ndUpperP(){ return project[3]; }

  /** Get the query results two levels above sample */
  inline LayerValue& get2ndUpperValue(){ return layerout[3]; }

  /** Get the Terrain lookup arrays */
  inline ArrayFloat2DPtr getLowerCBB(){ return cbb[0]; }

  inline ArrayFloat2DPtr getUpperCBB(){ return cbb[1]; }

  inline ArrayFloat2DPtr get2ndLowerCBB(){ return cbb[2]; }

  inline ArrayFloat2DPtr get2ndUpperCBB(){ return cbb[3]; }

  inline ArrayFloat2DPtr getLowerPBB(){ return pbb[0]; }

  inline ArrayFloat2DPtr getUpperPBB(){ return pbb[1]; }

  inline ArrayFloat2DPtr get2ndLowerPBB(){ return pbb[2]; }

  inline ArrayFloat2DPtr get2ndUpperPBB(){ return pbb[3]; }

  inline ArrayByte2DPtr getLowerBottomHit(){ return bottom[0]; }

  inline ArrayByte2DPtr getUpperBottomHit(){ return bottom[1]; }

  inline ArrayByte2DPtr get2ndLowerBottomHit(){ return bottom[2]; }

  inline ArrayByte2DPtr get2ndUpperBottomHit(){ return bottom[3]; }

  // ----------------------------------------------
  // Radar or center location of data
  //

  /** Get radar location */
  inline LLH getRadarLocation(){ return radarLocation; }

  /** Set radar location */
  inline void setRadarLocation(LLH& l){ radarLocation = l; }

  /** Get radar height */
  inline float getRadarHeightKMs(){ return radarLocation.getHeightKM(); }

  /** Get radar latitude degrees */
  inline float getRadarLatitudeDegs(){ return radarLocation.getLatitudeDeg(); }

  /** Get radar longitude degrees */
  inline float getRadarLongitudeDegs(){ return radarLocation.getLongitudeDeg(); }

  // ----------------------------------------------
  // Current grid cell location or at of the data
  //

  /** Get access grid location */
  inline LLH getAtLocation(){ return virtualLocation; }

  /** Set access grid location */
  inline void setAtLocation(LLH& l){ virtualLocation = l; }

  /** Get height in kilometers at grid location */
  inline float getAtLocationHeightKMs(){ return virtualLocation.getHeightKM(); }

  /** Set height in kilometers at grid location */
  inline void setAtLocationHeightKMs(float h){ virtualLocation.setHeightKM(h); }

  /** Get location height in virtual grid */
  inline float getAtHeightKMs(){ return virtualLocation.getHeightKM(); }

  /** Set Latitude and Longitude degrees at grid location */
  inline void
  setAtLocationLatLonDegs(double lat, double lon)
  {
    virtualLocation.setLatitudeDeg(lat);
    virtualLocation.setLongitudeDeg(lon);
  }

  /** Get location Latitude in virtual grid */
  inline float getAtLatitudeDegs(){ return virtualLocation.getLatitudeDeg(); }

  /** Get location Longitude in virtual grid */
  inline float getAtLongitudeDegs(){ return virtualLocation.getLongitudeDeg(); }

  // -----------------------------------------------
  // INPUTS for this grid cell location
  //
protected:

  DataType * layer[4];         ///< Currently four layers, 2 above, 2 below, getSpread fills
  DataProjection * project[4]; ///< One projection per layer
  LayerValue layerout[4];      ///< Output for layers, queryLayer fills

  LLH radarLocation;   ///< Radar location (constant for a grid)
  LLH virtualLocation; ///< Location in grid of the virtual layer/CAPPI we're calculating

public:
  ArrayFloat2DPtr cbb[4];   ///< One Terrain CBB array per layer
  ArrayFloat2DPtr pbb[4];   ///< One Terrain PBB array per layer
  ArrayByte2DPtr bottom[4]; ///< One Terrain bottom hit array per layer

  AngleDegs virtualAzDegs;   ///< Virtual Azimuth degrees at this location
  AngleDegs virtualElevDegs; ///< Virtual elevation degrees at this location
  LengthKMs virtualRangeKMs; ///< Virtual range in kilometers at this location
  double sinGcdIR;           ///< Cached sin GCD IR ratio
  double cosGcdIR;           ///< Cached cos GCD IR ratio

  // -----------------------------------------------
  // OUTPUTS for this grid cell location
  //
  // We have a single output, which is used for direct grid output for stage1. This is typically
  // not what is passed to stage2, which can be more complicated due to merging or appending.
  // That usually requires more fields, which are added by subclasses of VolumeValue.
  // Basically, if we make stage1 output CAPPI layers directly, we need a value to put in
  // the 2D/3D output.  This is it.

  double dataValue; ///< Final output calculated data value by resolve
  // Subclasses can add addition output fields
  // double dataWeight1; ///< Final output weight1 by resolver (1 by default for averaging)
};

/** A Volume value used for the standard merger using weighted averaging
 * of values.  We add a top and bottom sum that is passed onto stage2 to be weight averaged.
 * Basically the trick here is that we can do a global weighted average sum. */
class VolumeValueWeightAverage : public VolumeValue
{
public:
  float topSum;    ///< Top sum of values passed to stage2
  float bottomSum; ///< Bottom sum of weights passed to stage2
};
}
