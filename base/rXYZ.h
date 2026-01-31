#pragma once

#include <rIJK.h>
#include <rLLH.h>

#include <fmt/format.h>

namespace rapio {
class LLH;

/**  Representation of a geometric point in a 3-D cartesian space
 * where we center 0,0,0 at the center of a pure spherical earth.
 *   This is our base projection used for lat lon height space.
 *
 *   @ingroup rapio_data
 *   @brief Stores Cartesian 3D X,Y,Z coordinates.
 */
class XYZ {
public:

  /** Storage of coordinates */
  double x, y, z;

  /**  Initialize from argument list; the default is a point at the
   *   origin.
   */
  XYZ(double a = 0, double b = 0, double c = 0);

  /** Project to a XYZ from a LLH passed in */
  XYZ(const LLH& in);

  /** Reverse project to a LLH from us */
  LLH
  getLocation() const;

  /**  Subtraction operator for XYZs returns a IJK. */
  IJK
  operator - (const XYZ& p) const;

  /**  Adding a IJK to a XYZ results in another XYZ. */
  XYZ
  operator + (const IJK& v) const;

  /**  Subtracting a IJK from a XYZ results in another XYZ. */
  XYZ
  operator - (const IJK& v) const;

  /**  Additive assignment of a IJK to a XYZ. */
  XYZ &
  operator += (const IJK& v);

  /**  Subtractive assignment of a IJK from a Cpoint. */
  XYZ &
  operator -= (const IJK& v);
};

/** Stream print for a XYZ */
std::ostream&
operator << (std::ostream&,
  const XYZ&);
}

/** Format library support, allows fLogInfo("XYZ {}", xyz) */
template <>
struct fmt::formatter<rapio::XYZ> {
  constexpr auto parse(fmt::format_parse_context& ctx){ return ctx.begin(); }

  template <typename FormatContext>
  auto
  format(const rapio::XYZ& p, FormatContext& ctx) const
  {
    return fmt::format_to(ctx.out(), "({},{},{})", p.x, p.y, p.z);
  }
};
