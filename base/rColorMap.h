#pragma once

#include <rDataType.h>
#include <rColor.h>
#include <rURL.h>

#include <memory>
#include <map>

namespace rapio
{
/** Basic Color bin class interface */
class ColorBin : public Data
{
public:
  /** Default STL constructor */
  ColorBin(){ };

  /** All color bins return colors for a given value */
  virtual void
  getColor(double value, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const = 0;

  /** Destructor */
  virtual ~ColorBin(){ }
};

/** A Color bin that stores a single color for the bin range */
class SingleColorBin : public ColorBin
{
public:

  /** Create a single color bin */
  SingleColorBin(const std::string& label, double u, unsigned char r_, unsigned char g_, unsigned char b_,
    unsigned char a_) : myColor(r_, g_, b_, a_), myLabel(label)
  { }

  /** Return our color, note here we don't care about the value */
  virtual void
  getColor(double value, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const override
  {
    myColor.get(r, g, b, a);
  }

protected:
  /** The color this bin stores */
  Color myColor;

  /** The label for this bin */
  std::string myLabel;
};

/** A Color bin that linearally scales colors throughout the bin range */
class LinearColorBin : public ColorBin
{
public:

  /** Create a linear color bin which interpolates over the bin range */
  LinearColorBin(const std::string& label, double u, double l, unsigned char r_, unsigned char g_, unsigned char b_,
    unsigned char a_,
    unsigned char r2_, unsigned char g2_, unsigned char b2_, unsigned char a2_)
    : myLowerColor(r_, g_, b_, a_), myUpperColor(r2_, g2_, b2_, a2_), myLower(l), myUpper(u), myLabel(label)
  { }

  /** Return our color */
  virtual void
  getColor(double value, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const override
  {
    double wt = (value - myLower) / (myUpper - myLower);

    if (wt < 0) { wt = 0; } else if (wt > 1) { wt = 1; }
    const double nwt = (1.0 - wt);

    r = (unsigned char) (0.5 + nwt * myLowerColor.r + wt * myUpperColor.r);
    g = (unsigned char) (0.5 + nwt * myLowerColor.g + wt * myUpperColor.g);
    b = (unsigned char) (0.5 + nwt * myLowerColor.b + wt * myUpperColor.b);
    a = (unsigned char) (0.5 + nwt * myLowerColor.a + wt * myUpperColor.a);
  }

protected:
  /** The lower color of this bin */
  Color myLowerColor;

  /** The upper color of this bin */
  Color myUpperColor;

  /** The lower data value of this bin */
  double myLower;

  /** The upper data value of this bin */
  double myUpper;

  /** The label for this bin */
  std::string myLabel;
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

  /** Read color config for a single colormap record
   * We could fully cache the colormap configuration stuff, but this is
   * wasteful since most algorithms may never request a colormap or will request limited
   * numbers of colormaps.  So we do a fresh hunt for each and only cache the final colormap.
   * This gives up speed for the gain of RAM.  We could add another implementation ability
   * for something dealing with massive numbers of product/types.  Even so, the color maps
   * themselves do cache so we do this once per product type.
   */
  static bool
  read1ColorConfig(const std::string& key, std::map<std::string, std::string> & attributes);

  /** Read a W2 Color map */
  static std::shared_ptr<ColorMap>
  readW2ColorMap(const std::string& key, std::map<std::string, std::string>& attributes);

  /** Read a Paraview Color map */
  static std::shared_ptr<ColorMap>
  readParaColorMap(const std::string& key, std::map<std::string, std::string>& attributes);

  /** Read a W2 color map from a given URL */
  static std::shared_ptr<ColorMap>
  readW2ColorMap(const URL& u);

  /** Given a value, return data color for it.  This ignores special data values like missing, etc. and
   * treats those values as data, which you normally don't want. */
  virtual void
  getDataColor(double v, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const;

  /** Given a value, return colors for it */
  virtual void
  getColor(double v, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const;

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

  /** Delete the bins we store */
  ~ColorMap()
  {
    for (auto c:myColorBins) {
      delete c;
    }
  }

protected:

  /** Upper bound of each color bin */
  std::vector<double> myUpperBounds;

  /** Color bin for each upper bound */
  std::vector<ColorBin *> myColorBins;

  /** The missing value of colormap */
  Color myMissingColor;

  /** The unavailable value of colormap */
  Color myUnavailableColor;

private:
  /** Cache of keys to color maps */
  static std::map<std::string, std::shared_ptr<ColorMap> > myColorMaps;
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
