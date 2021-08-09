#pragma once

#include <rDataType.h>
#include <rColor.h>
#include <rURL.h>

#include <memory>

namespace rapio
{
/** Simple structure for holding a color bin */
class ColorBin : public Data
{
public:
  ColorBin(){ }; // STL wants for map, but we'll never call it directly

  ColorBin(const std::string& label, double u, unsigned char r_, unsigned char g_, unsigned char b_, unsigned char a_)
    : myLinear(false), r1(r_), g1(g_), b1(b_), a1(a_), r2(r_), g2(g_), b2(b_), a2(a_), upper(u), lower(u),
    myLabel(label)
  { }

  ColorBin(const std::string& label, double u, double l, unsigned char r_, unsigned char g_, unsigned char b_,
    unsigned char a_,
    unsigned char r2_, unsigned char g2_, unsigned char b2_, unsigned char a2_)
    : myLinear(true), r1(r_), g1(g_), b1(b_), a1(a_), r2(r2_), g2(g2_), b2(b2_), a2(a2_), upper(u), lower(l), myLabel(
      label)
  { }

  void
  getColor(double value, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const
  {
    if (myLinear) {
      double wt = (value - lower) / (upper - lower);
      if (wt < 0) { wt = 0; } else if (wt > 1) { wt = 1; }
      const double nwt = (1.0 - wt);
      r = (unsigned char) (0.5 + nwt * r1 + wt * r2);
      g = (unsigned char) (0.5 + nwt * g1 + wt * g2);
      b = (unsigned char) (0.5 + nwt * b1 + wt * b2);
      a = (unsigned char) (0.5 + nwt * a1 + wt * a2);
    } else {
      r = r1;
      g = g1;
      b = b1;
      a = a1;
    }
  }

  bool myLinear;
  unsigned char r1;
  unsigned char g1;
  unsigned char b1;
  unsigned char a1;
  unsigned char r2;
  unsigned char g2;
  unsigned char b2;
  unsigned char a2;
  double upper;
  double lower;
  std::string myLabel;
};

/** Color map converts values to colors */
class ColorMap : public DataType
{
public:

  // Static ability for getting colormaps, etc.

  /** Get cached color map from a key */
  static std::shared_ptr<ColorMap>
  getColorMap(const std::string& key);

  /** First attempt just read it */
  static std::shared_ptr<ColorMap>
  readColorMap(const URL& u);

  virtual void
  getColor(double v, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const;

  // first hack
  std::map<double, ColorBin> myValues;

  // We need a 'cache' based on a key to colormap so we don't keep rereading it
  // or do we cache by caller.  Humm
private:
  static std::map<std::string, std::shared_ptr<ColorMap> > myColorMaps;
};

class LinearColorMap : public ColorMap
{
  virtual void
  getColor(double v, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const override;
};
}
