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
  /** As a grid of data */
  addFloat2D("primary", "Dimensionless", num_radials, num_gates, fill);

  // These are the only ones we force...

  /** Azimuth per radial */
  addFloat1D("Azimuth", "Degrees", num_radials, 0.0f);

  /** Beamwidth per radial */
  addFloat1D("BeamWidth", "Degrees", num_radials, 1.0f);

  /** Gate width per radial */
  addFloat1D("GateWidth", "Meters", num_radials, 1000.0f);
}

void
RadialSet::resize(size_t num_radials, size_t num_gates, const float fill)
{
  auto list = getArrays();

  for (auto l:list) {
    // FIXME: Feel this could be more generic
    const int size = l->getDims().size();
    if (size == 1) {
      resizeFloat1D(l->getName(), num_radials, 0.0f);
    } else if (size == 2) {
      resizeFloat2D(l->getName(), num_radials, num_gates, fill);
    }
  }
}

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
