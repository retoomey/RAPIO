#pragma once

#include <rLL.h>
#include <rLength.h>

#include <iosfwd>

namespace rapio {
class LL;
class IJK;

/** Store a latitude/longitude/height */
class LLH : public LL {
protected:

  /** Height above mean sea level. */
  // Length myHeight;
  double myHeight;

public:

  /**  Default constructor sets lat, lon, and height to zero. */
  LLH();

  /** Build location from latitude, longitude and height.
   *
   *  @param const_double&   latitude degrees
   *  @param const_double&   longitude degrees
   *  @param const_Length&  height in kilometers
   */
  LLH(const double&,
    const double&,
    const double&);

  /** Build Location from LatLon and height.
   *
   *  @param LL&  angular location about earth center
   *  @param Length&  height above earth surface
   */
  LLH(const LL&, const double&);

  /** Do-nothing destructor. */
  ~LLH(){ }

  /** Set the height of this Location. */
  // void setHeight(const Length& d)
  void
  setHeight(double d)
  {
    myHeight = d;
  }

  /** Replace values without making new object */
  void
  set(const double& lat, const double& lon, const double& l)
  {
    myLatitudeDegs  = lat;
    myLongitudeDegs = lon;
    myHeight        = l;
  }

  /** @return the height (const Length& as return type) */
  const double&
  getHeightKM() const
  {
    return (myHeight);
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
           (myHeight == l.myHeight));
  }
};

/**  Print longitude and latitude. */
std::ostream&
operator << (std::ostream&,
  const rapio::LLH&);
}
