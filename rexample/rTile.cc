#include "rTile.h"
#include <iostream>

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

  // Require zoom level, center lat and center lon
  o.require("zoom", "0", "Tile zoom level from 0 (whole world) up to 20 or more");
  o.require("center-latitude", "38", "Latitude degrees for tile center");
  o.require("center-longitude", "-98", "Longitude degrees for tile center");
  o.require("image-height", "500", "Height or rows of the image in pixels");
  o.require("image-width", "500", "Width or cols of the image in pixels");
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
}

void
RAPIOTileAlg::processNewData(rapio::RAPIOData& d)
{
  // Look for any data the system knows how to read...
  auto r = d.datatype<rapio::DataType>();

  if (r != nullptr) {
    // Standard echo of data to output.
    LogInfo("-->Tile: " << r->getTypeName() << "\n");
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
