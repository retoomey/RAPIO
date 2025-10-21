#pragma once

#include <rUtility.h>

#include <rRadialSet.h>

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
 * @author Robert Toomey
 */
class LayerValue : public Utility
{
public:

  // Filled in/copied by query
  double value;        ///< Value of data at layer
  LengthKMs heightKMs; ///< Height in kilometers at the point of interest
  LengthKMs rangeKMs;  ///< Range in kilometers along the beampath
  int gate;            ///< Gate number at the point of interest, or -1
  int radial;          ///< Radial number at the point of interest, or -1
  bool haveTerrain;    ///< Do we have terrain information?

  DataTypePointerCache * c; ///< Cache to all the data arrays for this layer

  // Eventually we might handle LatLonGrid data, so these methods would
  // crash.  Up to higher up to validate the data.  Doing it here is a
  // massive slowdown.
  // FIXME: We could maybe subclass layer value but I wouldn't make it
  // a virtual class.

  // Not every resolver will want every field, so lazy grabbing here
  // will give us a slight speed boost by avoiding copying.
  // Static casts don't actually make any code.
  inline float
  getWeight()
  {
    auto * rc = static_cast<RadialSetPointerCache *>(c);

    return (rc->weights ? (*rc->weights)[radial][gate] : 1.0);
  }

  inline float
  getTerrainCBBPercent()
  {
    auto * rc = static_cast<RadialSetPointerCache *>(c);

    return (rc->cbb ? (*rc->cbb)[radial][gate] : 0.0);
  }

  inline float
  getTerrainPBBPercent()
  {
    auto * rc = static_cast<RadialSetPointerCache *>(c);

    return (rc->pbb ? (*rc->pbb)[radial][gate] : 0.0);
  }

  inline bool
  getTerrainBeamHitBottom()
  {
    auto * rc = static_cast<RadialSetPointerCache *>(c);

    return (rc->bottomHit ? (*rc->bottomHit)[radial][gate] : false);
  }

  /** RadialSets: Get beamwidth angle at the point of interest */
  inline AngleDegs
  getBeamWidthDegs()
  {
    auto * rc = static_cast<RadialSetPointerCache *>(c);
    auto * r  = static_cast<RadialSet *>(c->dt);

    return ((*rc->bw)[radial]);
  }

  /** RadialSets: Get elevation angle */
  inline AngleDegs
  getElevationDegs()
  {
    // For speed, no checking types here.  Should be higher
    return (static_cast<RadialSet *>(c->dt)->getElevationDegs());
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

  /** The layers we store, nearest 2 in each direction */
  enum class Layer {
    Lower,
    Upper,
    Lower2,
    Upper2,
  };
  static constexpr int LayerCount = 4;

  /** Ensure cleared out on construction..it should be */
  VolumeValue() : dataValue(0.0) // Default to 1.0 since final = v/w
  { }

  /** Get DataTypePointerCache for layer */
  inline DataTypePointerCache *
  getPC(Layer l)
  {
    return pc[static_cast<int>(l)];
  }

  /** Set DataTypePointerCache for layer */
  inline void
  setPC(Layer l, DataTypePointerCache * c)
  {
    pc[static_cast<int>(l)] = c;
  }

  /** Clear all the pointer caches */
  inline void
  clearPC()
  {
    pc = { };
  }

  /** Get the input layer/tilt directly below sample */
  inline DataType *
  getLower()
  {
    auto * c = getPC(Layer::Lower);

    if (c == nullptr) { // not liking the if.  Full cache faster?
      return nullptr;
    } else {
      return c->dt;
    }
  }

  /** Get the query results below sample */
  inline LayerValue& getLowerValue(){ return layerout[0]; }

  /** Get the input layer/tilt directly above sample */
  inline DataType *
  getUpper()
  {
    auto * c = getPC(Layer::Upper);

    if (c == nullptr) {
      return nullptr;
    } else {
      return c->dt;
    }
  }

  /** Get the query results above sample */
  inline LayerValue& getUpperValue(){ return layerout[1]; }

  /** Get the layer/tilt two levels below the sample */
  inline DataType *
  get2ndLower()
  {
    auto * c = getPC(Layer::Lower2);

    if (c == nullptr) {
      return nullptr;
    } else {
      return c->dt;
    }
  }

  /** Get the query results two levels below sample */
  inline LayerValue& get2ndLowerValue(){ return layerout[2]; }

  /** Get the layer/tilt two levels above the sample */
  inline DataType *
  get2ndUpper()
  {
    auto * c = getPC(Layer::Upper2);

    if (c == nullptr) {
      return nullptr;
    } else {
      return c->dt;
    }
  }

  /** Get the query results two levels above sample */
  inline LayerValue& get2ndUpperValue(){ return layerout[3]; }

  /** Get the DataTypePointerCache */
  inline DataTypePointerCache * getLowerPC(){ return getPC(Layer::Lower); }

  inline DataTypePointerCache * getUpperPC(){ return getPC(Layer::Upper); }

  inline DataTypePointerCache * get2ndLowerPC(){ return getPC(Layer::Lower2); }

  inline DataTypePointerCache * get2ndUpperPC(){ return getPC(Layer::Upper2); }

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

  // These are the inputs used for each layer...
  // We 'could' store them in the LayerValue.
  // Passed to queryLayer and then we fill in LayerValue.
  // I think the reason we didn't is because LayerValue is called
  // each time per grid point and we'd have to copy all those pointers
  // for every LayerValue output.
  // We could have a LayerInputValue or something though.
  std::array<DataTypePointerCache *, LayerCount> pc{ }; // < Four layers, 2 above, 2 below
protected:

  LLH radarLocation;   ///< Radar location (constant for a grid)
  LLH virtualLocation; ///< Location in grid of the virtual layer/CAPPI we're calculating

public:

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

  LayerValue layerout[LayerCount]; ///< Output for layers, queryLayer fills

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
