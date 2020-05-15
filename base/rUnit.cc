#include "rUnit.h"

#include "rConfig.h"
#include "rError.h"
#include "rConstants.h"

#include <map>
#include <cstring> // strcmp()
#include <cstdlib> // putenv()
#include <iostream>
#include <udunits.h>
// #include <dlfcn.h> // dlsym

using namespace rapio;

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
    LogSevere(
      "Unit '" << unit
               << "' not found in the udunits database file(s) in misc.\n");
    return (false);
  }

  unit_cache[unit] = setme;
  return (true);
}
}

void
Unit::initialize()
{
  // This should return a non-zero iff ut_read_xml from udunits2 is
  // available...

  /*  int *iptr;
   * iptr = (int *)dlsym(0, "ut_read_xml");
   * if (iptr == 0){  // FIXME: do we 'have' to have it? For now, yes
   *  LogSevere("Udunits2 failed to initialize...\n");
   *  exit(1);
   * }
   */

  // Udunits2 has xml configuration files
  URL url = Config::getConfigFile("misc/udunits2.xml");

  if (url.getPath() == "") {
    LogSevere("Udunits2 requires a misc/udunits2.xml file and others in your configuration\n");
    exit(1);
  }

  // Initialize environment to xml files and udunits
  Config::setEnvVar("UDUNITS2_XML_PATH", url.getPath());
  utInit("");
  LogInfo("UDUNITS initialized successfully\n");
}

bool
Unit::isValidUnit(const std::string& unit)
{
  utUnit u;

  return (getUtUnit(unit, u) != 0);
}

bool
Unit::getConverter(const std::string& from,
  const std::string                 & to,
  UnitConverter                     & setme)
{
  // Key holds char* rather than std::string because
  // lookup is 3x faster if we don't create new strings each time...
  // FIXME: Need to investigate this
  const Key lookup_key((char *)from.c_str(), (char *)to.c_str());

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
      LogSevere("Units '" << from << "' and '" << to << "' are incompatible\n");
      return (false);
    }
  }

  // FIXME: these leaks don't hurt anything but probably make valgrind unhappy
  Key insert_key(strdup(from.c_str()), strdup(to.c_str()));
  converter_cache[insert_key] = setme;
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
  if (fromUnit == toUnit) {
    toVal = fromVal;
    return (true);
  }

  UnitConverter uc;

  if (!getConverter(fromUnit, toUnit, uc)) { return (false); }

  toVal = uc.value(fromVal);
  return (true);
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
