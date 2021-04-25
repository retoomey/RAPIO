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

  /** Public API for users to create a single band RadialSet quickly,
   * note gates are padded with missing data as a grid. */
  static std::shared_ptr<RadialSet>
  Create(
    const std::string& TypeName,
    const std::string& Units,
    const LLH        & center,
    const Time       & datatime,
    const float      elevationDegrees,
    const float      firstGateDistanceMeters,
    const size_t     num_radials,
    const size_t     num_gates,
    const float      value = Constants::MissingData);

  // ------------------------------------------------------
  // Getting the 'data' of the 2d array...
  // @see DataGrid for accessing the various raster layers

  /** Return the location of the radar. */
  const LLH&
  getRadarLocation() const
  {
    // We use the location as the center
    return myLocation;
  };

  /** Elevation of radial set */
  double
  getElevation() const
  {
    return (myElevAngleDegs);
  };

  /** Set the target elevation of this sweep. */
  void
  setElevation(const double& targetElev)
  {
    myElevAngleDegs = targetElev;
  };

  // -----------------------
  // Vector access

  /** Allow reader/writer access to full vector */
  std::shared_ptr<Array<float, 1> >
  getAzimuthVector(){ return getFloat1D("Azimuth"); }

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
  getProjection(const std::string& layer = "primary") override;

  // ------------------------------------------------------------
  // Methods for factories, etc. to fill in data post creation
  // Normally you don't call these directly unless you are making
  // a factory

private:

  /** Post creation initialization of fields
   * Resize can change data size. */
  void
  init(
    const std::string& TypeName,
    const std::string& Units,
    const LLH        & location,
    const Time       & time,
    const float      elevationDegrees,
    const float      firstGateDistanceMeters,
    const size_t     num_radials,
    const size_t     num_gates,
    const float      fill = Constants::MissingData);

public:

  /** Update global attribute list for RadialSet */
  virtual void
  updateGlobalAttributes(const std::string& encoded_type) override;

  /** Sync any internal stuff to data from current attribute list,
   * return false on fail. */
  virtual bool
  initFromGlobalAttributes() override;

protected:
  /** The elevation angle of radial set in degrees */
  double myElevAngleDegs;

  /** Distance to the first gate */
  double myFirstGateDistanceM;
private:
  /** Cache lookup (FIXME: unique_ptr c++14 among other locations in code) */
  std::shared_ptr<RadialSetLookup> myLookup;
};
}
