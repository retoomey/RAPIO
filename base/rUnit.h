#pragma once

#include <rConfig.h>

#include <iosfwd>
#include <string>
#include <string_view>

namespace rapio {
/** Simple check of udunit configuration files, udunits will actually load these.
 * We check for existance and update the udunit search path environment. */
class ConfigUnit : public ConfigType {
public:
  /** Virtual object from config map to static for clarity. */
  virtual bool
  readConfig(std::shared_ptr<PTreeData> d) override { return readSettings(d); }

  /** Introduce self to configuration */
  static void
  introduceSelf();

  /** Actual work of reading/checking settings */
  static bool
  readSettings(std::shared_ptr<PTreeData>);
};

/**
 * Convenience wrapper around udunits2.
 *
 */
class Unit {
public:


  /** Store slope and intercept used by Unit */
  class UnitConverter {
public:
    double slope;
    double intercept;
    UnitConverter() : slope(1), intercept(0){ }

    double
    value(double d) const;
  };

  /**
   * Get a UnitConverter that converts from one unit to another.
   */
  static bool
  getConverter(std::string_view from,
    std::string_view            to,
    UnitConverter               & setme);

  /** Convert a value using a UnitConverter */
  static double
  value(const UnitConverter& uc, double d)
  {
    return (uc.value(d));
  }

public:

  /* Test a unit string's validity */
  static void
  initialize();

  /* Test a unit string's validity */
  static bool
  isValidUnit(std::string_view unit);

  /* Test two units' compatability.
   * NOTE: this is as expensive as the conversion itself,
   * so don't call this before value() as a safeguard --
   * call the four-argument version of value() and check its
   * success flag return value. */
  static bool
  isCompatibleUnit(std::string_view from,
    std::string_view                to);

  /**
   * Convert a value from one unit to another.
   */
  static double
  value(std::string_view from,
    std::string_view     to,
    double               fromValue);

  /**
   * Convert a value from one unit to another.
   * Like value(), but returns a success flag to let callers
   * know whether or not the conversion was successful.
   */
  static bool
  convert(std::string_view from,
    std::string_view       to,
    double                 fromValue,
    double                 & setmeToValue);

public:

  Unit() : myValue(0){ }

  /** Build a Unit object given a value along with its unit name. */
  Unit(double v, std::string_view s) : myValue(v), myUnitStr(s){ }

  /**
   * Convert the current Unit's value into other units.
   * Like value(string), but returns a success flag to let
   * callers know whether or not the conversion was successful.
   */
  bool
  value(std::string_view units, double& setme) const
  {
    return (convert(myUnitStr, units, myValue, setme));
  }

  /** Return this Unit's value in its native units */
  double
  value() const
  {
    return (myValue);
  }

  /** Return this Unit's units. */
  std::string
  unit() const
  {
    return (myUnitStr);
  }

private:

  double myValue;

  std::string myUnitStr;
};

/** Provide nice, printed output. */
std::ostream&
operator << (std::ostream&,
  const rapio::Unit&);
}
