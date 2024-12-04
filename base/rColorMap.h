#pragma once

#include <rDataType.h>
#include <rColor.h>
#include <rURL.h>

#include <memory>
#include <map>

namespace rapio
{
/** Store a constant or linear/gradiant color bin directly. Was subclasees, but
 * this is much faster in practice. */
class ColorBin : public Data {
public:
  ColorBin(bool l, const std::string& label,
    double l_, double u_,
    unsigned char& r_, unsigned char& g_, unsigned char& b_, unsigned char& a_,
    unsigned char& r2_, unsigned char& g2_, unsigned char& b2_, unsigned char& a2_
  ) : myLabel(label), myIsLinear(l), l(l_), u(u_), r(r_), g(g_), b(b_), a(a_), r2(r2_), g2(g2_), b2(b2_), a2(a2_), d(std::abs(
        u_ - l_)){ };

  std::string myLabel;
  bool myIsLinear;
  double l;
  double u;
  unsigned char r;
  unsigned char b;
  unsigned char g;
  unsigned char a;
  unsigned char r2;
  unsigned char b2;
  unsigned char g2;
  unsigned char a2;
  double d;
};


/** Color map converts values to colors */
class ColorMap : public Data
{
public:

  /* Default constructor */
  ColorMap() : myMissingColor(Color(0, 0, 255, 255)), myUnavailableColor(Color(0, 255, 255, 255)){ } // FIXME: green for now to make mistake obvious

  // Static ability for getting colormaps, etc.

  /** Get/load a color map from a key */
  static std::shared_ptr<ColorMap>
  getColorMap(const std::string& key);

  /** Given a value, return data color for it.  This ignores special data values like missing, etc. and
   * treats those values as data, which you normally don't want. */
  virtual void
  getDataColor(double v, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const = 0;

  /** Given a value, return colors for it */
  virtual void
  getColor(double v, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const = 0;

  /** Set the missing color for colormap */
  void
  setMissingColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255)
  {
    myMissingColor = Color(r, g, b, a);
  }

  /** Set the unavailable color for colormap */
  void
  setUnavailableColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255)
  {
    myUnavailableColor = Color(r, g, b, a);
  }

  /** Set special colors from parsable strings */
  void
  setSpecialColorStrings(const std::string& missing, const std::string& unavailable);

  /** For the GUI experiment a JSON streamed color map format we can apply
   * client size using javascript. */
  virtual void toJSON(std::ostream&){ }

  /** For the GUI experiment a SVG color map */
  virtual void
  toSVG(std::ostream&, const std::string& units = "", const size_t width = 80, const size_t height = 600)
  { }

protected:
  friend class ConfigColorMap;

  /** Key that was used to get this color map */
  std::string myName;

  /** The missing value of colormap */
  Color myMissingColor;

  /** The unavailable value of colormap */
  Color myUnavailableColor;
};

/** A color map that tries to be more efficient and prepare for the web assembly equivalent */
class DefaultColorMap : public ColorMap
{
public:

  /** For the GUI experiment a JSON streamed color map format we can apply
   * client size using javascript. */
  virtual void
  toJSON(std::ostream&) override;

  /** Create SVG format */
  virtual void
  toSVG(std::ostream&, const std::string& units = "", const size_t width = 80, const size_t height = 600) override;

  /** Add a bin to us */
  virtual void
  addBin(bool linear, const std::string& label, double u, double l,
    unsigned char r_, unsigned char g_, unsigned char b_, unsigned char a_,
    unsigned char r2_, unsigned char g2_, unsigned char b2_, unsigned char a2_);

  /** Given a value, return data color for it.  This ignores special data values like missing, etc. and
   * treats those values as data, which you normally don't want. */
  virtual void
  getDataColor(double v, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const;

  /** Given a value, return colors for it */
  virtual void
  getColor(double v, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const;

protected:

  /** Upper bound of each color bin */
  std::vector<double> myUpperBounds;

  /** The bin information for each section of colormap */
  std::vector<ColorBin> myColorInfo;
};

class NullColorMap : public ColorMap
{
  /** Given a value, return data color for it.  This ignores special data values like missing, etc. and
   * treats those values as data, which you normally don't want. */
  virtual void
  getDataColor(double v, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const;

  /** Given a value, return colors for it */
  virtual void
  getColor(double v, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const;
};
}
