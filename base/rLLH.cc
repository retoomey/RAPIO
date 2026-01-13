#include "rLLH.h"

#include "rXYZ.h"
#include "rConstants.h"
#include "rError.h"

#include <iomanip>
#include <iostream>

using namespace rapio;
using namespace std;

LLH::LLH() : LL(0, 0), myHeightKMs(0)
{ }

LLH::LLH(
  const AngleDegs& latitude, const AngleDegs& longitude, const LengthKMs& height
) : LL(latitude, longitude), myHeightKMs(height)
{ }

LLH::LLH(const LL& a, const LengthKMs& height)
  : LL(a.getLatitudeDeg(), a.getLongitudeDeg()), myHeightKMs(height)
{ }

ostream&
rapio::operator << (ostream& output, const LLH& loc)
{
  output << fmt::format("{}", loc);
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
