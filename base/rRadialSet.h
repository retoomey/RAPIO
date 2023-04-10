#pragma once

#include <rDataGrid.h>
#include <rLLH.h>
#include <rTime.h>
#include <rRadialSetLookup.h>

namespace rapio {
/** A Radial set is a collection of radials containing gates.  This makes
 * it a 2D data structure.  It stores 1D information for each radials.
 * It can store multiple bands of 2D data.
 *
 * @author Robert Toomey
 */
class RadialSet : public DataGrid {
public:
  friend RadialSetProjection;

  /** Construct uninitialized RadialSet, usually for
   * factories.  You probably want the Create method */
  RadialSet();

  /** Create a new RadialSet */
  RadialSet(
    const std::string& TypeName,
    const std::string& Units,
    const LLH        & center,
    const Time       & datatime,
    const float      elevationDegrees,
    const float      firstGateDistanceMeters,
    const size_t     num_radials,
    const size_t     num_gates);

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
    const size_t     num_radials,
    const size_t     num_gates);

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
  getGateWidthKMs()
  {
    LengthKMs widthKM = .250;
    auto& gw = getFloat1D("GateWidth")->ref();

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
  getAzimuthVector(){ return getFloat1D("Azimuth"); }

  /** Allow reader/writer access to full vector */
  std::shared_ptr<Array<float, 1> >
  getAzimuthSpacingVector()
  {
    auto array = getFloat1D("AzimuthSpacing");

    if (array == nullptr) {
      array = getFloat1D("BeamWidth");
    }
    return array;
  }

  /** Allow reader/writer access to full vector */
  std::shared_ptr<Array<float, 1> >
  getBeamWidthVector(){ return getFloat1D("BeamWidth"); }

  /** Allow reader/writer access to full vector */
  std::shared_ptr<Array<float, 1> >
  getGateWidthVector(){ return getFloat1D("GateWidth"); }

  /** Get number of radials for radial set */
  size_t
  getNumRadials()
  {
    return myDims.size() > 0 ? myDims[0].size() : 0;
  };

  /** Get number of gates for radial set */
  size_t
  getNumGates()
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

  /** Return pointer to the lookup for immediate access, only valid
   * while RadialSet is */
  RadialSetLookup *
  getRadialSetLookupPtr() const
  {
    return myLookup.get();
  }

protected:
  /** The elevation angle of radial set in degrees */
  double myElevAngleDegs;

  /** The cached cos of current elevation angle */
  double myElevCos;

  /** The cached tan of current elevation angle */
  double myElevTan;

  /** Distance to the first gate */
  double myFirstGateDistanceM;

  /** Cache lookup (FIXME: unique_ptr c++14 among other locations in code) */
  std::shared_ptr<RadialSetLookup> myLookup;
};
}
