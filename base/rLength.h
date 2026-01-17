#pragma once

#include <rData.h>

#include <iosfwd>
#include <string>
#include <rArith.h>

#include <fmt/format.h>

namespace rapio {
/**
 *  Representation of a physical length (distance).
 *
 *  @see Speed
 */
class Length : public Data {
private:

  double km; // kilometers

  Length(double k) : km(k){ }

public:

  /** The default Length is zero. */
  Length() : km(0){ }

  /** Build a Length object given a string as its unit. */
  Length(double      value,
    const std::string& unit);

  // -------------------------
  //   Named Constructors
  // -------------------------

  /** Interpret double as kilometers. */
  static Length
  Kilometers(double km)
  {
    return (Length(km));
  }

  /** Interpret double as meters. */
  static Length
  Meters(double m)
  {
    return (Kilometers(m / 1000.0));
  }

  /** Interpret double as centimeters. */
  static Length
  Centimeters(double cm)
  {
    return (Meters(cm / 100.0));
  }

  /** Interpret double as feet. */
  static Length
  Feet(double f)
  {
    return (Meters(f * 1200.0 / 3937.0));
  }

  /** Interpret double as kilofeet. */
  static Length
  Kilofeet(double kft)
  {
    return (Feet(kft * 1000.0));
  }

  /** Interpret double as inches. */
  static Length
  Inches(double in)
  {
    return (Feet(in / 12.0));
  }

  /** Interpret double as nautical miles. */
  static Length
  NauticalMiles(double nm)
  {
    return (Meters(nm * 1852.0));
  }

  /** Interpret double as statute miles. */
  static Length
  StatuteMiles(double sm)
  {
    return (Feet(sm * 5280.0));
  }

  // -----------------------
  //   Access Functions
  // -----------------------

  /* @unitStr such as kilometers, feet
   * @return value as double
   */
  double
  value(const std::string& unitStr);

  /** Return cenimeters. */
  double
  centimeters() const
  {
    return (km * 100000);
  }

  /** Return meters. */
  double
  meters() const
  {
    return (km * 1000);
  }

  /** Return kilometers. */
  double
  kilometers() const
  {
    return (km);
  }

  /** Return kilofeet. */
  double
  kiloFeet() const
  {
    return (km * (3937.0 / 1200.0));
  }

  /** Return feet. */
  double
  feet() const
  {
    return (kiloFeet() * 1000.0);
  }

  /** Return inches. */
  double
  inches() const
  {
    return (feet() * 12.0);
  }

  /** Return nautical miles. */
  double
  nauticalMiles() const
  {
    return (km / 1.852);
  }

  /** Return statute miles. */
  double
  statuteMiles() const
  {
    return (feet() / 5280);
  }

  /** Return degrees of latitude. */
  double
  degreesOfLatitude() const
  {
    return (km / 111.137);
  }

  // ---------------------------
  //   Operator Overloading
  // ---------------------------

  /** Provide nice, printed output. */
  friend std::ostream&
  operator << (std::ostream&,
    const Length&);

  Length             &
  operator += (const Length& l)
  {
    km += l.km;
    return (*this);
  }

  Length&
  operator -= (const Length& l)
  {
    km -= l.km;
    return (*this);
  }

  Length&
  operator *= (double d)
  {
    km *= d;
    return (*this);
  }

  Length&
  operator /= (double d)
  {
    km /= d;
    return (*this);
  }

  Length
  operator + (const Length& l) const
  {
    return (km + l.km);
  }

  Length
  operator - (const Length& l) const
  {
    return (km - l.km);
  }

  Length
  operator * (double d) const
  {
    return (km * d);
  }

  Length
  operator / (double d) const
  {
    return (km / d);
  }

  double
  operator / (const Length& l) const
  {
    return (km / l.km);
  }

  int
  compareTo(const Length& l) const
  {
    return (Arith::compare(km, l.km));
  }

  bool
  operator < (const Length& l) const
  {
    return (compareTo(l) < 0);
  }

  bool
  operator <= (const Length& l) const
  {
    return (compareTo(l) <= 0);
  }

  bool
  operator == (const Length& l) const
  {
    return (compareTo(l) == 0);
  }

  bool
  operator != (const Length& l) const
  {
    return (compareTo(l) != 0);
  }

  bool
  operator >= (const Length& l) const
  {
    return (compareTo(l) >= 0);
  }

  bool
  operator > (const Length& l) const
  {
    return (compareTo(l) > 0);
  }

  /** Negation operator. */
  friend Length
  operator - (const Length& a);

  /** Unary +. */
  friend const Length&
  operator + (const Length& a)
  {
    return (a);
  }

  /** left scalar multiplication. */
  friend Length
  operator * (double scale,
    const Length     & a);
};
}

/** Format library support, allows fLogInfo("Length {}", length) */
template <>
struct fmt::formatter<rapio::Length> {
  constexpr auto parse(fmt::format_parse_context& ctx){ return ctx.begin(); }

  template <typename FormatContext>
  auto
  format(const rapio::Length& l, FormatContext& ctx) const
  {
    return fmt::format_to(ctx.out(), "[{} km]", l.kilometers());
  }
};
