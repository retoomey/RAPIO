#pragma once

#include <rData.h>
#include <rConstants.h>
#include <rLength.h>
#include <rLLH.h>
#include <rDataType.h>
#include <rTime.h>
#include <rDataStore2D.h>
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

  virtual LLH
  getLocation() const override;

  virtual Time
  getTime() const override;

  RadialSet(const LLH& location,
    const Time       & time,
    const Length     & dist_to_first_gate)
    :
    myCenter(location),
    myTime(time),
    myFirst(dist_to_first_gate)
  {
    // Lookup for read/write factories
    myDataType = "RadialSet";

    /** Push back primary band.  This is the primary moment
     * of the Radial set */
    addFloat2D("primary", 0, 0); // Force add zero for moment...
  }

  /** Sparse2D template wants this method (read) */
  void
  set(size_t i, size_t j, const float& value) final override
  {
    myFloat2DData[0].set(i, j, value); // FIXME: API?
  }

  /** Replace a custom missing/range with our constants */
  void
  replaceMissing(const float missing, const float range)
  {
    for (auto& i:myFloat2DData[0]) {
      float * v = &i;
      if (*v == missing) {
        *v = Constants::MissingData;
      } else if (*v == range) {
        *v = Constants::RangeFolded;
      }
    }
  }

  /** Sparse2D template wants this method.
   * FIXME: not we currently pass data type, but we're
   * in the middle of changing from IS to HAS */
  void
  fill(const float& value) final override
  {
    std::fill(myFloat2DData[0].begin(), myFloat2DData[0].end(), value);
  }

  /** Return the location of the radar. */
  const LLH&
  getRadarLocation() const
  {
    return (myCenter);
  }

  /** Elevation of radial set */
  double
  getElevation() const;

  /** Set the target elevation of this sweep. */
  void
  setElevation(const double& targetElev);

  /** Storage for radials as 2D and 1D vectors  */
  void
  reserveRadials(size_t num_gates, size_t num_radials)
  {
    /** As a grid of data */
    myFloat2DData[0].resize(num_gates, num_radials, Constants::DataUnavailable);
    const size_t ydim = myFloat2DData[0].getY();

    /** Azimuth per radial */
    addFloat1D("Azimuth", ydim);

    /** Beamwidth per radial */
    addFloat1D("BeamWidth", ydim, 1.0f);

    /** Azimuth spaceing per radial */
    addFloat1D("AzimuthalSpacing", ydim, 1.0f);

    /** Gate width per radial */
    addFloat1D("GateWidth", ydim, 1000.0f);

    /** Radial time per radial */
    addInt1D("RadialTime", ydim, 0);

    /** Nyquist per radial */
    addFloat1D("Nyquist", ydim, Constants::MissingData);
  } // reserveRadials

  // -----------------------
  // Vector access

  /** Allow reader/writer access to full vector */
  DataStore<float> *
  getAzimuthVector(){ return getFloat1D("Azimuth"); }

  /** Allow reader/writer access to full vector */
  DataStore<float> *
  getBeamWidthVector(){ return getFloat1D("BeamWidth"); }

  /** Allow reader/writer access to full vector */
  DataStore<float> *
  getAzimuthSpacingVector(){ return getFloat1D("AzimuthalSpacing"); }

  /** Allow reader/writer access to full vector */
  DataStore<float> *
  getGateWidthVector(){ return getFloat1D("GateWidth"); }

  /** Allow reader/writer access to full vector */
  DataStore<int> *
  getRadialTimeVector(){ return getInt1D("RadialTime"); }

  /** Allow reader/writer access to full vector */
  DataStore<float> *
  getNyquistVector(){ return getFloat1D("Nyquist"); }

  /** Get number of gates for radial set */
  size_t
  getNumGates()
  {
    return myFloat2DData[0].getX();
  }

  /** Get number of radials for radial set */
  size_t
  getNumRadials()
  {
    return myFloat2DData[0].getY();
  }

  /** Distance from radar center to the first gate */
  Length
  getDistanceToFirstGate() const;

  virtual std::string
  getGeneratedSubtype() const;

  /** Set the units used for nyquist, if any */
  void
  setNyquistVelocityUnit(const std::string& unit)
  {
    myNyquistUnit = unit;
  }

  /** Get the units used for nyquist, if any */
  std::string
  getNyquistVelocityUnit()
  {
    return myNyquistUnit;
  }

protected:
  // I think these could be considered projection information

  /** The elevation angle of radial set in degrees */
  double myElevAngleDegs;

  /** Center of Radial set */
  LLH myCenter;

  /** Time of this radial set */
  Time myTime;

  /** Distance to the first gate */
  Length myFirst;

  /** Units for Nyquist values, if any */
  std::string myNyquistUnit;
};
}
