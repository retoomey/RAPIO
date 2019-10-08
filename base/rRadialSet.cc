#include "rRadialSet.h"

#include "rError.h"
#include "rUnit.h"
#include "rIJK.h"
#include "rLLH.h"

#include <cassert>

using namespace rapio;
using namespace std;

RadialSet::RadialSet(const LLH& location,
  const Time                  & time,
  const Length                & dist_to_first_gate)
  :
  myCenter(location),
  myTime(time),
  myFirst(dist_to_first_gate)
{
  // Lookup for read/write factories
  myDataType = "RadialSet";
  init(0, 0);
}

const LLH&
RadialSet::getRadarLocation() const
{
  return (myCenter);
}

std::string
RadialSet::getGeneratedSubtype() const
{
  return (formatString(getElevation(), 5, 2)); // RadialSet
}

double
RadialSet::getElevation() const
{
  return (myElevAngleDegs);
}

void
RadialSet::init(size_t num_radials, size_t num_gates, const float fill)
{
  /** Declare/update the dimensions */
  declareDims({ num_radials, num_gates });

  // Fill values are meaningless since we start off 0,0 for now...
  // FIXME: On a resize maybe fill in the default arrays?

  /** As a grid of data */
  // addFloat2D("primary", "Dimensionless", {0,1}, fill);
  addFloat2D("primary", "Dimensionless", { 0, 1 });

  // These are the only ones we force...

  /** Azimuth per radial */
  // addFloat1D("Azimuth", "Degrees", {0}, 0.0f);
  addFloat1D("Azimuth", "Degrees", { 0 });

  /** Beamwidth per radial */
  // addFloat1D("BeamWidth", "Degrees", {0}, 1.0f);
  addFloat1D("BeamWidth", "Degrees", { 0 });

  /** Gate width per radial */
  // addFloat1D("GateWidth", "Meters", {0}, 1000.0f);
  addFloat1D("GateWidth", "Meters", { 0 });
}

/*
 * void
 * RadialSet::resize(size_t num_radials, size_t num_gates, const float fill)
 * {
 * declareDims({num_radials, num_gates});
 * }
 */

size_t
RadialSet::getNumGates()
{
  return getY(getFloat2D("primary"));
}

size_t
RadialSet::getNumRadials()
{
  return getX(getFloat2D("primary"));
}

void
RadialSet::setElevation(const double& targetElev)
{
  myElevAngleDegs = targetElev;
}

LLH
RadialSet::getLocation() const
{
  return (getRadarLocation());
}

Time
RadialSet::getTime() const
{
  return (myTime);
}

Length
RadialSet::getDistanceToFirstGate() const
{
  if ((myFirst.meters() != Constants::MissingData) &&
    (myFirst != Length()))
  {
    /*
     * LogSevere("\n\n\nFIRST GATE stored " << myFirst << "\n");
     *
     * // temp hack.  Something up with library
     * const LLH& first_gate = myRadials[0].getStartLocation();
     * //IJK displacement_km = (first_gate - myCenter).getVector_ECCC();
     * IJK displacement_km = (first_gate - myCenter);
     * LogSevere("Calculated: " << Length::Kilometers(displacement_km.norm()) << "\n");
     */

    return (myFirst);
  }

  // Calculate a first distance based on center and start location
  // I think this was ALWAYS zero.  Start locations of radials were the center, lol
  //  assert(myRadials.size() > 0); // at least one radial present?
  //  const LLH& first_gate = myRadials[0].getStartLocation();
  // const LLH& first_gate = myRadials[0].getStartLocation();
  // const LLH& first_gate = myCenter; // Where is first gate start information?

  // IJK displacement_km = (first_gate - myCenter).getVector_ECCC();
  // IJK displacement_km = (first_gate - myCenter);
  // return (Length::Kilometers(displacement_km.norm()));
  return (Length::Kilometers(0));
}
