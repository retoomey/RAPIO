#include "rColor.h"
#include <cstdio>
#include <string>
#include <iostream>

namespace rapio
{
Color::Color(
  unsigned char red,
  unsigned char green,
  unsigned char blue,
  unsigned char alpha
)
  :   r(red), g(green), b(blue), a(alpha){ }

std::string
Color::getRGBColorString() const
{
  char buf[8];

  snprintf(buf, sizeof(buf), "#%02x%02x%02x", r, g, b);
  return buf;
}

double
Color::brightness() const
{
  // Charles Kerr's contrast with white/black formula for our text border
  // CIE luminance from linear red, green, blue
  return ( (r * 0.2125) + (g * 0.7154) + (b * 0.0721));
}

Color
Color::getContrastingColor() const
{
  // Given a color, return a good contrasting color - either white or black.
  if (brightness() < 128.0) {
    // The color is dark, return white
    return Color(0xFF, 0xFF, 0xFF);
  } else {
    // The color is light, return black
    return Color(0x00, 0x00, 0x00);
  }
}

std::ostream&
operator << (std::ostream& os, const Color& c)
{
  return (os
         << "<color> "
         << +c.r << " " << +c.g << " "
         << +c.b << " " << +c.a << " </color>");
}

bool
operator == (const Color& a, const Color& o)
{
  return ( a.r == o.r &&
         a.g == o.g &&
         a.b == o.b &&
         a.a == o.a );
}
}
