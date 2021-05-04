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

namespace {
// https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames#C.2FC.2B.2B
int
long2tilex(double lon, int z)
{
  return (int) (floor((lon + 180.0) / 360.0 * (1 << z)));
}

int
lat2tiley(double lat, int z)
{
  double latrad = lat * M_PI / 180.0;

  return (int) (floor((1.0 - asinh(tan(latrad)) / M_PI) / 2.0 * (1 << z)));
}

double
tilex2long(int x, int z)
{
  return x / (double) (1 << z) * 360.0 - 180;
}

double
tiley2lat(int y, int z)
{
  double n = M_PI - 2.0 * M_PI * y / (double) (1 << z);

  return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
}
}

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
    myfile << w->getPath() << " --- ";
  }
  // LogSevere("PATH " << w->getPath() << "\n");
  for (auto& a:w->getMap()) {
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
    // LogSevere("Field " << a.first << " == " << a.second << "\n");
  }
  if (debuglog) {
    myfile << "\n";
    myfile.close();
  }

  // If our single file exists use it to make tiles
  if (myTileData != nullptr) {
    std::string pathout = "";

    // --------------------------------------------------------------------
    // This should be broken up/reorganized into functions probably...
    // parse the incoming url and params
    // Look for supported URL formats...
    std::vector<std::string> pieces;
    Strings::splitWithoutEnds(w->getPath(), '/', &pieces);
    if (pieces.size() > 0) {
      auto& type = pieces[0];

      // FIXME: I could see these being modules...

      // for WMS there's a BBOX sent by leaflet.tileLayer.wms or openlayers source.TileArcGISRest
      if (type == "wms") {
        // Go through the bbox parameters, check length/type, etc.
        // Make sure they are numbers (harden URL for security)
        std::vector<std::string> bbox;
        Strings::splitWithoutEnds(myOverride["BBOX"], ',', &bbox);
        if (bbox.size() != 4) {
          LogSevere("Expected 4 parameters in BBOX for WMS server request, got " << bbox.size() << "\n");
          return;
        }
        for (auto a:bbox) {
          try{
            std::stof(a);
          }catch (const std::exception& e) {
            LogSevere("Non number passed for BBOX parameter? " << a << "\n");
            return;
          }
        }
        // Safe now to make disk key.  Might be better to have wms/tms ultimately use the same key?
        // We'll have to enhance this for time and multi product at some point
        pathout = cache + "/wms/T" + bbox[0] + "/T" + bbox[1] + "/T" + bbox[2] + "/T" + bbox[3] + "/tile.png";

        // for TMS  the {x}{y}{z} we would get boundaries from the tile number and zoom
        // This divides the world into equal spherical mercator squares.  We'll translate to bbox
        // for the writer
      } else if (type == "tms") {
        // Verify the z/y/x and that it's integers
        if (pieces.size() < 4) {
          LogSevere("Expected http://tms/z/x/y for start of tms path\n");
          return;
        }
        int x, y, z;
        try{
          z = std::stoi(pieces[1]);
          x = std::stoi(pieces[2]);
          y = std::stoi(pieces[3]);
        }catch (const std::exception& e) {
          LogSevere("Non number passed for z, x or y? " << pieces[1] << ", " << pieces[2] << ", " << pieces[3] << "\n");
          return;
        }

        // Safe now to make disk key.  Might be better to have wms/tms ultimately use the same key?
        // We'll have to enhance this for time and multi product at some point
        pathout = cache + "/tms/" + std::to_string(z) + "/" + std::to_string(x) + "/" + std::to_string(y) + ".png";

        // Slippy tiles to bbox... https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
        const double lon1 = tilex2long(x, z); // Maybe it's backwards?
        const double lat1 = tiley2lat(y, z);
        const double lon2 = tilex2long(x + 1, z);
        const double lat2 = tiley2lat(y + 1, z);
        myOverride["BBOX"] = std::to_string(lon1) + "," + std::to_string(lat1) + "," + std::to_string(lon2) + ","
          + std::to_string(lat2);
        LogSevere("Calculated " << myOverride["BBOX"] << "\n");
        myOverride["BBOXSR"] = "4326"; // Lat/Lon
        // Wow you'd think all the documentation it would be standard or something lol,
        // FIXME: come back to this
        LogSevere("TMS tiles not working yet :( use WMS, havan't gotten the projection math correct here...\n");
        return;
      } else {
        LogSevere("Expected either wms or tms server types. http://servername/wms/... \n");
        return;
      }
      // LogInfo("Server request using " << pieces[0] << "\n");
    }
    // --------------------------------------------------------------------


    // FIXME: Think I need to refactor the whole write methods now that I've
    // added so many features to make it 'cleaner'
    // writeOutputProduct(r->getTypeName(), r, myOverride); // Typename will be replaced by -O filters

    // FIXME: for production check path doesn't go up (security)
    if (!OS::isRegularFile(pathout)) {
      // LogInfo("Make: " << pathout << "\n");
      // FIXME: always the same input file for this first pass.  We need to add product name and
      // time.  Also for a server we need more fetch API I think
      writeDirectOutput(URL(pathout), myTileData, myOverride);
    } else {
      // LogInfo("IN CACHE:" << pathout << "\n");
    }

    // Ok webserver can auto respond with this given file..
    w->setFile(pathout);
  }
} // RAPIOTileAlg::processWebMessage

int
main(int argc, char * argv[])
{
  // Create and run a tile alg
  RAPIOTileAlg alg = RAPIOTileAlg();

  alg.executeFromArgs(argc, argv);
}
