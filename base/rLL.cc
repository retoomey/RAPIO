#include "rLL.h"
#include "rConstants.h"

#include <iostream>

using namespace rapio;

LL::LL() : myLatitudeDegs(0), myLongitudeDegs(0)
{ }

LL::LL(const double& lat, const double& lon) :
  myLatitudeDegs(lat),
  myLongitudeDegs(lon)
{
  // std::cout << "LL SET TO " << lat << ", " << lon << "\n";
}

namespace rapio {
std::ostream&
operator << (std::ostream& os, const LL& latlon)
{
  os << "[lat=" << latlon.myLatitudeDegs << ",lon=" << latlon.myLongitudeDegs
     << ']';
  return (os);
}
}

bool
LL::isValid() const
{
  const double lat(myLatitudeDegs);
  const double lon(myLongitudeDegs);

  return (-90 <= lat && lat <= 90 && -180 < lon && lon < 180);
}

LengthKMs
LL::getSurfaceDistanceToKMs(const LL& b) const
{
  // Assuming a perfect sphere earth, give great circle
  // distance between two locations.  Extremely accurate, if
  // not precise.  Note height of locations is meaningless here, we
  // are assuming both locations are on earth surface
  double alat = myLatitudeDegs * M_PI / 180.0;
  double blat = b.getLatitudeDeg() * M_PI / 180.0;

  double longdiff    = (myLongitudeDegs - b.getLongitudeDeg()) * M_PI / 180.0;
  double d           = sin(alat) * sin(blat) + cos(alat) * cos(blat) * cos(longdiff);
  LengthKMs distance = acos(d)
    * (Constants::EarthRadiusKM);

  return (distance);
}
