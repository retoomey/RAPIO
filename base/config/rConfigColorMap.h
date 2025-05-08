#pragma once

#include <rConfig.h>
#include <rColorMap.h>

#include <string>

#include <library_sci_palette.h>

namespace rapio {
/** Does the work of reading in color map configurations */
class ConfigColorMap : public ConfigType {
public:

  /** Get/load a color map from a key, usually DataType */
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

  static std::shared_ptr<ColorMap>
  readPalColorMap(const URL& url);

  static std::shared_ptr<ColorMap>
  readPalColorMap(const std::string& key, std::map<std::string, std::string>& attributes);

  /** Read a W2 Color map */
  static std::shared_ptr<ColorMap>
  readW2ColorMap(const std::string& key, std::map<std::string, std::string>& attributes);

  /** Read a Paraview Color map */
  static std::shared_ptr<ColorMap>
  readParaColorMap(const std::string& key, std::map<std::string, std::string>& attributes);

  /** Read a W2 color map from a given URL */
  static std::shared_ptr<ColorMap>
  readW2ColorMap(const URL& u);

  SCIPalette *SCIPaletteFromFile(char *filename);

private:

  /** Cache of keys to color maps */
  static std::map<std::string, std::shared_ptr<ColorMap> > myColorMaps;
};
}
