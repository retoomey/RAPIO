#include "rLength.h"
#include "rError.h"
#include "rUnit.h"

#include <cassert>

using namespace rapio;

namespace {
bool
unitIsKM(const std::string& unit)
{
  return (unit == "Kilometers" ||
         unit == "Kilometer" ||
         unit == "kilometers" ||
         unit == "kilometer");
}
}

Length::Length(double val, const std::string& unit)
{
  km = unitIsKM(unit) ? val : Unit::value(unit, "kilometers", val);
}

double
Length::value(const std::string& unit)
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
