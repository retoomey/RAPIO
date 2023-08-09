#pragma once

#include <rUtility.h>
#include <rDataType.h>
#include <rFactory.h>
#include <rRAPIOOptions.h>

namespace rapio {
/** Organizer of outputs for a layer, typically upper tilt or lower tilt
 * around the location of interest.
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

/** Organizer for in/out of a VolumeValueResolver.
 * These are available data values for a volume resolver to use to interpolate
 * or extrapolate data values involving tilts and volumes.
 *
 * @author Robert Toomey
 */
class VolumeValue : public Utility
{
protected:

  DataType * layer[4];    ///< Currently four layers, 2 above, 2 below, getSpread fills
  LayerValue layerout[4]; ///< Output for layers, queryLayer fills

public:

  /** Ensure cleared out on construction..it should be */
  VolumeValue()
  {
    layer[0] = layer[1] = layer[2] = layer[3] = nullptr;
  }

  // We handle up to two tilts around the sample.  We could
  // reimplement a N system but it would probably be slow if you
  // want to resolve using every layer or tilt.  Could be a future
  // experiment though.  In general, once you get a couple layers/tilts
  // away your data has very little effect.  We're assuming locality
  // of data here.

  /** Get the layer/tilt directly below sample, if any */
  inline DataType *& getLower(){ return layer[0]; } // lower;}

  /** Get the layer/tilt query below sample, if any */
  inline LayerValue& getLowerValue(){ return layerout[0]; } // lower;}

  /** Get the layer/tilt directly above sample, if any */
  inline DataType *& getUpper(){ return layer[1]; } // upper; }

  inline LayerValue& getUpperValue(){ return layerout[1]; } // lower;}

  /** Get the layer/tilt directly below the lower, if any */
  inline DataType *& get2ndLower(){ return layer[2]; } // nextLower; }

  inline LayerValue& get2ndLowerValue(){ return layerout[2]; } // lower;}

  /** Get the layer/tilt directly above the upper, if any */
  inline DataType *& get2ndUpper(){ return layer[3]; } // nextUpper; }

  inline LayerValue& get2ndUpperValue(){ return layerout[3]; } // lower;}

  // Odd value out at moment, probably for calculation speed
  LengthKMs cHeight; ///< Radar location height in Kilometers (constant)

  // INPUTS for this value location
  // This is the location of interest between layers
  LengthKMs layerHeightKMs;  ///< Height of the virtual layer we're calculating, the merger layer
  AngleDegs virtualAzDegs;   ///< Virtual Azimuth degrees at this location
  AngleDegs virtualElevDegs; ///< Virtual elevation degrees at this location
  LengthKMs virtualRangeKMs; ///< Virtual range in kilometers at this location
  double sinGcdIR;           ///< Cached sin GCD IR ratio
  double cosGcdIR;           ///< Cached cos GCD IR ratio

  // OUTPUTS ---------------------
  double dataValue;   ///< Final output calculated data value by resolver
  double dataWeight1; ///< Final output weight1 by resolver
};


/** Volume value resolver */
class VolumeValueResolver : public Utility
{
public:
  /** Enum for layers in queryLayer */
  enum Layer {
    lower, upper, lower2, upper2
  };

  /** Use this to introduce default built-in RAPIO VolumeValueResolver subclasses.
   * Note: You don't have to use this ability, it's not called by default algorithm.
   * To use it, call VolumeValueResolver::introduceSelf()
   * To override or add another, call VolumeValueResolver::introduce(key, valueresolver)
   */
  static void
  introduceSelf();

  /** Introduce a new VolumeValueResolver by name */
  static void
  introduce(const std::string            & key,
    std::shared_ptr<VolumeValueResolver> factory);

  /** Introduce dynamic help */
  static std::string
  introduceHelp();

  /** Introduce suboptions */
  static void
  introduceSuboptions(const std::string& name, RAPIOOptions& o);

  /** Help function, subclasses return help information. */
  virtual std::string
  getHelpString(const std::string& fkey){ return ""; }

  /** Attempt to create VolumeValueResolver by name.  This looks up any registered
   * classes and attempts to call the virtual create method on it. */
  static std::shared_ptr<VolumeValueResolver>
  createVolumeValueResolver(
    const std::string & key,
    const std::string & params);

  /** Attempt to create VolumeValueResolver command param */
  static std::shared_ptr<VolumeValueResolver>
  createFromCommandLineOption(
    const std::string & option);

  /** Called on subclasses by the VolumeVolumeResolver to create/setup the Resolver.
   * To use by name, you would override this method and return a new instance of your
   * Volume class. */
  virtual std::shared_ptr<VolumeValueResolver>
  create(const std::string & params)
  {
    return nullptr;
  }

  /** Calculate/interpret values between layers */
  virtual void calc(VolumeValue& vv){ vv.dataValue = Constants::DataUnavailable; }

  /** Calculate vertical height due to a degree shift (for inside beamwidth checking) */
  void
  heightForDegreeShift(VolumeValue& vv, DataType * set, AngleDegs delta, LengthKMs& heightKMs);

  /** Query info on a RadialSet from cached location information given by a VolumeValue.
   * This basically 'fast' calculates from LLH to RadialSet information.
   * FIXME: group the outputs I think into another structure */
private:
  bool
  queryLayer(VolumeValue& vv, DataType * set, LayerValue& l);
public:

  /** Query a lot of layers a bit more efficiently.  This gets all four effective tilts
   * for advanced calculation. */
  void
  queryLayers(VolumeValue& vv, bool& haveLower, bool& haveUpper, bool& haveLLower,
    bool& haveUUpper);

  /** Fill in data query layer for a tilt/layer above/below.  This isn't done
   * automatically since it takes time and some resolvers may not need this information.
   * This is convenient but slow for a lot of layers. */
  inline bool
  queryLayer(VolumeValue& vv, const Layer& l)
  {
    switch (l) { // Just use the index into arrays instead of hard values
        case Layer::lower: return (queryLayer(vv, vv.getLower(), vv.getLowerValue()));

        case Layer::upper: return (queryLayer(vv, vv.getUpper(), vv.getUpperValue()));

        case Layer::lower2: return (queryLayer(vv, vv.get2ndLower(), vv.get2ndLowerValue()));

        case Layer::upper2: return (queryLayer(vv, vv.get2ndUpper(), vv.get2ndUpperValue()));
    }
    return false;
  }
};

/** Virtual range in KMs */
class RangeVVResolver : public VolumeValueResolver
{
public:

  /** Introduce into VolumeValueResolver factory */
  static void
  introduceSelf()
  {
    std::shared_ptr<RangeVVResolver> newOne = std::make_shared<RangeVVResolver>();
    Factory<VolumeValueResolver>::introduce("range", newOne);
  }

  /** Create by factory */
  virtual std::shared_ptr<VolumeValueResolver>
  create(const std::string & params) override
  {
    return std::make_shared<RangeVVResolver>();
  }

  virtual void
  calc(VolumeValue& vv) override;

  /** Help function, subclasses return help information. */
  virtual std::string
  getHelpString(const std::string& fkey) override { return "Test pattern: Output range KMs from radar center."; }
};

/** Azimuth from 0 to 360 or so of virtual */
class AzimuthVVResolver : public VolumeValueResolver
{
public:

  /** Introduce into VolumeValueResolver factory */
  static void
  introduceSelf()
  {
    std::shared_ptr<AzimuthVVResolver> newOne = std::make_shared<AzimuthVVResolver>();
    Factory<VolumeValueResolver>::introduce("azimuth", newOne);
  }

  /** Create by factory */
  virtual std::shared_ptr<VolumeValueResolver>
  create(const std::string & params) override
  {
    return std::make_shared<AzimuthVVResolver>();
  }

  virtual void
  calc(VolumeValue& vv) override;

  /** Help function, subclasses return help information. */
  virtual std::string
  getHelpString(const std::string& fkey) override { return "Test pattern: Output virtual azimuth values for cells."; }
};

/** Projected terrain blockage of lower tilt.
 * Experiment to display some of the terrain fields for debugging */
class TerrainVVResolver : public VolumeValueResolver
{
public:
  /** Introduce into VolumeValueResolver factory */
  static void
  introduceSelf()
  {
    std::shared_ptr<TerrainVVResolver> newOne = std::make_shared<TerrainVVResolver>();
    Factory<VolumeValueResolver>::introduce("terrain", newOne);
  }

  /** Create by factory */
  virtual std::shared_ptr<VolumeValueResolver>
  create(const std::string & params) override
  {
    return std::make_shared<TerrainVVResolver>();
  }

  virtual void
  calc(VolumeValue& vv) override;

  /** Help function, subclasses return help information. */
  virtual std::string
  getHelpString(const std::string& fkey) override { return "Test pattern: Output terrain CBB% scaled by 100."; }
};
}
