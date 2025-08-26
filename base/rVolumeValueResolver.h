#pragma once

#include <rUtility.h>
#include <rDataType.h>
#include <rFactory.h>
#include <rRAPIOOptions.h>
#include <rRadialSet.h>
#include <rRadialSetProjection.h>
#include <rVolumeValue.h>
#include <rLLCoverageArea.h>
#include <rPartitionInfo.h>
#include <rArrayAlgorithm.h>

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
    const std::string    & radarName,
    const std::string    & typeName,
    const std::string    & units,
    const LLH            & center,
    const bool           noMissingSet, // FIXME: Thinking general param passing
    const PartitionInfo  & partition,
    const LLCoverageArea &radarGrid
  )
  {
    return false;
  }

  /** Add data to a grid cell for sending to stage2, used by stage one.
   * This is called for every grid cell. */
  virtual void
  add(VolumeValue * vvp, short x, short y, short z, size_t partIndex){ };

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

  /** Create a volume value resolver for calculating grid values */
  VolumeValueResolver() : myGlobalWeight(1.0), myVarianceWeight(1.0 / 62500.0), myMissing(Constants::MissingData),
    myUnavailable(Constants::DataUnavailable){ }

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

  /** Generate the weight from range for a resolver.  Magic number comes
   * from original w2merger.  Variance affects the speed of drop off as
   * you increase in range from radar center */
  static inline double
  rangeToWeight(LengthKMs r, double varianceU1)
  {
    // Inverse square (similiar to exponential decay for default values)
    // return (1.0 / (r*r));
    // const double V=50;  W2merger parameter
    // const double TimeVariance = 5*5;
    // const double DistanceVariance = V*V;
    // const double variance = DistanceVariance * TimeVariance;
    // Division is slower than multiplication, so we use 1/variance here
    // FIXME: We could experiment using Schraudolph fast exp here.
    return ::expf(-(r * r) * varianceU1);
  }

  /** Set a global weight multiplier, usually from stage 1 */
  void setGlobalWeight(float w){ myGlobalWeight = w; }

  /** Set a global variance weight multiplier, usually from stage 1 */
  void setVarianceWeight(float v){ myVarianceWeight = v; }

  /** Set a missing output value, usually in stage 1.  This allows
   * basic mask generation overriding. */
  void setMissingValue(const SentinelDouble& i){ myMissing = i; }

  /** Set an unavailable output value, usually in stage 1.  This allows
   * basic mask generation overriding. */
  void setUnavailableValue(const SentinelDouble& i){ myUnavailable = i; }

protected:

  /** The global weight multiplier we can use during calculations */
  float myGlobalWeight;

  /** The global sigma weight multiplier we can use during calculations */
  float myVarianceWeight;

  /** The missing value used by the resolver */
  SentinelDouble myMissing;

  /** The unavailable value used by the resolver */
  SentinelDouble myUnavailable;

  // FIXME: Thinking we could do function pointers or something for different methods
  // of querying the data.

  /** Query a single layer.  Inline the code since this is called a silly amount of times. */
  inline bool
  queryMatrixLayer(VolumeValue& vv, LayerValue& l, DataTypePointerCache * c, ArrayAlgorithm * m)
  {
    // Verified. Tilts can be null high above or below us
    if (c == nullptr) {
      return false;
    }

    // Assume RadialSet cache (FIXME: Generalize for all DataTypes)
    // Using this pointer cache increases speed massively, like 7x or more
    auto * rc = static_cast<RadialSetPointerCache *>(c);
    auto * r  = static_cast<RadialSet *>(c->dt);
    auto * p  = (RadialSetProjection *) (rc->project);

    // Projection of height range using attentuation
    // Notice we cached sin and cos GCD for the grid

    Project::Cached_BeamPath_LLHtoAttenuationRange(vv.getRadarHeightKMs(),
      vv.sinGcdIR, vv.cosGcdIR, r->getElevationTan(), r->getElevationCos(), l.heightKMs, l.rangeKMs);

    // If good, query more stuff
    if (p->AzRangeToRadialGate(vv.virtualAzDegs, l.rangeKMs, l.radial, l.gate)) {
      // Call the remapper with the current data.
      m->setSource(r->getFloat2D());
      float value;
      if (m->remap(l.radial, l.gate, value)) {
        l.value = value;
      } else {
        l.value = Constants::MissingData;
      }

      #if 0
      // Old way doing it direct to average.  I want to use the
      // ArrayAlgorithm class since this will allow plugin/changing it
      //
      // Data value at gate (getValueAtAzRange sets this)
      auto& d = r.getFloat2DRef();


      //
      // Complete alpha code...
      //
      // Matrix average/cressmen around the radial/gate.
      // Start with averaging good values.  Uggh.  Also assuming a
      // full RadialSet.
      const int mgate   = p.getNumGates();
      const int mradial = p.getNumRadials();
      constexpr const size_t TOTALRADIALS  = 8;                // 8 spread left/right azimuth
      constexpr const size_t TOTALRADIALS2 = TOTALRADIALS / 2; // 4 half spread
      constexpr const size_t TOTALGATES    = 12;               // 12
      constexpr const size_t TOTALGATES2   = TOTALGATES / 2;   // 4 half spread

      double totalValue = 0;
      size_t totalCount = 0;

      // Init of a start radial
      int atradial = l.radial - TOTALRADIALS2;
      if (atradial < 0) { atradial += mradial; } // roll azimuth

      int startgate = l.gate - TOTALGATES2;
      if (startgate < 0) { startgate = 0; } // clip gates
      int endgate = l.gate + TOTALGATES2;
      if (endgate >= mgate) { endgate = mgate - 1; }

      // Loop over radial range with wrap around
      for (size_t rad = 0; rad < TOTALRADIALS; ++rad) {
        // Loop over clipped gate range
        for (size_t gate = startgate; gate <= endgate; ++gate) {
          auto& v = d[atradial][gate];
          if (Constants::isGood(v)) {
            totalValue += v;
            totalCount++;
          }
        }

        if (++atradial >= mradial) { atradial = 0; } // roll
      }

      // Final average of good values...
      if (totalCount > 0) {
        l.value = totalValue / totalCount;
      } else {
        l.value = Constants::MissingData;
      }
      #endif // if 0

      // Meta data of the 'point' is from the hit gate.

      // Beamwidth/elevation info
      l.beamWidth = (*rc->bw)[l.radial];
      l.elevation = r->getElevationDegs();

      // Terrain info
      // FIXME:  Use the pointer cache to speed this up as well.
      const bool t = r->haveTerrain();
      l.haveTerrain       = t; // Checking t each time here results in one less jmp in assembly..crazy
      l.terrainCBBPercent = t ? (*rc->cbb)[l.radial][l.gate] : 0.0;
      l.terrainPBBPercent = t ? (*rc->pbb)[l.radial][l.gate] : 0.0;
      l.beamHitBottom     = t ? (*rc->bottomHit)[l.radial][l.gate] : 0.0;
      return true;
    }

    return false;
  } // queryLayer

  /** Query a single layer.  Inline the code since this is called a silly amount of times. */
  inline bool
  queryLayer(VolumeValue& vv, LayerValue& l, DataTypePointerCache * c)
  {
    // Verified. Tilts can be null high above or below us
    if (c == nullptr) {
      return false;
    }

    // Assume RadialSet cache (FIXME: Generalize for all DataTypes)
    // Using this pointer cache increases speed massively, like 7x or more
    auto * rc = static_cast<RadialSetPointerCache *>(c);
    auto * r  = static_cast<RadialSet *>(c->dt);
    auto * p  = (RadialSetProjection *) (rc->project);

    l.elevation = r->getElevationDegs();

    // Projection of height range using attentuation
    // Notice we cached sin and cos GCD for the grid

    Project::Cached_BeamPath_LLHtoAttenuationRange(vv.getRadarHeightKMs(),
      vv.sinGcdIR, vv.cosGcdIR, r->getElevationTan(), r->getElevationCos(), l.heightKMs, l.rangeKMs);

    // If good, query more stuff
    // FIXME: Speed up by making a pointer version of this?
    if (p->getValueAtAzRange(vv.virtualAzDegs, l.rangeKMs, l.value, l.radial, l.gate)) {
      // Data value at gate (getValueAtAzRange sets this)
      // l.value = r.getFloat2DRef()[l.radial][l.gate]; This is slow, projection has pointer

      // Beamwidth info
      l.beamWidth = (*rc->bw)[l.radial];
      #if 0
      AngleDegs compare = r->getBeamWidthRef()[l.radial]; // Pull the slooow way and compare
      AngleDegs compare = (*rc->bw)[l.radial];
      if (l.beamWidth != compare) {
        LogSevere("Test beamwidth crash\n");
        exit(1);
      }
      #endif

      // Terrain info
      // Turns out O(CONUS) of get ref calls is slower than snails, so we cache the pointers
      const bool t = r->haveTerrain();
      l.haveTerrain       = t; // Checking t each time here results in one less jmp in assembly..crazy
      l.terrainCBBPercent = t ? (*rc->cbb)[l.radial][l.gate] : 0.0;
      l.terrainPBBPercent = t ? (*rc->pbb)[l.radial][l.gate] : 0.0;
      l.beamHitBottom     = t ? (*rc->bottomHit)[l.radial][l.gate] : 0.0;

      return true;
    }

    return false;
  } // queryLayer

public:

  // Direct routines for querying a layer.  I was abstracting it to enum, but
  // it's not worth the slow down.  If we add tilts/layers we can just add methods

  /** Query lower tilt */
  inline bool
  queryLower(VolumeValue& vv)
  {
    return (queryLayer(vv, vv.getLowerValue(), vv.getPC(VolumeValue::Layer::Lower)));
  }

  /** Query 2nd lower tilt */
  inline bool
  query2ndLower(VolumeValue& vv)
  {
    return (queryLayer(vv, vv.get2ndLowerValue(), vv.getPC(VolumeValue::Layer::Lower2)));
  }

  /** Query upper tilt */
  inline bool
  queryUpper(VolumeValue& vv)
  {
    return (queryLayer(vv, vv.getUpperValue(), vv.getPC(VolumeValue::Layer::Upper)));
  }

  /** Query 2nd upper tilt */
  inline bool
  query2ndUpper(VolumeValue& vv)
  {
    return (queryLayer(vv, vv.get2ndUpperValue(), vv.getPC(VolumeValue::Layer::Upper2)));
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
