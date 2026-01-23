#include "rConfigColorMap.h"

#include "rStrings.h"
#include "rError.h"
#include "rIODataType.h"
#include "rColorMap.h"

#include <memory>
#include <stdlib.h>
#include <cmath>

#include <library_sci_palette.h>

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
  std::shared_ptr<DefaultColorMap> colormap;
  const std::string rootpath = "colormaps/"; // FIXME: const?

  // Use filename attribute if we have it, else we fail
  std::string filename;

  filename = attributes["filename"];
  if (!filename.empty()) {
    filename = rootpath + filename;
  } else {
    fLogSevere("Paraview colormap requires a filename to a map");
    return nullptr;
  }

  // The 'name' key to match in file
  const std::string name = attributes["name"];

  // Now try to read it
  URL find = Config::getConfigFile(filename);

  if (find.empty()) {
    fLogSevere("Cannot read Paraview color map file {}", filename);
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
      fLogSevere("Exception processing XML {} {}", filename, e.what());
    }
  }

  fLogInfo("Loaded para colormap with key {}", key);
  return colormap;
} // ColorMap::readParaColorMap

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

// Function for reading in .pal files
std::shared_ptr<ColorMap>
ConfigColorMap::readPalColorMap(const URL& url)
{
  std::shared_ptr<DefaultColorMap> colormap;
  auto xml = IODataType::read<PTreeData>(url.toString());

  try{
    // out = std::make_shared<ColorMap>(); GOOP
    colormap = std::make_shared<DefaultColorMap>();
    // auto map = xml->getTree()->getChild("colorMap");

    // This is old read generic ability, which is a ton of color maps
    // but not all.

    // auto bins    = map.getChildren("colorBin");

    SCIPalette * pal;
    pal = new SCIPalette;

    char flag[8], buffer[128];
    FILE * inf;

    std::string fname = url.toString();

    if ( (inf = fopen(fname.c_str(), "r")) == NULL) { return NULL; }

    // count the number of range colors and linear colors in the .pal file, as these are handled differently
    pal->number_range_colors  = 0;
    pal->number_linear_colors = 0;
    while (fgets(buffer, 126, inf) != NULL) {
      if (buffer[0] == 'R') { ++pal->number_range_colors; }
      if (buffer[0] == 'L') { ++pal->number_linear_colors; }
    }

    // Prepare to add range colors, if present
    if (pal->number_range_colors > 0) {
      pal->ranges        = new short int * [pal->number_range_colors];
      pal->range_palette = new int * [pal->number_range_colors];

      for (int c = 0; c < pal->number_range_colors; ++c) {
        pal->ranges[c]        = new short int [2];
        pal->range_palette[c] = new int [3];
      }
    }
    // Prepare to add linear colors, if present
    if (pal->number_linear_colors > 0) {
      pal->linear_palette = new int * [pal->number_linear_colors];

      for (int c = 0; c < pal->number_linear_colors; ++c) {
        pal->linear_palette[c] = new int [3];
      }
    }

    int ri = 0, li = 0;
    rewind(inf);
    // While there are still lines in the colormap file, read colormap data to the range colors array, linear colors array, or min/max values
    while (fgets(buffer, 126, inf) != NULL) {
      if (buffer[0] == 'R') {
        sscanf(buffer, "%s %hd %hd %d %d %d", flag, &(pal->ranges[ri][LOW]), &(pal->ranges[ri][HIGH]),
          &(pal->range_palette[ri][RED]), &(pal->range_palette[ri][GREEN]), &(pal->range_palette[ri][BLUE]));
        ++ri;
      }
      if (buffer[0] == 'L') {
        sscanf(buffer, "%s %d %d %d", flag, &(pal->linear_palette[li][RED]), &(pal->linear_palette[li][GREEN]),
          &(pal->linear_palette[li][BLUE]));
        ++li;
      }
      if (buffer[0] == 'B') {
        sscanf(buffer, "%s %hd %hd", flag, &pal->min, &pal->max);
      }
    }
    fclose(inf);

    int colors   = pal->number_range_colors;
    int colors2  = pal->number_linear_colors;
    double lower = -HUGE_VAL;
    double upper = HUGE_VAL;

    // from the B line: the minimum and maximum values for the linear section
    int lbound = pal->min;
    int ubound = pal->max;

    float lin_spacing = 1;

    // Derive the spacing for the linear colors in the colormap
    if (colors2 > 0) {
      lin_spacing = (float) ((ubound - lbound) / colors2);
    }

    int red, blue, green;
    std::string redh, blueh, greenh;

    std::string tname;

    // Turn the colormap data from the file into colormap data that algorithms can use
    for (int k = 0; k < colors; k++) {
      int lrange = pal->range_palette[k][LOW];
      int urange = pal->range_palette[k][HIGH];
      red   = pal->range_palette[k][RED];
      blue  = pal->range_palette[k][BLUE];
      green = pal->range_palette[k][GREEN];

      std::stringstream sstream;
      sstream << std::hex << red;
      redh = sstream.str();
      sstream << std::hex << blue;
      blueh = sstream.str();
      sstream << std::hex << green;
      greenh = sstream.str();

      char buf[ 64 ];
      snprintf(buf, sizeof(buf), "%.1f", upper);
      tname = buf;
      colormap->addBin(true, tname, urange / 10, lrange / 10, red, green, blue, 255, red, green, blue, 255);
    }

    for (int k = 0; k < colors2; k++) {
      ubound = lbound + lin_spacing;
      std::cout << "lbound " << lbound << " ubound " << ubound << "\n";
      red   = pal->linear_palette[k][RED];
      blue  = pal->linear_palette[k][BLUE];
      green = pal->linear_palette[k][GREEN];

      std::stringstream sstream;
      sstream << std::hex << red;
      redh = sstream.str();
      sstream << std::hex << blue;
      blueh = sstream.str();
      sstream << std::hex << green;
      greenh = sstream.str();
      // std::cout << "red: " << redh << " blue: " << blueh << " green: " << greenh << "\n";
      // std::cout << "red: " << red << " blue: " << blue << " green: " << green << "\n";

      char buf[ 64 ];
      snprintf(buf, sizeof(buf), "%.1f", upper);
      tname = buf;
      colormap->addBin(true, tname, ubound / 10, lbound / 10, red, green, blue, 255, red, green, blue, 255);
      lbound = ubound;
    }


    // FIXME: need to read the unit right? Conversion?
  }catch (const std::exception& e) {
    colormap = nullptr;
    fLogSevere("Tried to read color map and failed: {}", e.what());
  }
  fLogInfo("Loaded w2 colormap at {}", url.toString());
  return colormap;
} // ColorMap::readPalColorMap

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
        fLogSevere("Bad or missing upperbound value in colorBin attribute");
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

SCIPalette *
SCIPaletteFromFile(char * filename)
{
  SCIPalette * pal;

  pal = new SCIPalette;

  char flag[8], buffer[128];
  FILE * inf;

  if ( (inf = fopen(filename, "r")) == NULL) { return NULL; }

  pal->number_range_colors  = 0;
  pal->number_linear_colors = 0;
  while (fgets(buffer, 126, inf) != NULL) {
    if (buffer[0] == 'R') { ++pal->number_range_colors; }
    if (buffer[0] == 'L') { ++pal->number_linear_colors; }
  }

  if (pal->number_range_colors > 0) {
    pal->ranges        = new short int * [pal->number_range_colors];
    pal->range_palette = new int * [pal->number_range_colors];

    for (int c = 0; c < pal->number_range_colors; ++c) {
      pal->ranges[c]        = new short int [2];
      pal->range_palette[c] = new int [3];
    }
  }

  if (pal->number_linear_colors > 0) {
    pal->linear_palette = new int * [pal->number_linear_colors];

    for (int c = 0; c < pal->number_linear_colors; ++c) {
      pal->linear_palette[c] = new int [3];
    }
  }

  int ri = 0, li = 0;

  rewind(inf);
  while (fgets(buffer, 126, inf) != NULL) {
    if (buffer[0] == 'R') {
      sscanf(buffer, "%s %hd %hd %d %d %d", flag, &(pal->ranges[ri][LOW]), &(pal->ranges[ri][HIGH]),
        &(pal->range_palette[ri][RED]), &(pal->range_palette[ri][GREEN]), &(pal->range_palette[ri][BLUE]));
      ++ri;
    }
    if (buffer[0] == 'L') {
      sscanf(buffer, "%s %d %d %d", flag, &(pal->linear_palette[li][RED]), &(pal->linear_palette[li][GREEN]),
        &(pal->linear_palette[li][BLUE]));
      ++li;
    }
    if (buffer[0] == 'B') {
      sscanf(buffer, "%s %hd %hd", flag, &pal->min, &pal->max);
    }
  }
  fclose(inf);

  return pal;
} // SCIPaletteFromFile
