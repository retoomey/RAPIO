#pragma once

#include <rData.h>

namespace rapio
{
/** Store a color object */
class Color : public Data
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
