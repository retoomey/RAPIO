#include "rUnit.h"

#include "rConfig.h"
#include "rError.h"
#include "rConstants.h"

#include <map>
#include <cstring> // strcmp()
#include <cstdlib> // putenv()
#include <iostream>

#if HAVE_UDUNITS2
# include <udunits.h>
#endif

using namespace rapio;

#if HAVE_UDUNITS
namespace {
struct Key {
  char * from;
  char * to;
  Key(char * f, char * t) : from(f), to(t){ }

  bool
  operator < (const Key& that) const
  {
    int i = strcmp(from, that.from);

    if (i) { return (i < 0); }

    return (strcmp(to, that.to) < 0);
  }
};

std::map<Key, Unit::UnitConverter> converter_cache;
std::map<std::string, utUnit> unit_cache;

const bool
getUtUnit(const std::string& u, utUnit& setme)
{
  const std::string& unit = u.empty() ? "dimensionless" : u;

  // Hunt a cache of converter objects...
  auto it(unit_cache.find(unit));

  if (it != unit_cache.end()) {
    setme = it->second;
    return (true);
  }

  if (utScan(unit.c_str(), &setme)) {
    fLogSevere("Unit '{}' not found in the udunits database file(s) in misc.", unit);
    return (false);
  }

  unit_cache[unit] = setme;
  return (true);
}
}
#endif // if HAVE_UDUNITS

void
ConfigUnit::introduceSelf()
{
  std::shared_ptr<ConfigType> units = std::make_shared<ConfigUnit>();

  Config::introduce("unit", units);
}

bool
ConfigUnit::readSettings(std::shared_ptr<PTreeData>)
{
  #if HAVE_UDUNITS
  // Look for one of the neccessary udunits2 xml files
  URL url = Config::getConfigFile("misc/udunits2.xml");

  if (url.getPath() == "") {
    fLogSevere("Udunits2 requires a misc/udunits2.xml file and others in one of your configuration paths.");
    return false;
  }
  // Initialize environment to xml files and udunits
  Config::setEnvVar("UDUNITS2_XML_PATH", url.getPath());
  #endif
  return true;
}

void
Unit::initialize()
{
  #if HAVE_UDUNITS
  // Initialize udunits2
  utInit("");
  #endif
}

bool
Unit::isValidUnit(const std::string& unit)
{
  #if HAVE_UDUNITS
  utUnit u;

  return (getUtUnit(unit, u) != 0);

  #else
  return false;

  #endif
}

bool
Unit::getConverter(const std::string& from,
  const std::string                 & to,
  UnitConverter                     & setme)
{
  #if HAVE_UDUNITS
  // Key holds char* rather than std::string because
  // lookup is 3x faster if we don't create new strings each time...
  // FIXME: Need to investigate this
  const Key lookup_key((char *) from.c_str(), (char *) to.c_str());

  auto it(converter_cache.find(lookup_key));

  if (it != converter_cache.end()) {
    setme = it->second;
    return (true);
  }

  // Never convert anything to dimensionless..doesn't make
  // logical sense.  Use 1 and 0 hack here to enforce this
  // for all code
  if ((to == "dimensionless") || (to == "")) {
    setme.slope     = 1;
    setme.intercept = 0;
  } else {
    utUnit f, t;

    if (!getUtUnit(from, f) ||
      !getUtUnit(to, t) ||
      utConvert(&f, &t, &setme.slope, &setme.intercept))
    {
      fLogSevere("Units '{}' and '{}' are incompatible", from, to);
      return (false);
    }
  }

  // FIXME: these leaks don't hurt anything but probably make valgrind unhappy
  Key insert_key(strdup(from.c_str()), strdup(to.c_str()));

  converter_cache[insert_key] = setme;
  #endif // if HAVE_UDUNITS
  return (true);
} // Unit::getConverter

bool
Unit::isCompatibleUnit(const std::string& from,
  const std::string                     & to)
{
  UnitConverter tmp;

  return (getConverter(from, to, tmp));
}

bool
Unit::convert(const std::string& fromUnit,
  const std::string            & toUnit,
  double                       fromVal,
  double                       & toVal)
{
  #if HAVE_UDUNITS
  if (fromUnit == toUnit) {
    toVal = fromVal;
    return (true);
  }

  UnitConverter uc;

  if (!getConverter(fromUnit, toUnit, uc)) { return (false); }

  toVal = uc.value(fromVal);
  return (true);

  #else // if HAVE_UDUNITS
  fLogSevere("Not compiled with Udunits2 support, can't convert units!");
  return false;

  #endif // if HAVE_UDUNITS
}

double
Unit::UnitConverter::value(double d) const
{
  if (Constants::MissingData == d) { return (d); }

  return (slope * d + intercept);
}

double
Unit::value(const std::string& from,
  const std::string          & to,
  double                     val)
{
  double d(Constants::MissingData);

  convert(from, to, val, d);
  return (d);
}

std::ostream&
rapio::operator << (std::ostream& os, const Unit& u)
{
  os << u.value() << ' ' << u.unit();
  return (os);
}
