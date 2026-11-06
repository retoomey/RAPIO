#include "rLength.h"
#include "rError.h"
#include "rUnit.h"

#include <cassert>

using namespace rapio;

namespace {
bool
unitIsKM(std::string_view unit)
{
  return (unit == "Kilometers" ||
         unit == "Kilometer" ||
         unit == "kilometers" ||
         unit == "kilometer");
}
}

Length::Length(double val, std::string_view unit)
{
  km = unitIsKM(unit) ? val : Unit::value(unit, "kilometers", val);
}

double
Length::value(std::string_view unit)
{
  return (unitIsKM(unit) ? km : Unit::value("kilometers", unit, km));
}

namespace rapio {
std::ostream&
operator << (std::ostream& os, const Length& l)
{
  return os << fmt::format("{}", l);
}

Length
operator * (double scale, const Length& a)
{
  return (Length::Kilometers(scale * a.kilometers()));
}

Length
operator - (const Length& a)
{
  return (Length::Kilometers(-a.kilometers()));
}
}
