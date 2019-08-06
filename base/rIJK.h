#pragma once

#include <rData.h>
#include <rLLH.h>

// #include <iosfwd>

namespace rapio {
/**  Representation of a geometric vector in a 3-D cartesian space.
 *   The units of which are in kilometers
 */
class IJK : public Data {
public:

  /** Storage of coordinates */
  double x, y, z;

  /**  Initialize from argument list; the default is the zero
   *   vector.
   */
  IJK(double a = 0, double b = 0, double c = 0);

  /** Calculate IJK where it's delta between two LLH */
  IJK(const LLH& a, const LLH& b);

  /** Build from a reference point and a list of differences.
   *
   *  @param LLH&  reference point
   *  @param double&  difference in latitude
   *  @param double&  difference in longitude
   *  @param Length&  difference in height
   */
  IJK(
    const LLH    & ref,
    const double & del_lat,
    const double & del_lon,
    const Length & del_ht
  );

  /** Build from a reference point and a list of Lengths.
   *
   *  FIXME: "distance east" and "distance north" are ambiguous
   *  terms that should be precisely defined here.  For example,
   *  is "distance east" a distance along
   *
   *  (1) the surface of the earth,
   *
   *  (2) a sphere whose radius corresponds to the altitude of the
   *      reference point at the tail of the displacement vector,
   *
   *  (3) a sphere whose radius corresponds to the altitude of the
   *      point at the head of the displacement vector,
   *
   *  (4) some particular straight line, or
   *
   *  (5) something else?
   *
   *  @param LLH&     reference point
   *  @param Length&  distance east
   *  @param Length&  distance north
   *  @param Length&  difference in height
   */
  IJK(
    const LLH&,
    const Length&,
    const Length&,
    const Length&
  );

  /**  Negation operator.
   */
  friend IJK
  operator - (const IJK& v)
  {
    return (IJK(-v.x, -v.y, -v.z));
  }

  /**  Vector addition.
   */
  IJK
  operator + (const IJK& v) const
  {
    return (IJK(x + v.x, y + v.y, z + v.z));
  }

  /**  Vector subtraction.
   */
  IJK
  operator - (const IJK& v) const
  {
    return (IJK(x - v.x, y - v.y, z - v.z));
  }

  /**  Dot product.
   */
  double
  operator * (const IJK& v) const
  {
    return (x * v.x + y * v.y + z * v.z);
  }

  /**  Cross product.
   */
  IJK
  operator % (const IJK& v) const
  {
    return (IJK
           (
             y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x
           ));
  }

  /**  Right scalar multiplication.
   */
  IJK
  operator * (double f) const
  {
    return (IJK(x * f, y * f, z * f));
  }

  /**  Scalar division.
   */
  IJK
  operator / (double f) const
  {
    return (IJK(x / f, y / f, z / f));
  }

  /**  Scalar multiplicative assignment.
   */
  IJK&
  operator *= (double f)
  {
    x *= f;
    y *= f;
    z *= f;
    return (*this);
  }

  /**  Scalar divisive assignment.
   */
  IJK&
  operator /= (double f)
  {
    x /= f;
    y /= f;
    z /= f;
    return (*this);
  }

  /**  Additive assignment.
   */
  IJK&
  operator += (const IJK& v)
  {
    x += v.x;
    y += v.y;
    z += v.z;
    return (*this);
  }

  /**  Subtractive assignment.
   */
  IJK&
  operator -= (const IJK& v)
  {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return (*this);
  }

  /**  Return unit vector. */
  IJK
  unit() const;

  /**  Return norm (magnitude) of vector. */
  double
  norm() const;

  /**  Return the square of the norm of a vector. */
  double
  normSquared() const;
};

/**  Left scalar multiplication of a IJK. */
IJK
operator * (double, const IJK&);

/**  Print a nicely formatted string representing a IJK.
 *
 *   The components are presented as a comma-delimited list surrounded
 *   by square brackets, "[" and "]".
 */
std::ostream&
operator << (std::ostream&, const IJK&);
}
