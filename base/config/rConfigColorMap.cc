#include "rConfigColorMap.h"

#include "rStrings.h"
#include "rError.h"
#include "rIODataType.h"
#include "rColorMap.h"

#include <memory>
#include <cmath>

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
    fLogSevere("Requested color information such as color map and we could not locate ", config);
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
      fLogSevere("Exception processing {} {}", config, e.what());
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
    std::cout << "readW2ColorMap has been called" << "\n";
    colormap = readW2ColorMap(find.toString());

    colormap->setSpecialColorStrings(attributes["missing"], attributes["unavailable"]);
  }
  return colormap;
}

std::shared_ptr<ColorMap>
ConfigColorMap::readParaColorMap(const std::string& key,
  std::map<std::string, std::string>              & attributes)
{
  const std::string rootpath = "colormaps/";
  std::string filename       = attributes["filename"];

  if (!filename.empty()) {
    filename = rootpath + filename;
  } else {
    fLogSevere("Paraview colormap requires a filename to a map");
    return nullptr;
  }

  URL find = Config::getConfigFile(filename);

  if (!find.empty()) {
    // Delegate to the URL overload
    return readParaColorMap(find, attributes);
  }

  fLogSevere("Cannot read Paraview color map file {}", filename);
  return nullptr;
}

std::shared_ptr<ColorMap>
ConfigColorMap::readParaColorMap(const URL& find,
  std::map<std::string, std::string>      & attributes)
{
  std::shared_ptr<DefaultColorMap> colormap;
  const std::string name = attributes["name"];

  try {
    auto xml = IODataType::read<PTreeData>(find.toString());
    if (xml != nullptr) {
      auto topTree = xml->getTree()->getChild("doc");
      auto cm      = topTree.getChildren("ColorMap");

      for (auto& c: cm) {
        const auto pname = c.getAttr("name", std::string(""));
        if (pname == name) {
          float minvalue = -HUGE_VAL;
          std::string l  = attributes["lowdata"];
          if (!l.empty()) { minvalue = std::stof(l); }

          float maxvalue = +HUGE_VAL;
          std::string h  = attributes["highdata"];
          if (!h.empty()) { maxvalue = std::stof(h); }

          if (maxvalue < minvalue) { std::swap(minvalue, maxvalue); }

          colormap = std::make_shared<DefaultColorMap>();
          float fullRange = maxvalue - minvalue;
          bool firstPoint = true;

          float prev_x, prev_r, prev_g, prev_b, prev_o;
          float lower;

          auto points = c.getChildren("Point");
          for (auto& p: points) {
            const auto x = p.getAttr("x", float(1.0));
            const auto o = p.getAttr("o", float(1.0));
            const auto r = p.getAttr("r", float(1.0));
            const auto g = p.getAttr("g", float(1.0));
            const auto b = p.getAttr("b", float(1.0));

            if (firstPoint) {
              prev_x = x;
              prev_o = o;
              prev_r = r;
              prev_g = g;
              prev_b = b;
              // FIX: Just multiply x by the range! No weird fractions.
              lower      = minvalue + (x * fullRange);
              firstPoint = false;
              continue;
            }

            float upper     = minvalue + (x * fullRange);
            std::string buf = fmt::format("{:.1f}", upper);

            // FIX: Linear is true, and lower colors (prev) are passed before upper colors (current)
            colormap->addBin(true, buf, upper, lower,
              ((short) (prev_r * 255.0f)), ((short) (prev_g * 255.0f)), ((short) (prev_b * 255.0f)),
              ((short) (prev_o * 255.0f)),
              ((short) (r * 255.0f)), ((short) (g * 255.0f)), ((short) (b * 255.0f)), ((short) (o * 255.0f))
            );

            prev_x = x;
            prev_o = o;
            prev_r = r;
            prev_g = g;
            prev_b = b;
            lower  = upper;
          }
          colormap->setSpecialColorStrings(attributes["missing"], attributes["unavailable"]);
          break;
        }
      }
    }
  } catch (const std::exception& e) {
    fLogSevere("Exception processing XML {} {}", find.toString(), e.what());
  }

  return colormap;
} // ConfigColorMap::readParaColorMap

// Function for reading in .pal files
std::shared_ptr<ColorMap>
ConfigColorMap::readPalColorMap(const std::string& key,
  std::map<std::string, std::string>             & attributes)
{
  std::shared_ptr<ColorMap> colormap;
  const std::string rootpath = "colormaps/"; // FIXME: const?

  // Use filename attribute if we have it, else use the key
  std::string filename;

  filename = attributes["filename"];
  if (!filename.empty()) {
    filename = rootpath + filename;
    std::cout << "filename1: " << filename << "\n";
  } else {
    // Fall back to trying the key as a W2 colormap lookup name...
    filename = rootpath + key + ".xml";
  }

  // This find step is copied over from the other palette reader functions
  // Now try to read it
  URL find = Config::getConfigFile(filename);

  std::cout << "find: " << find << "\n";

  // This redirects to the function which actually reads the colormap
  if (!find.empty()) {
    colormap = readPalColorMap(find.toString());

    colormap->setSpecialColorStrings(attributes["missing"], attributes["unavailable"]);
  }
  return colormap;
}

std::shared_ptr<ColorMap>
ConfigColorMap::readPalColorMap(const URL& url)
{
  std::shared_ptr<DefaultColorMap> colormap;
  std::string fname = url.toString();
  std::ifstream inf(fname);

  if (!inf.is_open()) {
    fLogSevere("Could not open PAL file: {}", fname);
    return nullptr;
  }

  try {
    colormap = std::make_shared<DefaultColorMap>();

    // Modern C++ structs to replace the legacy SCIPalette
    struct RangeColor { short low, high; int r, g, b; };
    struct LinearColor { int r, g, b; };

    std::vector<RangeColor> ranges;
    std::vector<LinearColor> linears;
    short boundMin = 0, boundMax = 0;

    std::string line;
    // Single-pass stream reading replaces the old two-pass fgets/rewind logic
    while (std::getline(inf, line)) {
      if (line.empty()) { continue; }

      std::istringstream iss(line);
      std::string type;
      iss >> type;

      // Extract values cleanly without sscanf buffer risks
      if (type == "R") {
        RangeColor rc;
        if (iss >> rc.low >> rc.high >> rc.r >> rc.g >> rc.b) {
          ranges.push_back(rc);
        }
      } else if (type == "L") {
        LinearColor lc;
        if (iss >> lc.r >> lc.g >> lc.b) {
          linears.push_back(lc);
        }
      } else if (type == "B") {
        iss >> boundMin >> boundMax;
      }
    }

    double upper = HUGE_VAL;

    // Process Range Colors
    for (const auto& rc : ranges) {
      std::string buf = fmt::format("{:.1f}", upper);

      // Converted / 10 to / 10.0 to prevent implicit integer division truncation
      colormap->addBin(true, buf, rc.high / 10.0, rc.low / 10.0,
        rc.r, rc.g, rc.b, 255,
        rc.r, rc.g, rc.b, 255);
    }

    // Process Linear Colors
    if (!linears.empty()) {
      float lin_spacing = static_cast<float>(boundMax - boundMin) / linears.size();
      float lbound      = boundMin;

      for (const auto& lc : linears) {
        float ubound = lbound + lin_spacing;

        std::string buf = fmt::format("{:.1f}", upper);

        colormap->addBin(true, buf, ubound / 10.0, lbound / 10.0,
          lc.r, lc.g, lc.b, 255,
          lc.r, lc.g, lc.b, 255);
        lbound = ubound;
      }
    }
  } catch (const std::exception& e) {
    colormap = nullptr;
    fLogSevere("Tried to read color map and failed: {}", e.what());
  }

  if (colormap) {
    fLogInfo("Loaded PAL colormap at {}", fname);
  }
  return colormap;
} // ConfigColorMap::readPalColorMap

std::shared_ptr<ColorMap>
ConfigColorMap::readW2ColorMap(const URL& url)
{
  std::shared_ptr<DefaultColorMap> colormap;
  auto xml = IODataType::read<PTreeData>(url.toString());

  if (xml == nullptr) {
    fLogSevere("Failed to read or parse W2 ColorMap XML at {}", url.toString());
    return nullptr;
  }

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
        fLogSevere("Bad or missing upperbound value in colorBin attribute");
        return nullptr;
      }
      if (upperStr == "infinity") {
        upper = HUGE_VAL;
      } else if (upperStr == "-infinity") {
        upper = -HUGE_VAL;
      } else {
        try {
          upper = std::stod(upperStr);
        } catch (const std::exception& e) {
          fLogSevere("Failed to parse upperBound '{}' as double: {}", upperStr, e.what());
          return nullptr;
        }
      }

      // colors
      auto colors = b.getChildren("color");

      if (colors.size() < 0) {
        fLogSevere("Missing colors in colorBin");
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
      if (label.empty()) {
        if (c1 == c2) {
          name = fmt::format("{:.1f}", upper);
        } else {
          name = fmt::format("{:.1f}-{:.1f}", lower, upper);
        }
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
    fLogSevere("Tried to read color map and failed: {}", e.what());
  }
  fLogInfo("Loaded w2 colormap at {}", url.toString());
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
      std::cout << "type = " << type << "\n";
      if (type.empty()) { type = "w2"; }
      // FIXME: lowercase force?
      if (!((type == "w2") || (type == "para") || (type == "pal"))) {
        fLogSevere("Unrecognized color map type '{}', tryinig 'w2'", type);
        type = "w2";
      }

      // Delagate to different creation functions...
      if (type == "w2") {
        colormap = readW2ColorMap(key, attributes);
      } else if (type == "para") {
        colormap = readParaColorMap(key, attributes);
      } else if (type == "pal") {
        colormap = readPalColorMap(key, attributes);
      }
    } else {
      // If not in the config, try the with w2 which will
      // look for key.xml
      attributes["filename"] = ""; // Force reader to use key.xml
      colormap = readW2ColorMap(key, attributes);
    }

    // If anything failed use my debugging high contrast all data map
    if (colormap == nullptr) { // ALWAYS return a color map
      fLogSevere("Using testing high contrast ColorMap for key {}", key);
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
