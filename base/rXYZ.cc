#include <rXYZ.h>

#include <rConstants.h>
#include <rLLH.h>
#include <iostream>

#include <math.h>

using namespace rapio;
using namespace std;

XYZ::XYZ(double a, double b, double c)
  : x(a), y(b), z(c)
{ }

LLH
XYZ::getLocation() const
{
  double r = sqrt(x * x + y * y + z * z);

  double lat = ((asin(z / r) * 180.0 / M_PI));
  double lon = ((atan2(y, x) * 180.0 / M_PI));

  double h = r - Constants::EarthRadiusKM;

  return (LLH(lat, lon, h));
}

XYZ::XYZ(const LLH& loc)
{
  const double r = Constants::EarthRadiusKM
    + loc.getHeightKM();

  const double phi      = loc.getLongitudeDeg() * M_PI / 180.0;
  const double beta     = loc.getLatitudeDeg() * M_PI / 180.0;
  const double cos_beta = std::cos(beta);

  x = r * std::cos(phi) * cos_beta;
  y = r * std::sin(phi) * cos_beta;
  z = r * std::sin(beta);

  // std::cout << this << " Create XYZ " << x1 << ", " << y1 << " " << z1 << "\n";
  // x = x1; y = y1; z = z1;
}

IJK
XYZ::operator - (const XYZ& p) const
{
  return (IJK(x - p.x, y - p.y, z - p.z));
}

XYZ
XYZ::operator + (const IJK& v) const
{
  return (XYZ(x + v.x, y + v.y, z + v.z));
}

XYZ
XYZ::operator - (const IJK& v) const
{
  return (XYZ(x - v.x, y - v.y, z - v.z));
}

XYZ&
XYZ::operator += (const IJK& v)
{
  x += v.x;
  y += v.y;
  z += v.z;

  return (*this);
}

XYZ&
XYZ::operator -= (const IJK& v)
{
  x -= v.x;
  y -= v.y;
  z -= v.z;

  return (*this);
}

ostream&
rapio::operator << (ostream& os, const XYZ& p)
{
  os << "(" << p.x << "," << p.y << "," << p.z << ")" << flush;
  return (os);
}
