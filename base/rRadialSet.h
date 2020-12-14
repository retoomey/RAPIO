#pragma once

#include <rData.h>
#include <rConstants.h>
#include <rLength.h>
#include <rLLH.h>
#include <rDataType.h>
#include <rTime.h>
#include <rDataGrid.h>

#include <vector>

namespace rapio {
/** A Radial set is a collection of radials containing gates.  This makes
 * it a 2D data structure.  It stores 1D information for each radials.
 * It can store multiple bands of 2D data.
 *
 * @author Robert Toomey */
class RadialSet : public DataGrid {
public:

  /** Construct uninitialized RadialSet, usually for
   * factories */
  RadialSet();

  /** Construct a radial set */
  RadialSet(const LLH& location,
    const Time       & time,
    const Length     & dist_to_first_gate);

  /** Return the location of the radar. */
  const LLH&
  getRadarLocation() const
  {
    // We use the location as the center
    return myLocation;
  };

  /** Elevation of radial set */
  double
  getElevation() const;

  /** Set the target elevation of this sweep. */
  void
  setElevation(const double& targetElev);

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

  /** Get number of gates for radial set */
  size_t
  getNumGates();

  /** Get number of radials for radial set */
  size_t
  getNumRadials();

  /** Distance from radar center to the first gate */
  Length
  getDistanceToFirstGate() const;

  virtual std::string
  getGeneratedSubtype() const override;

  /** Resize the data structure */
  void
  init(size_t rows, size_t cols, const float fill = 0);

  /** Resize the data structure */
  // virtual void
  // resize(size_t rows, size_t cols, const float fill = 0) override;

  /** Sync any internal stuff to data from current attribute list,
   * return false on fail. */
  virtual bool
  initFromGlobalAttributes() override;

  /** Update global attribute list for RadialSet */
  virtual void
  updateGlobalAttributes(const std::string& encoded_type) override;

protected:
  // I think these could be considered projection information

  /** The elevation angle of radial set in degrees */
  double myElevAngleDegs;

  /** Distance to the first gate */
  Length myFirst;
};
}
