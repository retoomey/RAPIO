#include "rLLH.h"

#include "rXYZ.h"
#include "rConstants.h"
#include "rError.h"

#include <iomanip>
#include <iostream>

using namespace rapio;
using namespace std;

LLH::LLH() : LL(0, 0), myHeight(0)
{ }

LLH::LLH(
  const double& latitude, const double& longitude, const double& height
) : LL(latitude, longitude), myHeight(height)
{ }

LLH::LLH(const LL& a, const double& height)
  : LL(a.getLatitudeDeg(), a.getLongitudeDeg()), myHeight(height)
{ }

ostream&
rapio::operator << (ostream& output, const LLH& exp)
{
  output
    << "(lat=" << std::setprecision(8) << exp.getLatitudeDeg() << ","
    << "lon=" << std::setprecision(8) << exp.getLongitudeDeg() << ","
    << "h=" << std::setprecision(8) << exp.getHeightKM() << ")";

  return (output);
}

LLH
LLH::operator + (const IJK& d) const
{
  return ((XYZ(*this) + d).getLocation());
}

LLH&
LLH::operator += (const IJK& d)
{
  return (*this = *this + d);
}

LLH
LLH::operator - (const IJK& d) const
{
  return ((XYZ(*this) - d).getLocation());
}

LLH&
LLH::operator -= (const IJK& d)
{
  return (*this = *this - d);
}

IJK
LLH::operator - (const LLH& loc) const
{
  return (IJK(XYZ(loc) - XYZ(*this)));
}
