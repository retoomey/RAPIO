#include <rIJK.h>

#include <rConstants.h>
#include <rLLH.h>
#include <rXYZ.h>

#include <cassert>
#include <math.h>
#include <iostream>

using namespace rapio;
using namespace std;

IJK::IJK(double a, double b, double c)
  : x(a), y(b), z(c)
{
  // std::cout << this << " NEW IJK " << x << ", " << y << ", " << z << "\n";
}

IJK::IJK(
  const LLH& a,
  const LLH& b
)
{
  IJK v = XYZ(b) - XYZ(a);

  x = v.x;
  y = v.y;
  z = v.z;
}

IJK::IJK(
  const LLH       & ref,
  const double    & del_lat,
  const double    & del_lon,
  const LengthKMs & del_htKMs
)
{
  LLH target_loc
  (
    ref.getLatitudeDeg() + del_lat,
    ref.getLongitudeDeg() + del_lon,
    ref.getHeightKM() + del_htKMs
  );

  IJK b = XYZ(target_loc) - XYZ(ref);

  x = b.x;
  y = b.y;
  z = b.z;
}

IJK::IJK(
  const LLH       & ref,
  const LengthKMs & dist_east,
  const LengthKMs & dist_north,
  const LengthKMs & del_ht
)
{
  const double Earth_Circumference = 2.0 * M_PI * 6371000.;
  const double deg_per_meter       = 360.0 / Earth_Circumference;
  const double DEGtoRAD = M_PI / 180.;

  auto X         = dist_east * 1000;
  auto Y         = dist_north * 1000;
  auto start_lat = ref.getLatitudeDeg();
  auto start_lon = ref.getLongitudeDeg();

  const double Range = sqrt(X * X + Y * Y);
  double Az = atan2(X, Y) / DEGtoRAD;

  if (Az < 0) { Az = 360.0 + Az; }

  const double target_lat = start_lat + (Range * cos(Az * DEGtoRAD)
    * deg_per_meter);
  const double target_lon = start_lon + (
    (Range * sin(Az * DEGtoRAD) * deg_per_meter)
    / cos((start_lat + target_lat) / 2.0 * DEGtoRAD));

  const double target_htKM = ref.getHeightKM() + del_ht;

  LLH target_loc(target_lat, target_lon, target_htKM);
  IJK a = XYZ(target_loc) - XYZ(ref);

  x = a.x;
  y = a.y;
  z = a.z;
}

ostream&
rapio::operator << (ostream& os, const IJK& v)
{
  return os << fmt::format("{}", v);
}

IJK
IJK::unit() const
{
  double len = norm();

  if (len <= 0) {
    return (*this); // zero vector
  }
  return ((*this) / len);
}

double
IJK::norm() const
{
  return (sqrt(x * x + y * y + z * z));
}

double
IJK::normSquared() const
{
  return (x * x + y * y + z * z);
}

IJK
rapio::operator * (double f, const IJK& v)
{
  return (IJK(f * v.x, f * v.y, f * v.z));
}
