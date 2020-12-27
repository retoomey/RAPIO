#pragma once

#include <rUtility.h>
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
  {
    // Actually should be constructor init
    myLabel  = label;
    upper    = lower = u;
    myLinear = false;
    r1       = r2 = r_;
    g2       = g1 = g_;
    b2       = b1 = b_;
    a2       = a1 = a_;
  }

  ColorBin(const std::string& label, double u, double l, unsigned char r_, unsigned char g_, unsigned char b_,
    unsigned char a_,
    unsigned char r2_, unsigned char g2_, unsigned char b2_, unsigned char a2_)
  {
    myLabel  = label;
    upper    = u;
    lower    = l;
    myLinear = true;
    r1       = r_;
    r2       = r2_;
    g1       = g_;
    g2       = g2_;
    b1       = b_;
    b2       = b2_;
    a2       = a_;
    a2       = a2_;
  }

  void
  getColor(double value, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a)
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
class ColorMap : public Utility
{
public:
  // Should we link to datatype?  Or just a name?  Only certain datatypes
  // have the attribute for a color map, some may be different than others.
  // should a colormap function exist for ALL datatype objects?
  // DataType right now is just 'data' with a location and time reference...
  // but is this fundamental?  Could we generalize the usage of the time/
  // location within the datatype, allowing different 'ways' of indexing...

  // We need a 'cache' based on a key to colormap so we don't keep rereading it
  // or do we cache by caller.  Humm
  // std::map<std::string, ColorMap> myColormaps;

  // playing with ideas here
  // datatype --> bool haveColorMapName(std::string& name);
  //
  // There's reading a URL vs having config hunt for it.  We need to
  // generalize this I think.  There are cases for either one.
  // Could be use a special URL format for configuration hunting? lol
  // this could generalize it. Or a URL subsetting 'huntConfig = true'

  /** First attempt just read it */
  static std::shared_ptr<ColorMap>
  readColorMap(const URL& u);

  void
  getColor(double v, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a);

  // first hack
  std::map<double, ColorBin> myValues;
};
}
