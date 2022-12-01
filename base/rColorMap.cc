#include "rColorMap.h"
#include "rColor.h"
#include "rError.h"
#include "rURL.h"
#include "rIODataType.h"
#include "rIOURL.h"
#include "rConfig.h"
#include "rStrings.h"

using namespace rapio;

std::map<std::string, std::shared_ptr<ColorMap> > ColorMap::myColorMaps;

void
ColorMap::setSpecialColorStrings(const std::string& missing, const std::string& unavailable)
{
  unsigned char r, g, b, a;

  if (!Color::RGBStringToColor(missing, r, g, b, a)) {
    // Set the missing color to what the bins currently return
    getDataColor(Constants::MissingData, r, g, b, a);
  }
  setMissingColor(r, g, b, a);

  if (!Color::RGBStringToColor(unavailable, r, g, b, a)) {
    // Set the unavailable color to what the bins currently return
    getDataColor(Constants::DataUnavailable, r, g, b, a);
  }
  setUnavailableColor(r, g, b, a);
}

bool
ColorMap::read1ColorConfig(const std::string& key,
  std::map<std::string, std::string>        & attributes)
{
  const std::string config = "colormaps/colorconfig.xml";
  bool found = false;

  const URL find = Config::getConfigFile(config);
  auto xml       = IODataType::read<PTreeData>(find.toString());

  if (find.empty()) {
    LogSevere("Requested color information such as color map and we could not locate " + config + "\n");
  } else {
    try{
      if (xml != nullptr) {
        auto topTree = xml->getTree()->getChild("colorconfig");
        auto maps    = topTree.getChildOptional("colormaps");
        if (maps != nullptr) {
          auto cm = maps->getChildren("colormap");
          for (auto c: cm) {
            const auto ckey = c.getAttr("key", std::string(""));
            // So basically if we find the key we stop here.
            if (key == ckey) {
              attributes = c.getAttrMap();
              found      = true;
              break;
            }
          }
        } else {  }
      }
    }catch (const std::exception& e) {
      LogSevere("Exception processing " << config << " " << e.what() << "\n");
    }
  }
  return found;
} // ColorMap::read1ColorConfig

std::shared_ptr<ColorMap>
ColorMap::getColorMap(const std::string& key)
{
  std::shared_ptr<ColorMap> colormap;

  // Check the color map cache...
  auto lookup = myColorMaps.find(key);

  if (lookup == myColorMaps.end()) {
    // ... if not found, look for the color configuration information
    std::map<std::string, std::string> attributes;

    bool foundConfig = read1ColorConfig(key, attributes);
    if (foundConfig) {
      // Read the type of color map and check we can handle it
      std::string type = attributes["type"];
      if (type.empty()) { type = "w2"; }
      // FIXME: lowercase force?
      if (!((type == "w2") || (type == "para"))) {
        LogSevere("Unrecognized color map type '" << type << "', trying 'w2'\n");
        type = "w2";
      }

      // Delagate to different creation functions...
      if (type == "w2") {
        colormap = readW2ColorMap(key, attributes);
      } else if (type == "para") {
        colormap = readParaColorMap(key, attributes);
      }
    }

    // If anything failed use my debugging high contrast all data map
    if (colormap == nullptr) { // ALWAYS return a color map
      LogSevere("Using testing high contrast ColorMap for key " << key << "\n");
      colormap = std::make_shared<NullColorMap>();
    }

    // Cache this color map so we don't hunt for it again
    myColorMaps[key] = colormap;
  } else {
    colormap = lookup->second;
  }
  return colormap;
} // ColorMap::getColorMap

std::shared_ptr<ColorMap>
ColorMap::readW2ColorMap(const std::string& key,
  std::map<std::string, std::string>      & attributes)
{
  std::shared_ptr<ColorMap> colormap;
  const std::string rootpath = "colormaps/"; // FIXME: const?

  // Use filename attribute if we have it, else use the key
  std::string filename;

  filename = attributes["filename"];
  if (!filename.empty()) {
    filename = rootpath + filename;
  } else {
    // Fall back to trying the key as a W2 colormap lookup name...
    filename = rootpath + key + ".xml";
  }

  // Now try to read it
  URL find = Config::getConfigFile(filename);

  if (!find.empty()) {
    colormap = ColorMap::readW2ColorMap(find.toString());

    colormap->setSpecialColorStrings(attributes["missing"], attributes["unavailable"]);
  }
  return colormap;
}

std::shared_ptr<ColorMap>
ColorMap::readParaColorMap(const std::string& key,
  std::map<std::string, std::string>        & attributes)
{
  std::shared_ptr<ColorMap> colormap;
  const std::string rootpath = "colormaps/"; // FIXME: const?

  // Use filename attribute if we have it, else we fail
  std::string filename;

  filename = attributes["filename"];
  if (!filename.empty()) {
    filename = rootpath + filename;
  } else {
    LogSevere("Paraview colormap requires a filename to a map\n");
    return nullptr;
  }

  // The 'name' key to match in file
  const std::string name = attributes["name"];

  // Now try to read it
  URL find = Config::getConfigFile(filename);

  if (find.empty()) {
    LogSevere("Cannot read Paraview color map file " + filename + "\n");
  } else {
    try{
      auto xml = IODataType::read<PTreeData>(find.toString());
      if (xml != nullptr) {
        auto topTree = xml->getTree()->getChild("doc");
        auto cm      = topTree.getChildren("ColorMap");
        for (auto& c: cm) {
          const auto pname = c.getAttr("name", std::string(""));
          if (pname == name) {
            // Getting data value range if provided
            float minvalue = -HUGE_VAL;
            std::string l  = attributes["lowdata"];
            if (!l.empty()) {
              minvalue = std::stof(l); // FIXME: exception?
            }
            float maxvalue = +HUGE_VAL; // this low or maybe higher?
            std::string h  = attributes["highdata"];
            if (!h.empty()) {
              maxvalue = std::stof(h); // FIXME: exception?
            }
            if (maxvalue < minvalue) {
              float tempvalue = minvalue;
              minvalue = maxvalue;
              maxvalue = tempvalue;
            }

            colormap = std::make_shared<ColorMap>();
            //   const auto pspace = c.getAttr("space", std::string(""));
            // Ok I 'think' for paraview we stretch percentage from -1 to 1 over X which we match to our data values.
            // So I will map -1 to 1 to minvalue to maxvalue on the data
            // We'll use custom missing/unavailable for outside.  This is probably 'close' enough
            float fullRange = maxvalue - minvalue; // 100% range so x * this   -1 = -30, 1 = 110;

            bool firstPoint = true;
            float x2, o2, r2, g2, b2; // The old values or 'left' of bin
            float lower;
            auto points = c.getChildren("Point");
            for (auto& p: points) {
              const auto x = p.getAttr("x", float(1.0)); // -1 to 1 over the values
              const auto o = p.getAttr("o", float(1.0));
              const auto r = p.getAttr("r", float(1.0));
              const auto g = p.getAttr("g", float(1.0));
              const auto b = p.getAttr("b", float(1.0));
              if (firstPoint) {
                // Keep the left side
                x2 = x;
                o2 = o;
                r2 = r;
                g2 = g;
                b2 = b;
                // lower = minvalue+((x+1.0f/2.0f)*fullRange);
                lower      = minvalue; // assuming x = -1 is this always true? lol  I don't know this format well
                firstPoint = false;
                continue;
              }
              // std::cout << "POINT: " << x << ", " << o << " , " << r << " , " << g << " , " << b << "\n";

              float upper       = minvalue + ((x + 1.0f / 2.0f) * fullRange); // mapping -1 1 to minValue maxValue
              ColorBin * newOne = new LinearColorBin("X", upper, lower,
                  ((short) (r * 255.0f)),
                  ((short) (g * 255.0f)),
                  ((short) (b * 255.0f)),
                  ((short) (1.0f * 255.0f)), // alpha 1 for moment
                                             // ((short) (o * 255.0f)), // alpha 1 for moment
                  ((short) (r2 * 255.0f)),   //
                  ((short) (g2 * 255.0f)),
                  ((short) (b2 * 255.0f)),
                  ((short) (1.0f * 255.0f)) // alpha 1 for moment
                                            // ((short) (o2 * 255.0f)) // alpha 1 for moment
              );
              colormap->myUpperBounds.push_back(upper);
              colormap->myColorBins.push_back(newOne);
              // Keep the left side
              x2    = x;
              o2    = o;
              r2    = r;
              g2    = g;
              b2    = b;
              lower = upper;
            }

            colormap->setSpecialColorStrings(attributes["missing"], attributes["unavailable"]);

            break;
          }
        }
      }
    }catch (const std::exception& e) {
      LogSevere("Exception processing XML " << filename << " " << e.what() << "\n");
    }
  }

  LogInfo("Loaded para colormap with key " << key << "\n");
  return colormap;
} // ColorMap::readParaColorMap

std::shared_ptr<ColorMap>
ColorMap::readW2ColorMap(const URL& url)
{
  std::shared_ptr<ColorMap> out;
  auto xml = IODataType::read<PTreeData>(url.toString());

  try{
    out = std::make_shared<ColorMap>();
    auto map = xml->getTree()->getChild("colorMap");

    // This is old read generic ability, which is a ton of color maps
    // but not all.
    auto bins    = map.getChildren("colorBin");
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

      // There's interpolation/linear vs just using upper bin.
      out->myUpperBounds.push_back(upper);
      if (( c1 == c2) || ( finite(upper) == 0) || ( finite(lower) == 0) ) {
        out->myColorBins.push_back(new SingleColorBin(label, upper, c1.r, c1.g, c1.b, c1.a));
      } else {
        out->myColorBins.push_back(new LinearColorBin(label, upper, lower, c1.r, c1.g, c1.b, c1.a, c2.r, c2.g, c2.b,
          c2.a));
      }
      lower = upper;

      // Make it upper...
      // bins[upperBound] = bin;  so map from upperbound to 'bin' with color...
    }

    // FIXME: need to read the unit right? Conversion?
  }catch (const std::exception& e) {
    out = nullptr;
    LogSevere("Tried to read color map and failed: " << e.what() << "\n");
  }
  LogInfo("Loaded w2 colormap at " << url.toString() << "\n");
  return out;
} // ColorMap::readW2ColorMap

void
ColorMap::getDataColor(double v,
  unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const
{
  // This assumes sorted
  auto i = std::upper_bound(myUpperBounds.begin(), myUpperBounds.end(), v);

  if (i != myUpperBounds.end()) {
    myColorBins[i - myUpperBounds.begin()]->getColor(v, r, g, b, a);
  } else {
    // Ok this is a value > the last upper bound.  So we just pin to the last bin
    myColorBins[myColorBins.size() - 1]->getColor(v, r, g, b, a);
  }
}

void
ColorMap::getColor(double v,
  unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const
{
  // Use a custom missing value here. Typically missing is the most called
  if (v == Constants::MissingData) {
    myMissingColor.get(r, g, b, a);
    return;
  }
  if (v == Constants::DataUnavailable) {
    myUnavailableColor.get(r, g, b, a);
    return;
  }

  // This assumes sorted
  auto i = std::upper_bound(myUpperBounds.begin(), myUpperBounds.end(), v);

  if (i != myUpperBounds.end()) {
    myColorBins[i - myUpperBounds.begin()]->getColor(v, r, g, b, a);
  } else {
    // Ok this is a value > the last upper bound.  So we just pin to the last bin
    myColorBins[myColorBins.size() - 1]->getColor(v, r, g, b, a);
  }
}

void
NullColorMap::getDataColor(double v, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const
{
  // a test grid
  if (v == Constants::MissingData) {
    r = 255;
    g = 0;
    b = 0;
    a = 255;
  } else if (v == Constants::DataUnavailable) {
    r = 0;
    g = 0;
    b = 255;
    a = 255;
  } else if (v == Constants::RangeFolded) {
    r = 255;
    g = 255;
    b = 0;
    a = 255;
  } else {
    r = 0;
    g = 255;
    b = 0;
    a = 255;
  }
}

void
NullColorMap::getColor(double v, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const
{
  // a test grid
  if (v == Constants::MissingData) {
    r = 255;
    g = 0;
    b = 0;
    a = 255;
  } else if (v == Constants::DataUnavailable) {
    r = 0;
    g = 0;
    b = 255;
    a = 255;
  } else if (v == Constants::RangeFolded) {
    r = 255;
    g = 255;
    b = 0;
    a = 255;
  } else {
    r = 0;
    g = 255;
    b = 0;
    a = 255;
  }
}
