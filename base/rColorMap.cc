#include "rColorMap.h"
#include "rColor.h"
#include "rError.h"
#include "rURL.h"
#include "rIODataType.h"
#include "rIOURL.h"
#include "rConfig.h"

using namespace rapio;

std::map<std::string, std::shared_ptr<ColorMap> > ColorMap::myColorMaps;

std::shared_ptr<ColorMap>
ColorMap::getColorMap(const std::string& key)
{
  std::shared_ptr<ColorMap> colormap;

  auto lookup = myColorMaps.find(key);
  if (lookup == myColorMaps.end()) {
    URL find = Config::getConfigFile("colormaps/" + key + ".xml");
    if (find != "") {
      colormap = ColorMap::readColorMap(find.toString());
    }
    if (colormap == nullptr) { // ALWAYS return a color map
      LogSevere("Linear color map used for " << key << "\n");
      colormap = std::make_shared<LinearColorMap>();
    }
    myColorMaps[key] = colormap;
  } else {
    colormap = lookup->second;
  }
  return colormap;
}

// Factory default color map read/creation
std::shared_ptr<ColorMap>
ColorMap::readColorMap(const URL& url)
{
  std::shared_ptr<ColorMap> out = std::make_shared<ColorMap>();
  auto xml = IODataType::read<PTreeData>(url.toString());

  try{
    auto map = xml->getTree()->getChild("colorMap");
    // lowestBound and highestBound is the old readAutomatic..
    // otherwise readGeneric...we'll do that first.
    // get children "colorBin".  Humm could be optional maaaybe?
    // get children "unit".  Optional maaaaaybe?
    // loop through colorBin elements....ok we can count first time

    // This is old read generic ability, which is a ton of color maps
    // but not all.
    auto bins = map.getChildren("colorBin");
    // LogSevere("We have " << bins.size() << " color bins in this file\n");
    double lower = -HUGE_VAL;
    double upper = HUGE_VAL;

    for (auto& b:bins) {
      // parseColorBin(b, currentUpper, nextUpper, aNEWBIN);

      // Get each 'bin' of color map.
      // ColorMap -------------------------------------------------------

      // Read the upperBound tag as a string, parse to numbers
      auto upperStr = b.getAttr("upperBound", std::string(""));
      //    double upper = 0.0;
      if (upperStr.empty()) {
        LogSevere("Bad or missing upperbound value in colorBin attribute\n");
        return nullptr;
      }
      if (upperStr == "infinity") {
        upper = HUGE_VAL;
      } else if (upperStr == "-infinity") {
        upper = -HUGE_VAL;
      } else {
        upper = atof(upperStr.c_str());
      }

      // colors
      auto colors = b.getChildren("color");

      if (colors.size() < 0) {
        LogSevere("Missing colors in colorBin\n");
        return nullptr;
      }
      // Color class, right? Or faster without it?
      Color c1, c2;
      if (colors.size() > 0) {
        // These will default to white if missing
        // I don't consider this super critical, since user will see it visually
        // and know to fix the color map
        std::string s;
        s    = colors[0].getAttr("r", (std::string) "0xFF");
        c1.r = strtol(s.c_str(), NULL, 0);
        s    = colors[0].getAttr("b", (std::string) "0xFF");
        c1.b = strtol(s.c_str(), NULL, 0);
        s    = colors[0].getAttr("g", (std::string) "0xFF");
        c1.g = strtol(s.c_str(), NULL, 0);
        s    = colors[0].getAttr("a", (std::string) "0xFF");
        c1.a = strtol(s.c_str(), NULL, 0);
        if (colors.size() > 1) {
          s    = colors[1].getAttr("r", (std::string) "0xFF");
          c2.r = strtol(s.c_str(), NULL, 0);
          s    = colors[1].getAttr("b", (std::string) "0xFF");
          c2.b = strtol(s.c_str(), NULL, 0);
          s    = colors[1].getAttr("g", (std::string) "0xFF");
          c2.g = strtol(s.c_str(), NULL, 0);
          s    = colors[1].getAttr("a", (std::string) "0xFF");
          c2.a = strtol(s.c_str(), NULL, 0);
        } else {
          c2 = c1;
        }
      }

      auto label = b.getAttr("name", std::string(""));
      std::string name;

      // If no name is given for bin, use the bound to make one
      if (label.empty()) { // This can be done better I think..
        char buf[ 64 ];
        if (c1 == c2) {
          snprintf(buf, sizeof(buf), "%.1f", upper);
        } else {
          snprintf(buf, sizeof(buf), "%.1f-%.1f", lower, upper);
        }
        name = buf;
      } else {
        name = label;
      }

      // const std::string nameAttr (binElement.attribute( "name" ));
      // bin = ColorMap::ColorBin( *color, *color2, lowerBound, upperBound, nameAttr);
      // Do we use a color bin object.  Hummm.
      // Let's use a stupid simple lookup at moment...
      // There's interpolation/linear vs just using upper bin.

      if (( c1 == c2) || ( finite(upper) == 0) || ( finite(lower) == 0) ) {
        out->myValues[upper] = ColorBin(label, upper, c1.r, c1.g, c1.b, c1.a);
      } else {
        out->myValues[upper] = ColorBin(label, upper, lower, c1.r, c1.g, c1.b, c1.a, c2.r, c2.g, c2.b, c2.a);
      }
      lower = upper;

      // Make it upper...
      // bins[upperBound] = bin;  so map from upperbound to 'bin' with color...
    }
    // FIXME: need to read the unit right? Conversion?

    // auto failz = map.getChildren("fail"); This will give zero size
  }catch (const std::exception& e) {
    LogSevere("Tried to read color map and failed: " << e.what() << "\n");
  }
  return out;
} // ColorMap::readColorMap

// Could we inline this?
// For loops not using a color object is a bit faster
void
ColorMap::getColor(double v,
  unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const
{
  // Maybe cache missing
  // if (v == Constants::MissingData){
  //  return myMissingColor;
  // }
  const std::map<double, ColorBin>::const_iterator i = myValues.upper_bound(v);

  if (i != myValues.end()) {
    i->second.getColor(v, r, g, b, a);
    // r = c.r; g = c.g; b = c.b; a = c.a;
  } else {
    // Missing in color map..
    r = 0;
    g = 0;
    b = 0;
    a = 255;
  }
}

void
LinearColorMap::getColor(double v,
  unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const
{
  if (v > 200) { v = 200; }
  if (v < -50.0) { v = -50.0; }
  float weight = (250 - v) / 250.0;
  r = weight;
  g = 0;
  b = 0;
  a = 255;
}
