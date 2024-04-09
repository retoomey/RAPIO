#pragma once

#include <rUtility.h>
#include <rDataType.h>
#include <rFactory.h>
#include <rRAPIOOptions.h>
#include <rRadialSet.h>
#include <rRadialSetLookup.h>
#include <rVolumeValue.h>

namespace rapio {
class RAPIOAlgorithm;

/** Class responsible for inputting/outputting the results of an entire grid of VolumeValue
 * calculations.  For averaging merger, this writes raw files which are read by stage2.
 * For velocity appending this might write a different raw or netcdf file. */
class VolumeValueIO : public Utility
{
public:
  /** Create IO class */
  VolumeValueIO(){ };

  /** Initialize a volume value IO for sending data.  Typically called by stage1 to
   * prepare to send the grid resolved values */
  virtual bool
  initForSend(
    const std::string   & radarName,
    const std::string   & typeName,
    const std::string   & units,
    const LLH           & center,
    size_t              xBase,
    size_t              yBase,
    std::vector<size_t> dims
  )
  {
    return false;
  }

  /** Add data to a grid cell for sending to stage2, used by stage one.
   * This is called for every grid cell. */
  virtual void
  add(VolumeValue * vvp, short x, short y, short z){ };

  /** Send/write stage2 data.  Give an algorithm pointer so we call do alg things if needed. */
  virtual void
  send(RAPIOAlgorithm * alg, Time aTime, const std::string& asName)
  {
    LogInfo("This resolver doesn't currently send stage2 data.\n");
  }
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
  virtual void calc(VolumeValue * vv){ vv->dataValue = Constants::DataUnavailable; }

  /** Calculate vertical height due to a degree shift (for inside beamwidth checking) */
  void
  heightForDegreeShift(VolumeValue& vv, DataType * set, AngleDegs delta, LengthKMs& heightKMs);

  /** Return the VolumeValue class used for this resolver.  This allows resolvers to
   * create/store as much output as they want.  It is used to pass in our database of inputs
   * to the resolver and then generate the various outputs */
  virtual std::shared_ptr<VolumeValue>
  getVolumeValue()
  {
    return std::make_shared<VolumeValue>();
  }

  /** Return the VolumeValueOutputter class used for this resolver.  This allows resolvers to
   * write/read final output as they want. */
  virtual std::shared_ptr<VolumeValueIO>
  getVolumeValueIO()
  {
    return std::make_shared<VolumeValueIO>();
  }

private:

  /** Query a single layer.  Inline the code since this is called a silly amount of times. */
  inline bool
  queryLayer(VolumeValue& vv, DataType * set, DataProjection * project, LayerValue& l)
  {
    l.clear();

    if (set == nullptr) { return false; }

    // Casts for moment since we currently only work with radialsets.  if eventually we
    // add in lat lon grids we'll need more work.  I'm trying to avoid virtual for speed.
    RadialSet& r = *((RadialSet *) set);
    RadialSetProjection& p = *((RadialSetProjection *) project);

    l.elevation = r.getElevationDegs();

    // Projection of height range using attentuation
    // Notice we cached sin and cos GCD for the grid

    Project::Cached_BeamPath_LLHtoAttenuationRange(vv.getRadarHeightKMs(),
      vv.sinGcdIR, vv.cosGcdIR, r.getElevationTan(), r.getElevationCos(), l.heightKMs, l.rangeKMs);

    const bool have = p.getValueAtAzRange(vv.virtualAzDegs, l.rangeKMs, l.value, l.radial, l.gate);

    // If good, query more stuff
    if (have) {
      // Data value at gate (getValueAtAzRange sets this)
      // l.value = r.getFloat2DRef()[l.radial][l.gate];

      // Beamwidth info
      l.beamWidth = r.getBeamWidthRef()[l.radial];

      // Terrain info
      const bool t = r.haveTerrain();
      l.haveTerrain       = t; // Checking t each time here results in one less jmp in assembly..crazy
      l.terrainCBBPercent = t ? r.getTerrainCBBPercentRef()[l.radial][l.gate] : 0.0;
      l.terrainPBBPercent = t ? r.getTerrainPBBPercentRef()[l.radial][l.gate] : 0.0;
      l.beamHitBottom     = t ? (r.getTerrainBeamBottomHitRef()[l.radial][l.gate] != 0) : 0.0;
    }

    return have;
  } // queryLayer

public:

  // Direct routines for querying a layer.  I was abstracting it to enum, but
  // it's not worth the slow down.  If we add tilts/layers we can just add methods

  /** Query lower tilt */
  inline bool
  queryLower(VolumeValue& vv)
  {
    return (queryLayer(vv, vv.getLower(), vv.getLowerP(), vv.getLowerValue()));
  }

  /** Query 2nd lower tilt */
  inline bool
  query2ndLower(VolumeValue& vv)
  {
    return (queryLayer(vv, vv.get2ndLower(), vv.get2ndLowerP(), vv.get2ndLowerValue()));
  }

  /** Query upper tilt */
  inline bool
  queryUpper(VolumeValue& vv)
  {
    return (queryLayer(vv, vv.getUpper(), vv.getUpperP(), vv.getUpperValue()));
  }

  /** Query 2nd upper tilt */
  inline bool
  query2ndUpper(VolumeValue& vv)
  {
    return (queryLayer(vv, vv.get2ndUpper(), vv.get2ndUpperP(), vv.get2ndUpperValue()));
  }

  /** Query multiple layers at once.  Maybe there's a future optimization here. */
  inline void
  queryLayers(VolumeValue& vv, bool& haveLower, bool& haveUpper, bool& haveLLower,
    bool& haveUUpper)
  {
    haveLower  = queryLower(vv);
    haveUpper  = queryUpper(vv);
    haveLLower = query2ndLower(vv);
    haveUUpper = query2ndUpper(vv);
  }

  /** Query multiple layers at once.  Maybe there's a future optimization here. */
  inline void
  queryLayers(VolumeValue& vv, bool& haveLower, bool& haveUpper)
  {
    haveLower = queryLower(vv);
    haveUpper = queryUpper(vv);
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
  calc(VolumeValue * vv) override;

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
  calc(VolumeValue * vv) override;

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
  calc(VolumeValue * vv) override;

  /** Help function, subclasses return help information. */
  virtual std::string
  getHelpString(const std::string& fkey) override { return "Test pattern: Output terrain CBB% scaled by 100."; }
};
}
