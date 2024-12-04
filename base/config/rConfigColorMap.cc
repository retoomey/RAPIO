#include "rConfigColorMap.h"

#include "rStrings.h"
#include "rError.h"
#include "rIODataType.h"
#include "rColorMap.h"

#include <memory>

using namespace rapio;

std::map<std::string, std::shared_ptr<ColorMap> > ConfigColorMap::myColorMaps;

bool
ConfigColorMap::read1ColorConfig(const std::string& key,
  std::map<std::string, std::string>              & attributes)
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
ConfigColorMap::readW2ColorMap(const std::string& key,
  std::map<std::string, std::string>            & attributes)
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
    colormap = readW2ColorMap(find.toString());

    colormap->setSpecialColorStrings(attributes["missing"], attributes["unavailable"]);
  }
  return colormap;
}

std::shared_ptr<ColorMap>
ConfigColorMap::readParaColorMap(const std::string& key,
  std::map<std::string, std::string>              & attributes)
{
  std::shared_ptr<DefaultColorMap> colormap;
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

            colormap = std::make_shared<DefaultColorMap>();
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

              float upper = minvalue + ((x + 1.0f / 2.0f) * fullRange); // mapping -1 1 to minValue maxValue

              char buf[ 64 ];
              snprintf(buf, sizeof(buf), "%.1f", upper);

              colormap->addBin(false, buf, upper, lower,
                //  c1.r, c1.g, c1.b, c1.a,
                //		c2.r, c2.g, c2.b, c2.a);

                ((short) (r * 255.0f)),
                ((short) (g * 255.0f)),
                ((short) (b * 255.0f)),
                ((short) (1.0f * 255.0f)), // alpha 1 for moment
                                           // ((short) (o * 255.0f)), // alpha 1 for moment
                ((short) (r2 * 255.0f)),   //
                ((short) (g2 * 255.0f)),
                ((short) (b2 * 255.0f)),
                ((short) (1.0f * 255.0f)) // alpha 1 for moment
              );

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
ConfigColorMap::readW2ColorMap(const URL& url)
{
  std::shared_ptr<DefaultColorMap> colormap;
  auto xml = IODataType::read<PTreeData>(url.toString());

  try{
    // out = std::make_shared<ColorMap>(); GOOP
    colormap = std::make_shared<DefaultColorMap>();
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

      const bool single = (( c1 == c2) || ( finite(upper) == 0) || ( finite(lower) == 0) );
      colormap->addBin(!single, label, upper, lower, c1.r, c1.g, c1.b, c1.a, c2.r, c2.g, c2.b, c2.a);

      lower = upper;

      // Make it upper...
      // bins[upperBound] = bin;  so map from upperbound to 'bin' with color...
    }

    // FIXME: need to read the unit right? Conversion?
  }catch (const std::exception& e) {
    colormap = nullptr;
    LogSevere("Tried to read color map and failed: " << e.what() << "\n");
  }
  LogInfo("Loaded w2 colormap at " << url.toString() << "\n");
  return colormap;
} // ColorMap::readW2ColorMap

std::shared_ptr<ColorMap>
ConfigColorMap::getColorMap(const std::string& key)
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
    colormap->myName = key;
  } else {
    colormap = lookup->second;
  }
  return colormap;
} // ColorMap::getColorMap
