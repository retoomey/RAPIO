#include "rRadialSet.h"

#include "rError.h"
#include "rUnit.h"
// #include "rDisplacement.h"
#include "rIJK.h"
#include "rLLH.h"

#include <cassert>

using namespace rapio;
using namespace std;

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
  assert(myRadials.size() > 0); // at least one radial present?
  const LLH& first_gate = myRadials[0].getStartLocation();
  // IJK displacement_km = (first_gate - myCenter).getVector_ECCC();
  IJK displacement_km = (first_gate - myCenter);
  return (Length::Kilometers(displacement_km.norm()));
}
