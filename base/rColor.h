#pragma once

#include <string>

namespace rapio
{
/** Store a color object
 *
 * @ingroup rapio_data
 * @brief Stores a color such as RGB.
 * */
class Color
{
public:
  /** Construct a color with defaults */
  Color(
    unsigned char red     = 255,
    unsigned char green   = 255,
    unsigned char blue    = 255,
    unsigned char opacity = 255
  );
  /** Destroy a color */
  virtual ~Color(){ }

  /** Obtain a hexadecimal string representing the RGB color. */
  std::string
  getRGBColorString() const;

  /** Convert hexadecimal string into RGB color values */
  static bool
  RGBStringToColor(const std::string& s,
    unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a);

  /** Get color into values */
  inline void
  get(unsigned char& red, unsigned char& green, unsigned char& blue, unsigned char& alpha) const
  {
    red   = r;
    green = g;
    blue  = b;
    alpha = a;
  }

  /** Get color brightness between 0 and 255 (0 = black, 255 = white). */
  double
  brightness() const;

  /** Get a contrasting color - either white or black. */
  Color
  getContrastingColor() const;

  /** Two colors are equal if their rgba values are all the same. */
  friend bool
  operator == (const Color& a, const Color& b);

  // Allow direct access to color values vs get/set here,
  // there are no side effects other than changing the color

  /** Red component. */
  unsigned char r;

  /** Green component. */
  unsigned char g;

  /** Blue component. */
  unsigned char b;

  /** Alpha (opacity) component. */
  unsigned char a;
};

/** Write a Color object to a stream. */
std::ostream&
operator << (std::ostream&, const Color&);
}
