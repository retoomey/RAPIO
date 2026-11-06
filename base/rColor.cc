#include "rColor.h"
#include <charconv>
#include <cstdio>
#include <string>
#include <string_view>
#include <iostream>
#include "rError.h"

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
  return fmt::format("#{:02x}{:02x}{:02x}", r, g, b);
}

bool
Color::RGBStringToColor(std::string_view s,
  unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a)
{
  bool failed = false;

  if (!s.empty()) {
    const size_t l = s.length();
    // "#FF00FF" and "#FF00FF00" styles
    if (l > 0) {
      // Marked color format
      if (s[0] == '#') {
        auto parseHex = [](std::string_view sub, unsigned char& out) -> bool {
            unsigned int val = 0;

            auto [ptr, ec] = std::from_chars(sub.data(), sub.data() + sub.size(), val, 16);
            out = static_cast<unsigned char>(val);
            return (ec != std::errc()) || (ptr != sub.data() + sub.size());
          };
        if (l >= 7) { // "#FF00FF"
          failed |= parseHex(s.substr(1, 2), r);
          failed |= parseHex(s.substr(3, 2), g);
          failed |= parseHex(s.substr(5, 2), b);
          if (l >= 9) {
            failed |= parseHex(s.substr(7, 2), a);
          } else {
            a = 255;
          }
        } else {
          failed = true;
        }
      } else {
        failed = true;
      }
    }
  } else {
    failed = true;
  }
  return !failed;
} // Color::RGBStringToColor

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
