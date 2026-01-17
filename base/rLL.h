#pragma once

#include <rData.h>
#include <rConstants.h>

#include <fmt/format.h>

namespace rapio {
/** Store a latitude/longitude location */
class LL : public Data {
protected:

  /**  Latitude, [-90,90] degrees. */
  double myLatitudeDegs;

  /**  Longitude, (-180,180] degrees. */
  double myLongitudeDegs;

public:

  /**  Default constructor sets lat, lon to zero. */
  LL();

  /** Constuct a LL
   *
   *   @param lat  latitude in degrees
   *   @param lon  longitude in degrees
   */
  LL(const double& lat, const double& lon);

  /**  Return the latitude angle. */
  const double&
  getLatitudeDeg() const
  {
    return (myLatitudeDegs);
  }

  /** Set the latitude of this Location. */
  void
  setLatitudeDeg(const double& a)
  {
    myLatitudeDegs = a;
  }

  /**  Return the longitude angle. */
  const double&
  getLongitudeDeg() const
  {
    return (myLongitudeDegs);
  }

  /** Set the longitude of this Location. */
  void
  setLongitudeDeg(const double& a)
  {
    myLongitudeDegs = a;
  }

  /** Set a latitude longitude */
  void
  set(const double& lat, const double& lon)
  {
    myLatitudeDegs  = lat;
    myLongitudeDegs = lon;
  }

  /**  Print a nicely formatted string for this LL. */
  friend std::ostream&
  operator << (std::ostream&,
    const LL&);

  /**  Determine whether this LL contains reasonable values. */
  bool
  isValid() const;

  /** @return the Great Circle Distance to another location, on the
   * surface of the earth. */
  LengthKMs
  getSurfaceDistanceToKMs(const LL& b) const;
};
}

/** Format library support for LL */
template <>
struct fmt::formatter<rapio::LL> {
  constexpr auto
  parse(fmt::format_parse_context& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto
  format(const rapio::LL& loc, FormatContext& ctx) const
  {
    return fmt::format_to(
      ctx.out(),
      "[lat={:.8g},lon={:.8g}]",
      loc.getLatitudeDeg(),
      loc.getLongitudeDeg()
    );
  }
};
