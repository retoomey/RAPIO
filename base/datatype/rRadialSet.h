#pragma once

#include <rDataGrid.h>
#include <rLLH.h>
#include <rTime.h>

namespace rapio {
/** Store collection of pointers for fast access to the RadialSet.
 * Useful for massive loops like in fusion.  Gets around the safety
 * checks, etc.  This works for the 'general' most used fields.
 * With custom arrays you're still stuck with the get methods. */
class RadialSet;
class RadialSetPointerCache : public DataTypePointerCache {
public:
  ArrayFloat1DPtr bw; ///< Beamwidth reference array

  // Assuming initTerrain is true, otherwise nullptr
  ArrayFloat2DPtr cbb;      ///< One Terrain Cumulative beam blockage array per layer
  ArrayFloat2DPtr pbb;      ///< One Terrain Partial beam blockage array per layer
  ArrayByte2DPtr bottomHit; ///< One Terrain bottom hit array per layer
};

/** A Radial set is a collection of radials containing gates.  This makes
 * it a 2D data structure.  It stores 1D information for each radials.
 * It can store multiple bands of 2D data.
 *
 * @author Robert Toomey
 */
class RadialSet : public DataGrid {
public:

  // Constants for special fields
  static constexpr const char * BeamWidth      = "BeamWidth";
  static constexpr const char * Azimuth        = "Azimuth";
  static constexpr const char * GateWidth      = "GateWidth";
  static constexpr const char * AzimuthSpacing = "AzimuthSpacing";

  /** Construct uninitialized RadialSet, usually for
   * factories.  You probably want the Create method */
  RadialSet();

  /** Public API for users to create a single band RadialSet quickly,
   * using polar grid style.  RadialSets are typically small enough that
   * we don't gain anything from variable radial length.
   * Note that data is uninitialized/random memory since most algorithms
   * you'll fill it in and it wastes time to double fill it. */
  static std::shared_ptr<RadialSet>
  Create(
    const std::string& TypeName,
    const std::string& Units,
    const LLH        & center,
    const Time       & datatime,
    const float      elevationDegrees,
    const float      firstGateDistanceMeters,
    const float      gateWidthMeters,
    const size_t     num_radials,
    const size_t     num_gates);

  /** Public API for users to clone a RadialSet */
  std::shared_ptr<RadialSet>
  Clone();

  /** Remap to another RadialSet resolution, optionally projecting
   * slant range to ground.  Useful for polar algorithms that need to
   * march in vertical polar with multiple elevation angles. */
  std::shared_ptr<RadialSet>
  Remap(const float gateWidthMeters,
    const size_t    num_radials,
    const size_t    num_gates,
    bool            projectToGround);

  /** Get a pointer cache to critical things.  Note, changing the RadialSet
   * massively might invalidate these pointers.  They can also go out of scope.
   */
  std::shared_ptr<DataTypePointerCache>
  getDataTypePointerCache() override
  {
    if (myPointerCache == nullptr) {
      auto newone = std::make_shared<RadialSetPointerCache>();

      auto& n = *newone;
      // All these objects are cached as shared_ptr in RadialSet
      // So they will stay in scope.
      n.dt      = this;
      n.project = getProjection().get(); // Using primary (main one)
      n.bw      = getFloat1D(BeamWidth)->ptr();
      // FIXME: Check initterrain?
      n.cbb          = getFloat2D(Constants::TerrainCBBPercent)->ptr();
      n.pbb          = getFloat2D(Constants::TerrainPBBPercent)->ptr();
      n.bottomHit    = getByte2D(Constants::TerrainBeamBottomHit)->ptr();
      myPointerCache = newone;
    }
    return myPointerCache;
  }

  /** Return the location of the radar. */
  const LLH&
  getRadarLocation() const
  {
    // We use the location as the center
    return myLocation;
  };

  /** Elevation of radial set */
  AngleDegs
  getElevationDegs() const
  {
    return (myElevAngleDegs);
  };

  /** Radar name for radial set, if any */
  std::string
  getRadarName() const
  {
    std::string aName;

    getString("radarName-value", aName);
    return aName;
  }

  /** Radar VCP number for radial set, if any */
  int
  getVCP() const
  {
    int vcp = -99;
    std::string vcpstr;

    getString("vcp-value", vcpstr);
    try {
      vcp = std::stoi(vcpstr);
    }catch (const std::exception&) { }
    return vcp;
  }

  /** Cached sin of elevation angle, used in speed calculations in volumes */
  double
  getElevationCos() const
  {
    return (myElevCos);
  }

  /** Cached tan of elevation angle, used in speed calculation in volumes */
  double
  getElevationTan() const
  {
    return (myElevTan);
  }

  /** Get an estimated gatewidth of RadialSet, this will be gatewidth of first radial, if any */
  LengthKMs
  getGateWidthKMs() const
  {
    LengthKMs widthKM = .250;
    auto& gw = getFloat1DRef(GateWidth);

    if (gw.size() > 0) {
      widthKM = gw[0] / 1000.0;
    }
    return widthKM;
  }

  /** Set the target elevation of this sweep. */
  void
  setElevationDegs(AngleDegs targetElev)
  {
    myElevAngleDegs = targetElev;
    myElevCos       = cos(targetElev * DEG_TO_RAD);
    myElevTan       = tan(targetElev * DEG_TO_RAD);
    // We also use elevation as our subtype, so update that
    setSubType(getGeneratedSubtype());
  };

  /** Set the radar name for this radial set */
  void
  setRadarName(const std::string& aName)
  {
    setDataAttributeValue("radarName", aName, "dimensionless");
  }

  /** Set the radar vcp number */
  void
  setVCP(int vcp)
  {
    setDataAttributeValue("vcp", std::to_string(vcp), "dimensionless");
  }

  // -----------------------
  // Vector access

  /** Allow reader/writer access to full vector */
  std::shared_ptr<Array<float, 1> >
  getAzimuthVector(){ return getFloat1D(Azimuth); }

  /** Return quick ref to azimuths */
  ArrayFloat1DRef
  getAzimuthRef()
  {
    return (getFloat1D(Azimuth))->ref();
  }

  /** Allow reader/writer access to full vector */
  std::shared_ptr<Array<float, 1> >
  getAzimuthSpacingVector()
  {
    auto array = getFloat1D(AzimuthSpacing);

    if (array == nullptr) {
      array = getFloat1D(BeamWidth);
    }
    return array;
  }

  /** Allow reader/writer access to full vector */
  std::shared_ptr<Array<float, 1> >
  getBeamWidthVector(){ return getFloat1D(BeamWidth); }

  /** Return quick ref to beam width, assuming it exists. */
  ArrayFloat1DRef
  getBeamWidthRef()
  {
    return (getFloat1D(BeamWidth))->ref();
  }

  /** Allow reader/writer access to full vector */
  std::shared_ptr<Array<float, 1> >
  getGateWidthVector(){ return getFloat1D(GateWidth); }

  /** Get number of radials for radial set */
  size_t
  getNumRadials() const
  {
    return myDims.size() > 0 ? myDims[0].size() : 0;
  };

  /** Get number of gates for radial set */
  size_t
  getNumGates() const
  {
    return myDims.size() > 1 ? myDims[1].size() : 0;
  };

  /** Distance from radar center to the first gate */
  double
  getDistanceToFirstGateM() const
  {
    return myFirstGateDistanceM;
  };

  /** Generated default string for subtype from the data */
  virtual std::string
  getGeneratedSubtype() const override;

  /** Projection for data type */
  virtual std::shared_ptr<DataProjection>
  getProjection(const std::string& layer = Constants::PrimaryDataName) override;

  /** Update global attribute list for RadialSet */
  virtual void
  updateGlobalAttributes(const std::string& encoded_type) override;

  /** Sync any internal stuff to data from current attribute list,
   * return false on fail. */
  virtual bool
  initFromGlobalAttributes() override;

  /** Validate mandatory RadialSet arrays */
  void
  validateArrays(bool warnOnMissing, float gateWidthMeters);

  /** Initialize a RadialSet to given parameters */
  bool
  init(
    const std::string& TypeName,
    const std::string& Units,
    const LLH        & center,
    const Time       & datatime,
    const float      elevationDegrees,
    const float      firstGateDistanceMeters,
    const float      gateWidthMeters,
    const size_t     num_radials,
    const size_t     num_gates);

  /** Handle post read by sparse uncompression if wanted */
  virtual void
  postRead(std::map<std::string, std::string>& keys) override;

  /** Make ourselves MRMS sparse iff we're non-sparse.  This keeps
   * any DataGrid writers like netcdf generic not knowing about our
   * special sparse formats. */
  virtual void
  preWrite(std::map<std::string, std::string>& keys) override;

  /** Make ourselves MRMS non-sparse iff we're sparse */
  virtual void
  postWrite(std::map<std::string, std::string>& keys) override;

  // ------------------------------------------------
  // Terrain information methods
  // Optional arrays if a polar terrain has been run on this RadialSet

  /** Add or prepare the terrain arrays, pointers, and access methods.
   * Call this before calling haveTerrain or the array fetch routines.
   * If you're crashing calling the terrain ref functions, you probably
   * didn't do this. */
  void
  initTerrain();

  /** Do we have valid terrain arrays? */
  inline bool
  haveTerrain()
  {
    return myHaveTerrain;
  }

  /** Return quick ref to terrain CBB, valid if have terrain is true */
  ArrayFloat2DRef
  getTerrainCBBPercentRef()
  {
    return (getFloat2D(Constants::TerrainCBBPercent))->ref();
  }

  /** Return quick ref to terrain PBB, valid if have terrain is true */
  ArrayFloat2DRef
  getTerrainPBBPercentRef()
  {
    return (getFloat2D(Constants::TerrainPBBPercent))->ref();
  }

  /** Return quick ref to terrain bottom hit, valid if have terrain is true */
  ArrayByte2DRef
  getTerrainBeamBottomHitRef()
  {
    return (getByte2D(Constants::TerrainBeamBottomHit))->ref();
  }

  // ------------------------------------------------

protected:

  /** Deep copy our fields to a new subclass */
  void
  deep_copy(std::shared_ptr<RadialSet> n);

  /** The elevation angle of radial set in degrees */
  double myElevAngleDegs;

  /** The cached cos of current elevation angle */
  double myElevCos;

  /** The cached tan of current elevation angle */
  double myElevTan;

  /** Distance to the first gate */
  double myFirstGateDistanceM;

  /** Do we have the terrain arrays? */
  bool myHaveTerrain;
};
}
