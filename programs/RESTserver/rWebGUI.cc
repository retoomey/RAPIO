#include "rWebGUI.h"
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
RAPIOWebGUI::declarePlugins()
{
  // We're a web server, add port option.
  PluginWebserver::declare(this, "port");
}

void
RAPIOWebGUI::declareOptions(RAPIOOptions& o)
{
  o.setDescription("RAPIO Tile Algorithm");
  o.setAuthors("Robert Toomey");

  o.optional("i", "", "Initial file to use on startup (for quick web browsing).");
  o.optional("o", "CACHE", "Cache location for tile generation.");

  // Get location of binary + web is our default 'root'
  myRoot = OS::getProcessPath() + "/web";

  o.optional("root", myRoot, "Web root.  Defaults to binary location+'web'.");
}

/** RAPIOAlgorithms process options on start up */
void
RAPIOWebGUI::processOptions(RAPIOOptions& o)
{
  // Humm webserver should be getting these passed in, right?
  myOverride["cols"] = "256";
  myOverride["rows"] = "256";

  std::string startupFile = o.getString("i");

  myStartUpFile = "";
  if (!startupFile.empty()) {
    std::vector<std::string> pieces;
    Strings::splitWithoutEnds(startupFile, '=', &pieces);
    if (pieces.size() == 2) {
      myStartUpFile = pieces[1];
    } else {
      myStartUpFile = startupFile;
    }
  }

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
RAPIOWebGUI::processNewData(rapio::RAPIOData& d)
{
  // We don't use the record/asynchronous load.  Filename will
  // be passed in the web message
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

// ---------------------------------------
// All these are alpha/not working, basically
// an attempt to browse folders.  We'll get to this maybe
//
std::vector<std::string>
getProductList()
{
  std::string rootdisk = "/home/dyolf/DATA2/MAY3KTLX/";

  if (rootdisk[rootdisk.size() - 1] != '/') {
    rootdisk += '/';
  }

  std::vector<std::string> products;

  DIR * dirp = opendir(rootdisk.c_str()); // close?

  if (dirp == 0) {
    return products;
  }

  if (dirp != 0) {
    struct dirent * dp;
    while ((dp = readdir(dirp)) != 0) {
      const std::string full = rootdisk + dp->d_name;
      // Always ignore files starting with . (0 terminated so don't need length check)
      if (dp->d_name[0] == '.') { continue; }
      if (OS::isDirectory(full)) { // maybe inline here faster or we go the getdents rabbithole
        products.push_back(dp->d_name);
      }
    }
    closedir(dirp);
    //     for(int i =0; i<100; i++){
    //       products.push_back("Another"+std::to_string(i));
    //     }
  }
  sort(products.begin(), products.end(), std::less<std::string>());
  return products;
} // getProductList

std::vector<std::string>
getSubtypeList(const std::string& product)
{
  std::string rootdisk = "/home/dyolf/DATA2/MAY3KTLX/" + product;

  if (rootdisk[rootdisk.size() - 1] != '/') {
    rootdisk += '/';
  }

  std::vector<std::string> subtypes;

  // Duplicate the code..I think we'll have to sort the subtypes...
  // We could sort the products too right?
  DIR * dirp = opendir(rootdisk.c_str()); // close?

  if (dirp == 0) {
    return subtypes;
  }

  if (dirp != 0) {
    struct dirent * dp;
    while ((dp = readdir(dirp)) != 0) {
      const std::string full = rootdisk + dp->d_name;
      // Always ignore files starting with . (0 terminated so don't need length check)
      if (dp->d_name[0] == '.') { continue; }
      LogSevere("Checking subtype " << full << "\n");
      if (OS::isDirectory(full)) { // maybe inline here faster or we go the getdents rabbithole
        subtypes.push_back(dp->d_name);
      }
    }
    closedir(dirp);
  }
  sort(subtypes.begin(), subtypes.end(), std::greater<std::string>());
  return subtypes;
} // getSubtypeList

std::vector<std::string>
getTimeList(const std::string& product, const std::string& subtype)
{
  std::string rootdisk = "/home/dyolf/DATA2/MAY3KTLX/" + product + "/" + subtype;

  if (rootdisk[rootdisk.size() - 1] != '/') {
    rootdisk += '/';
  }
  LogSevere("Checking root: " << rootdisk << "\n");

  std::vector<std::string> times;

  // Duplicate the code..I think we'll have to sort the subtypes...
  // We could sort the products too right?
  DIR * dirp = opendir(rootdisk.c_str()); // close?

  if (dirp == 0) {
    return times;
  }

  if (dirp != 0) {
    struct dirent * dp;
    while ((dp = readdir(dirp)) != 0) {
      const std::string full = rootdisk + dp->d_name;
      // Always ignore files starting with . (0 terminated so don't need length check)
      if (dp->d_name[0] == '.') { continue; }
      if (OS::isRegularFile(full)) { // maybe inline here faster or we go the getdents rabbithole
        // FIXME: check ending?
        times.push_back(dp->d_name);
      }
    }
    closedir(dirp);
  }
  sort(times.begin(), times.end(), std::greater<std::string>());
  return times;
} // getTimeList

// These will become part of the 'navigator' class I think...
// There's duplicate code in these functions, but don't
// replace with a generic vector..if anything macro the shared
// code
std::string
getJSONProductList()
{
  auto l = getProductList();

  std::stringstream json;

  json << "{\"products\":[";
  std::string add = "";

  for (auto& p:l) {
    json << add << "\"" << p << "\"";
    add = ",";
  }
  json << "]}";
  return json.str();
}

std::string
getJSONSubtypeList(const std::string& product)
{
  auto l = getSubtypeList(product);

  std::stringstream json;

  json << "{\"subtypes\":[";
  std::string add = "";

  for (auto& p:l) {
    json << add << "\"" << p << "\"";
    add = ",";
  }
  json << "]}";
  return json.str();
}

std::string
getJSONTimeList(const std::string& product, const std::string& subtype)
{
  auto l = getTimeList(product, subtype);

  std::stringstream json;

  json << "{\"times\":[";
  std::string add = "";

  for (auto& p:l) {
    json << add << "\"" << p << "\"";
    add = ",";
  }
  json << "]}";
  return json.str();
}

void
RAPIOWebGUI::handlePathUI(WebMessage& w, std::vector<std::string>& pieces)
{
  LogSevere("Woooh...something calling our json stuff\n");

  // Ok for now make json by hand.  We need to get the javascript to react
  std::stringstream json;

  json << "{";
  json << " \"top\": ";
  static bool toggle = false;

  toggle = !toggle;

  json << (toggle ? " \"red\" " : " \"green\" ");
  json << "}";

  // w->setMessage("{ \"date\": \"05-16-2021\", \"milliseconds_since_epoch\": 1621193354208, \"time\": \"07:29:14 PM\" }", "application/json");
  w.setMessage(json.str(), "application/json");
  return;
}

void
RAPIOWebGUI::handlePathData(WebMessage& w, std::vector<std::string>& pieces)
{
  // We have to create a RECORD from the stuff passed in.
  // Then create a RAPIOData?  Have to cache the datatype somehow or it will be snail slow
  // From record we want 'up' and 'down' for example...navigation directions

  /*
   *    if (pieces.size() < 2){
   *      w->setMessage("{ \"ERROR\": \"Unknown\"}", "application/json");
   *      return;
   *    }
   *    // S3 requires the year top path added, right?
   *    std::string rootproduct = pieces[1];
   */

  /*        // We could map this from say 'KTLX' to this path...a source lookup basically...
   *      // So pieces[0] = ktlx --> this
   *      std::string rootdisk = "/home/dyolf/DATA2/MAY3KTLX/";
   *      // FIXME: ensure ends with "/";
   *
   *      // Navigator::getProductList()
   *      // s3 maybe?
   *      // ls the product folder or s3 aws python call the list back, right?
   *      //
   *      // Open directory
   *      DIR * dirp = opendir(rootdisk.c_str());
   *      if (dirp == 0){
   *        w->setMessage("{ \"ERROR\": \"Unknown\"", "application/json");
   *        return;
   *      }
   *
   *      // ----------------------------------------------------------------
   *      // Create json from directory list.  We could create a PTree
   *      std::stringstream json;
   *      json << "{\"products\":[";
   *      struct dirent * dp;
   *      std::string add = "";
   *      while ((dp = readdir(dirp)) != 0) {
   *        const std::string full = rootdisk + dp->d_name;
   *        //const std::string full = rootdisk + "/" + dp->d_name;
   *        // Always ignore files starting with . (0 terminated so don't need length check)
   *        if (dp->d_name[0] == '.') { continue; }
   *        if (OS::isDirectory(full)) {
   *          //json << add << "\"file\": \""<<dp->d_name<<"\"";
   *          json << add << "\""<<dp->d_name<<"\"";
   *          add = ","; // future adds need comma
   *        }
   *      }
   *      json << "]}";
   *      // ----------------------------------------------------------------
   *      // */

  /*
   *      for(size_t i=1; i<pieces.size(); i++){
   *        json << " \"value" << i  << "\": \"" << pieces[i] << "\"";
   *        if (i != pieces.size()-1){
   *          json << ", ";
   *        }
   *      }
   */
  if (pieces.size() < 2) {
    w.setMessage(getJSONProductList(), "application/json");
  } else {
    if (pieces.size() < 3) {
      const std::string s = pieces[1];
      LogSevere("Getting subtypes for " << s << "\n");
      w.setMessage(getJSONSubtypeList(s), "application/json");
    } else {
      const std::string s = pieces[1];
      const std::string t = pieces[2];
      LogSevere("Getting times for " << s << "\n");
      w.setMessage(getJSONTimeList(s, t), "application/json");
    }
  }
  return;
}

void
RAPIOWebGUI::handleColorMap(WebMessage& w, std::vector<std::string>& pieces, std::map<std::string,
  std::string>& settings)
{
  std::stringstream json;

  // for moment use the single loaded datatype for color map
  // though we should be requesting by name I think
  // This means we will need a way to request meta information on data
  if (myTileData != nullptr) {
    // std::shared_ptr<DataType> myTileData;
    // This assumes the loaded data which at some point we're gonna have to change.
    // We might want to get color map by NAME actually.
    auto c = myTileData->getColorMap();
    // Bleh need a json color map, right?  So we can apply it on front end side
    // Actually no, it can be a binary format.  It's just less flexible.
    // Also the web guys will want an svg generator as well.  Lots of color map work
    // to do.
    // Needs to be UTF-8 for text/plain.j
    // w.setMessage("This should work!", "text/plain");
    // We probably want a json version and a binary version.  Hate to say it.
    //
    c->toJSON(json);
    w.setMessage(json.str());
  }
}

void
RAPIOWebGUI::handleSVG(WebMessage& w, std::vector<std::string>& pieces, std::map<std::string,
  std::string>& settings)
{
  if (myTileData != nullptr) {
    // std::shared_ptr<DataType> myTileData;
    // This assumes the loaded data which at some point we're gonna have to change.
    // We might want to get color map by NAME actually.
    auto c     = myTileData->getColorMap();
    auto units = myTileData->getUnits();

    // Creating SVG on the fly, though guess we 'could' have a writer for it and cache files of them.
    // if (myTileData != nullptr) {
    // std::shared_ptr<DataType> myTileData;
    // This assumes the loaded data which at some point we're gonna have to change.
    // We might want to get color map by NAME actually.
    //  auto c = myTileData->getSVG();
    // }
    std::stringstream svg;
    c->toSVG(svg, units);
    w.setMessage(svg.str());
  }
}

void
RAPIOWebGUI::serveTile(WebMessage& w, std::string& pathout, std::map<std::string, std::string>& settings)
{
  // Write tile to cache using all the settings
  if (!OS::isRegularFile(pathout)) {
    writeDirectOutput(URL(pathout), myTileData, settings);
  } else {
    // LogInfo("IN CACHE:" << pathout << "\n");
  }
  w.setFile(pathout); // Web server file response
}

void
RAPIOWebGUI::handlePathWMS(WebMessage& w, std::vector<std::string>& pieces,
  std::map<std::string, std::string>& settings)
{
  // If our single file exists use it to make tiles
  if (myTileData == nullptr) {
    return; // FIXME: some sort of web error code or what?
  }

  // CACHE SETTING (post handleOverrides to avoid any hack there)
  std::string cache = settings["tilecachefolder"];
  // Go through the bbox parameters, check length/type, etc.
  // Make sure they are numbers (harden URL for security)
  std::vector<std::string> bbox;

  Strings::splitWithoutEnds(settings["BBOX"], ',', &bbox);
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
  std::string suffix  = settings["suffix"];
  std::string pathout = cache + "/wms/T" + bbox[0] + "/T" + bbox[1] + "/T" + bbox[2] + "/T" + bbox[3] + "/tile."
    + suffix;

  // LogInfo("WMS_BBOX:" << settings["BBOX"] << "\n");
  settings["TILETEXT"] = "";

  // for TMS  the {x}{y}{z} we would get boundaries from the tile number and zoom
  // This divides the world into equal spherical mercator squares.  We'll translate to bbox
  // for the writer
  serveTile(w, pathout, settings);
} // RAPIOWebGUI::handlePathWMS

void
RAPIOWebGUI::handlePathTMS(WebMessage& w, std::vector<std::string>& pieces, std::map<std::string,
  std::string>& settings)
{
  // If our single file exists use it to make tiles
  if (myTileData == nullptr) {
    return; // FIXME: some sort of web error code or what?
  }

  // CACHE SETTING (post handleOverrides to avoid any hack there)
  std::string cache = settings["tilecachefolder"];

  // Verify the z/y/x and that it's integers
  if (pieces.size() < 4) {
    LogSevere("Expected http://tms/z/x/y for start of tms path\n");
    for (auto k:pieces) {
      std::cout << "PIECE " << k << "\n";
    }
    return;
  }
  int x, y, z;

  try{
    z = std::stoi(pieces[1]);
    x = std::stoi(pieces[2]);
    // Google vs TMS can flip this.  Usually fixable with -y or y passed into URL
    // Typically 'google' is 0,0 at top left, and 'tms' is 0,0 at bottom right
    y = std::stoi(pieces[3]);
  }catch (const std::exception& e) {
    LogSevere("Non number passed for z, x or y? " << pieces[1] << ", " << pieces[2] << ", " << pieces[3] << "\n");
    return;
  }

  // Safe now to make disk key.  Might be better to have wms/tms ultimately use the same key?
  // We'll have to enhance this for time and multi product at some point
  std::string suffix  = settings["suffix"];
  std::string pathout = cache + "/tms/" + std::to_string(z) + "/" + std::to_string(x) + "/" + std::to_string(y)
    + "." + suffix;

  // Slippy tiles to bbox... https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
  const double lon1 = tilex2long(x, z); // Maybe it's backwards?
  const double lat1 = tiley2lat(y, z);
  const double lon2 = tilex2long(x + 1, z);
  const double lat2 = tiley2lat(y + 1, z);

  settings["BBOX"] = std::to_string(lon1) + "," + std::to_string(lat2) + "," + std::to_string(lon2) + ","
    + std::to_string(lat1);
  settings["BBOXSR"]   = "4326"; // Lat/Lon
  settings["TILETEXT"] = "X:" + std::to_string(x) + ",Y:" + std::to_string(y) + ",Z:" + std::to_string(z);

  // LogInfo("TMS XYZ (" << x << ", " << y << ", " << z << ") BBOX: " << settings["BBOX"] << "\n");

  serveTile(w, pathout, settings);
} // RAPIOWebGUI::handlePathTMS

void
RAPIOWebGUI::handlePathDefault(WebMessage& w)
{
  // First try to get the file
  std::string path = w.getPath();

  // Set this here instead of webserver since 'maybe' we'll want root
  // directory someday
  if (w.getPath() == "/") { path += "index.html"; }

  #if 0
  // Deprecated
  // Don't think we need this.  Relative paths should be
  // enough.  'Possibly' some info might be needed later?

  // oops need the port from settings
  std::string hostname = OS::getHostName();

  if (OS::isWSL()) {
    // Hack so I can run browser on host.
    // Really what we need is the ip of the calling web page
    hostname = "localhost";
  }

  // Just in case we pass back a png or something, don't try to macro it
  if (Strings::endsWith(path, ".html")) {
    const std::string portStr = std::to_string(WebServer::port);
    w.addMacro("WMS", "http://" + hostname + ":" + portStr + "/wms");
    w.addMacro("HOSTNAME", hostname);
    w.addMacro("PORT", portStr);
    LogSevere("HOSTNAME is " << hostname << " and " << portStr << "\n");
  }
  #endif // if 0

  // Find the file...
  // Remove '..' the most dangerous root breaker.
  size_t pos;
  std::string toRemove = "..";

  while ((pos = path.find(toRemove)) != std::string::npos) {
    path.erase(pos, toRemove.size());
  }
  std::string loc = myRoot + path;

  w.setFile(loc); // Web server handles the cases
} // RAPIOWebGUI::handlePathDefault

void
RAPIOWebGUI::logWebMessage(const WebMessage& w)
{
  std::ofstream myfile;

  myfile.open("logtile.txt", ios::app);
  myfile << w.getPath() << " --- ";
  for (auto& a:w.getMap()) {
    myfile << a.first << "=" << a.second << ":";
  }
  myfile << "\n";
  myfile.close();
}

void
RAPIOWebGUI::handleOverrides(const std::map<std::string, std::string>& params,
  std::map<std::string, std::string>                                 & settings)
{
  // Filter web GET params into ours if needed, WMS/TMS can use
  // different strings.
  for (auto& a:params) {
    settings[a.first] = a.second;

    // Leaflet WMS fields to ours...
    if (a.first == "bbox") {
      settings["BBOX"] = a.second;
    } else if (a.first == "height") {
      settings["rows"] = a.second;
    } else if (a.first == "width") {
      settings["cols"] = a.second;
    } else if (a.first == "srs") {   // wms source
      if (a.second == "EPSG:4326") { // lat lon
        settings["BBOXSR"] = "4326";
      } else if (a.second == "EPSG:3857") {
        settings["BBOXSR"] = "3857";
      }
    }
    // wms "format=image/jpeg" should probably handle it
    else if (a.first == "format") {
      // Suffixes for our writer.  jpg, png, mrmstile
      std::string trimS = a.second;
      if (trimS == "image/jpeg") {
        // Making it png so we have transparency.  Not sure why the wms/tms
        // are passing image/jpeg by default
        trimS = "png";
      }
      Strings::removePrefix(trimS, "image/");
      settings["suffix"] = trimS;
    }
  }
  if (settings["suffix"] == "") {
    settings["suffix"] = "png";
  }
} // RAPIOWebGUI::handleOverrides

void
RAPIOWebGUI::processWebMessage(std::shared_ptr<WebMessage> wsp)
{
  LogInfo("Request: " << wsp->getPath() << "\n");
  auto& w = *wsp;

  // Local settings for this call
  std::map<std::string, std::string> settings = myOverride;

  // Extra debugging logging to a 'server' file
  const bool debuglog = false;

  settings["debug"] = debuglog; // image write could use this for more info
  if (debuglog) {
    logWebMessage(w);
  }

  // GET current URL params into settings
  handleOverrides(w.getMap(), settings);

  // Arcgis or other gis software asking for capabilities.
  if ((settings["service"] == "WMS") && (settings["request"] == "GetCapabilities")) {
    // Arcgis earth (or others) asking for WMS capabilities.
    const URL url = Config::getConfigFile("misc/webguicapabilities.xml");
    if (url.empty()) {
      LogInfo("Received capabilities request but can't find webguicapabilities.xml\n");
      w.setMessage("Error");
      return;
    } else {
      LogInfo("Request for capabilities succeeded.\n");
    }

    std::ifstream file(url.toString());
    std::ostringstream buffer;
    buffer << file.rdbuf();
    w.setMessage(buffer.str(), "application/xml");
    return;
  } else {
    bool service = (settings["service"] == "WMS");
    // LogInfo("Settings?  " << service << "\n");
  }

  // CACHE SETTING (Global/not changable)
  std::string cache = myOverride["tilecachefolder"];

  // Break up the URL path first to delegate further
  std::vector<std::string> pieces;

  Strings::splitWithoutEnds(w.getPath(), '/', &pieces);
  std::string type = "";

  if (pieces.size() > 0) {
    type = Strings::makeLower(pieces[0]);
  }

  // Note: Was going to have 'format' control the type of tile
  // generated.  However, browser caching on browsers gets very
  // confused if two urls get different data back, even with
  // get params it seems.  So we'll have a dedicated path for
  // mrmstiles.

  // Handle path
  if (type == "wms") {
    handlePathWMS(w, pieces, settings);
  } else if (type == "wmsdata") {
    settings["suffix"] = "mrmstile";
    handlePathWMS(w, pieces, settings);
  } else if (type == "tms") {
    handlePathTMS(w, pieces, settings);
  } else if (type == "tmsdata") {
    settings["suffix"] = "mrmstile";
    handlePathTMS(w, pieces, settings);
  } else if (type == "colormap") {
    handleColorMap(w, pieces, settings);
  } else if (type == "svg") {
    handleSVG(w, pieces, settings);
    // These are more experimental alpha
  } else if (type == "ui") {
    handlePathUI(w, pieces);
  } else if (type == "data") {
    handlePathData(w, pieces);
  } else {
    // Default path webpage, probably table of contents or something
    handlePathDefault(w);
  }
} // RAPIOWebGUI::processWebMessage

void
RAPIOWebGUI::execute()
{
  // Plugins (in particular we use the webserver plugin)
  for (auto p: myPlugins) {
    p->execute(this);
  }

  // We don't use process new data since that's a callback...
  // Direct read will be synchronous.
  if (!myStartUpFile.empty()) {
    // Handle the current -i "builder:filename" extension
    std::string aBuilder;
    std::string aFile;
    IODataType::iPathParse(myStartUpFile, aFile, aBuilder);
    LogInfo("Attempting to load given start up file " << aFile << "\n");
    auto d = IODataType::readDataType(aFile, aBuilder);
    if (d != nullptr) {
      myTileData = d;
    }
  }

  // Time until end of program.  Make it dynamic memory instead of heap or
  // the compiler will kill it too early (too smartz)
  static std::shared_ptr<ProcessTimer> fulltime(
    new ProcessTimer("Web server total runtime"));

  EventLoop::doEventLoop();
} // RAPIOWebGUI::execute

int
main(int argc, char * argv[])
{
  // Create and run a tile alg
  RAPIOWebGUI alg = RAPIOWebGUI();

  alg.executeFromArgs(argc, argv);
}
