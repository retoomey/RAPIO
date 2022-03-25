#pragma once

#include <rLL.h>
#include <rLength.h>
#include <rConstants.h>

#include <iosfwd>

namespace rapio {
class LL;
class IJK;

/** Store a latitude/longitude/height */
class LLH : public LL {
protected:

  /** Height above mean sea level. */
  // Length myHeightKMs;
  LengthKMs myHeightKMs;

public:

  /**  Default constructor sets lat, lon, and height to zero. */
  LLH();

  /** Build location from latitude, longitude and height.
   *
   *  @param latitude   latitude degrees
   *  @param longitude   longitude degrees
   *  @param height  height in kilometers
   */
  LLH(const AngleDegs& latitude,
    const AngleDegs  & longitude,
    const LengthKMs  & height);

  /** Build Location from LatLon and height.
   *
   *  @param a  angular location about earth center
   *  @param height  height above earth surface
   */
  LLH(const LL& a, const LengthKMs& height);

  /** Do-nothing destructor. */
  ~LLH(){ }

  /** Set the height of this Location. */
  void
  setHeightKM(LengthKMs d)
  {
    myHeightKMs = d;
  }

  /** Replace values without making new object */
  void
  set(const AngleDegs& lat, const AngleDegs& lon, const LengthKMs& l)
  {
    myLatitudeDegs  = lat;
    myLongitudeDegs = lon;
    myHeightKMs     = l;
  }

  /** @return the height (const Length& as return type) */
  const LengthKMs
  getHeightKM() const
  {
    return (myHeightKMs);
  }

  /** Add a Displacement to get a new Location. */
  LLH
  operator + (const IJK&) const;

  /** Subtract a Displacement to get a new Location. */
  LLH
  operator - (const IJK&) const;

  /** Add a IJK to get a new Location. */
  LLH     &
  operator += (const IJK&);

  /** Subtract a IJK to get a new Location. */
  LLH     &
  operator -= (const IJK&);

  /** Subtract another Location and return a IJK */
  IJK
  operator - (const LLH&) const;

  /** Comparison of locations */
  bool
  operator == (const LLH& l) const
  {
    return ((myLatitudeDegs == l.myLatitudeDegs) &&
           (myLongitudeDegs == l.myLongitudeDegs) &&
           (myHeightKMs == l.myHeightKMs));
  }
};

/**  Print longitude and latitude. */
std::ostream&
operator << (std::ostream&,
  const rapio::LLH&);
}
