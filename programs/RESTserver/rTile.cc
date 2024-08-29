#include "rTile.h"
#include <iostream>
#include <sys/stat.h>

#include <dirent.h>
#include <sys/stat.h>

#include <fstream>

using namespace rapio;

/*
 * A RAPIO algorithm for making tiles
 *
 * Basically all we do is use command parameters to
 * override the output settings dynamically.  These settings
 * are taken from rapiosettings.xml for 'default' behavior.
 *
 * Real time Algorithm Parameters and I/O
 * http://github.com/retoomey/RAPIO
 *
 * @author Robert Toomey
 **/
void
RAPIOTileAlg::declareOptions(RAPIOOptions& o)
{
  o.setDescription("RAPIO Tile Algorithm");
  o.setAuthors("Robert Toomey");

  // Optional zoom level, center lat and center lon
  // Note: Running as tile server you don't need any of these
  o.optional("zoom", "0", "Tile zoom level from 0 (whole world) up to 20 or more");
  o.optional("center-latitude", "38", "Latitude degrees for tile center");
  o.optional("center-longitude", "-98", "Longitude degrees for tile center");
  o.optional("image-height", "256", "Height or rows of the image in pixels");
  o.optional("image-width", "256", "Width or cols of the image in pixels");

  // Optional flags (my debugging stuff, or maybe future filters/etc.)
  o.optional("flags", "", "Extra flags for determining tile drawing");
}

/** RAPIOAlgorithms process options on start up */
void
RAPIOTileAlg::processOptions(RAPIOOptions& o)
{
  // Make an "<output>" string overriding the rapiosettings.xml.  These will
  // override any of those params. We only do a single level, which for now
  // at least is enough I think.  This API might develop more later if needed.
  // "<output mode="tile" suffix="png" cols="500" rows="500" zoom="12" centerLatDegs="35.22" centerLonDegs="-97.44"/>";
  myOverride["mode"] = "tile"; // We want tile mode for output
  myOverride["zoom"] = o.getString("zoom");
  myOverride["cols"] = o.getString("image-height");
  myOverride["rows"] = o.getString("image-width");
  myOverride["centerLatDegs"] = o.getString("center-latitude");
  myOverride["centerLonDegs"] = o.getString("center-longitude");
  myOverride["flags"]         = o.getString("flags");

  // Steal the normal output location as suggested cache folder
  // This will probably work unless someone deliberately tries to break it by passing
  // our other crazier output options
  std::string cache = o.getString("o");

  if (cache.empty()) {
    cache = "CACHE";
  }
  myOverride["tilecachefolder"] = cache;
}

void
RAPIOTileAlg::processNewData(rapio::RAPIOData& d)
{
  // Look for any data the system knows how to read...
  auto r = d.datatype<rapio::DataType>();

  if (r != nullptr) {
    LogInfo("-->Tile: " << r->getTypeName() << "\n");
    // Eh do we have to write it to disk?  Should stream it back, right?
    // We're gonna want to send it back on the command line right?
    writeOutputProduct(r->getTypeName(), r, myOverride); // Typename will be replaced by -O filters
  }
} // RAPIOTileAlg::processNewData

int
main(int argc, char * argv[])
{
  // Create and run a tile alg
  RAPIOTileAlg alg = RAPIOTileAlg();

  alg.executeFromArgs(argc, argv);
}
