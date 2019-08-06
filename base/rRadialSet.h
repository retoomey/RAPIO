#pragma once

#include <rData.h>
#include <rConstants.h>
#include <rLength.h>
#include <rLLH.h>
#include <rDataType.h>
#include <rTime.h>

#include <vector>

namespace rapio {
class Radial : public Data {
public:

  Radial(
    const double & starting_azimuth,
    const double & azimuthal_spacing,
    const double & elevation_of_sweep,
    const LLH    & location_of_first_gate,
    const Time   & time_of_first_gate,
    const Length & width_of_gate,
    const double & physical_beamwidth
  ) :
    gate_width(width_of_gate),
    az(starting_azimuth),
    beam_width(physical_beamwidth),
    az_spacing(azimuthal_spacing),
    elev_angle(elevation_of_sweep),
    start_location(location_of_first_gate),
    start_time(time_of_first_gate)
  { }

  void
  fill(const float& val)
  {
    for (float& i:values) {
      i = val;
    }
  }

  /** The average width of a gate within this radial. */
  const Length&
  getGateWidth() const
  {
    return (gate_width);
  }

  /** The constant (starting) azimuth of this radial. */
  const double&
  getAzimuth() const
  {
    return (az);
  }

  /** The constant beamwidth of this radial. Not necessarily
   *  the azimuthal spacing ... */
  const double&
  getPhysicalBeamWidth() const
  {
    return (beam_width);
  }

  /** The azimuthal spacing */
  const double&
  getAzimuthalSpacing() const
  {
    return (az_spacing);
  }

  /** The elevation (of the sweep) containing this radial. */
  const double&
  getElevation() const
  {
    return (elev_angle);
  }

  /** The starting location and time of this radial. */
  const LLH&
  getStartLocation() const
  {
    return (start_location);
  }

  /** The starting location and time of this radial. */
  const Time&
  getStartTime() const
  {
    return (start_time);
  }

  /** The average width of a gate within this radial. */
  void
  setGateWidth(const Length& len)
  {
    gate_width = len;
  }

  /** The constant (starting) azimuth of this radial. */
  void
  setAzimuth(const double& ang)
  {
    az = ang;
  }

  /** The constant beamwidth of this radial. */
  void
  setPhysicalBeamWidth(const double& ang)
  {
    beam_width = ang;
  }

  /** The constant beamwidth of this radial. */
  void
  setAzimuthalSpacing(const double& ang)
  {
    az_spacing = ang;
  }

  /** The elevation (of the sweep) containing this radial. */
  void
  setElevation(const double& ang)
  {
    elev_angle = ang;
  }

  /** The starting location and time of this radial. */
  void
  setStartLocation(const LLH& l)
  {
    start_location = l;
  }

  /** The starting location and time of this radial. */
  void
  setStartTime(const Time& t)
  {
    start_time = t;
  }

  void
  setNyquistVelocity(float nyquist, const std::string& unit)
  {
    myNyquist     = nyquist; // store per radial????
    myNyquistUnit = unit;    // Units could be at radial set level at least
  }

  bool
  getNyquestVelocity(float& nyquist, std::string& unit) const
  {
    nyquist = myNyquist;
    unit    = myNyquistUnit;
    return (unit != "");
  }

protected:

  Length gate_width; // getGateWidth()
  double az;         // getAzimuth()
  double beam_width; // getBeamWidth()
  double az_spacing; // getAzimuthalSpacing()
  double elev_angle; // getElevation()
  LLH start_location;
  Time start_time;
  float myNyquist;           // store per radial????
  std::string myNyquistUnit; // Units could be at radial set level at least

public:

  std::vector<float> values;

  // return &(myRadials[i].values[0]);
  const float *
  getDataVector() const
  {
    return (&(values[0]));
  }

  /** Used by direct writers to preset vector size */
  void
  setGateCount(size_t s)
  {
    values.resize(s);
  }
};

class RadialSet : public DataType {
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
  }

  /** Sparse2D template wants this method (read) */
  void
  set(size_t i, size_t j, const float& value) final override
  {
    myRadials[i].values[j] = value;
  }

  /** Sparse2D template wants this method */
  void
  fill(const float& val) final override
  {
    for (Radial& i: myRadials) {
      i.fill(val);
    }
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

  // Unimplmented ....

  /** Normalize the RadialSet to have a constant number
   * or gates per radial. */
  void
  normalizeGateNumber(){ }

  /** Clip radial set into a 360 degree one.  BROKEN */
  void
  makeLogical360(bool strictly_under_360 = false){ }

  void
  addRadial(Radial r)
  {
    myRadials.push_back(r);
  }

  size_t
  size_d()
  {
    return (myRadials.size());
  }

  size_t
  size_d(size_t i)
  {
    return (myRadials[i].values.size());
  }

  void
  addGate(size_t i, float v)
  {
    myRadials[i].values.push_back(v);
  }

  const Radial&
  getRadial(size_t i)
  {
    return (myRadials[i]);
  }

  float *
  getRadialVector(size_t i)
  {
    return (myRadials[i].values.data());
  }

  float *
  getRadialVector(size_t i, size_t j)
  {
    return (&(myRadials[i].values[j]));
  }

  Length
  getDistanceToFirstGate() const;

  virtual std::string
  getGeneratedSubtype() const;

protected:

  double myElevAngleDegs;

  LLH myCenter;
  Time myTime;
  Length myFirst;

  /** Store radials for radial set */
  std::vector<Radial> myRadials;
};
}
