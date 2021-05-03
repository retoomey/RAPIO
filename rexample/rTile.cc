#include "rTile.h"
#include <iostream>
#include <sys/stat.h>

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
    // If we're not a web server, then just process a tile in directly to output files...
    if (!isWebServer()) {
      // Standard echo of data to output.
      LogInfo("-->Tile: " << r->getTypeName() << "\n");
      writeOutputProduct(r->getTypeName(), r, myOverride); // Typename will be replaced by -O filters

      // If we're a web server, then probably cache/add to database, etc...
    } else {
      // We'll need database, etc. or volume for vertical slicing, etc.
      // For the first pass, simple use this data for ANY future tiles.
      // So for file= input option, we'll map that data file.
      // FIXME: More advanced ability
      LogSevere("We're a web server...replacing data used for tile generation\n");
      myTileData         = r;
      myOverride["mode"] = "tile"; // We want tile mode for output
      // None of these matter, we'll use BBOX from WMS (refactor)
      myOverride["zoom"] = "0";
      myOverride["centerLatDegs"] = "0";
      myOverride["centerLonDegs"] = "0";
    }
  }
} // RAPIOTileAlg::processNewData

void
RAPIOTileAlg::processWebMessage(std::shared_ptr<WebMessage> w)
{
  // Alpha of doing a tile server (using WMS at moment)
  // FIXME: need multiple file ability.  For now single file
  // as we get projection done

  // The settings and overrides
  const bool debuglog = false;

  myOverride["debug"] = debuglog;
  const std::string cache = "CACHE";

  std::ofstream myfile;
  if (debuglog) {
    myfile.open("logtile.txt", ios::app);
    myfile << w->myPath << " --- ";
  }
  for (auto& a:w->myMap) {
    if (debuglog) {
      myfile << a.first << "=" << a.second << ":";
    }
    if (a.first == "bbox") {
      myOverride["BBOX"] = a.second;
    } else {
      myOverride[a.first] = a.second;
    }
    // yeah yeah yeah..alpha steps
    if (a.first == "height") {
      myOverride["rows"] = a.second;
    }
    if (a.first == "width") {
      myOverride["cols"] = a.second;
    }
  }
  if (debuglog) {
    myfile << "\n";
    myfile.close();
  }

  // If our single file exists use it to make tiles
  if (myTileData != nullptr) {
    // for TMS  the {x}{y}{z} we would get boundaries from the tile number and zoom

    // FIXME: Think I need to refactor the whole write methods now that I've
    // added so many features to make it 'cleaner'
    // writeOutputProduct(r->getTypeName(), r, myOverride); // Typename will be replaced by -O filters

    // FIXME: for production check path doesn't go up (security)
    std::string pathout = cache + std::string(w->myPath) + "/tile.png";
    boost::filesystem::file_status status = boost::filesystem::status(pathout);
    if (!boost::filesystem::is_regular_file(status)) {
      // LogInfo("Make: " << pathout << "\n");
      // FIXME: always the same input file for this first pass.  We need to add product name and
      // time.  Also for a server we need more fetch API I think
      writeDirectOutput(URL(pathout), myTileData, myOverride);
    } else {
      // LogInfo("IN CACHE:" << pathout << "\n");
    }

    // Ok webserver can auto respond with this given file..
    w->message = "file";
    w->file    = pathout;
  }
} // RAPIOTileAlg::processWebMessage

int
main(int argc, char * argv[])
{
  // Create and run a tile alg
  RAPIOTileAlg alg = RAPIOTileAlg();

  alg.executeFromArgs(argc, argv);
}
