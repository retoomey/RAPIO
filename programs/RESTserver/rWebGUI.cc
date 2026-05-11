#include "rWebGUI.h"
#include "rGDALDataType.h"
#include "rVectorDataType.h"
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

bool
getTileBBoxFromPath(const std::vector<std::string>& pieces, double& minLon, double& minLat, double& maxLon,
  double& maxLat, int& x, int& y, int& z)
{
  if (pieces.size() < 4) { return false; }
  try {
    z = std::stoi(pieces[1]);
    x = std::stoi(pieces[2]);
    y = std::stoi(pieces[3]);
  } catch (const std::exception& e) {
    return false;
  }
  minLon = tilex2long(x, z);
  maxLat = tiley2lat(y, z); // Y is inverted in TMS (0 is North)
  maxLon = tilex2long(x + 1, z);
  minLat = tiley2lat(y + 1, z);
  return true;
}

std::shared_ptr<VectorDataType>
getCachedVectorLayer(std::shared_ptr<DataType> targetData, const std::string& requestedLayerName)
{
  if (!targetData) { return nullptr; }

  std::string layerName = requestedLayerName;

  if (auto gdalData = std::dynamic_pointer_cast<GDALDataType>(targetData)) {
    // Fallback: Find the first Vector layer if none requested
    if (layerName.empty()) {
      auto catalog = gdalData->getCatalog();
      for (const auto& entry : catalog) {
        if (entry.type == "Vector") {
          layerName = entry.name;
          break;
        }
      }
    }

    if (!layerName.empty()) {
      auto genericLayer = gdalData->getLayer(layerName);
      auto vectorData   = std::dynamic_pointer_cast<VectorDataType>(genericLayer);
      if (!vectorData) {
        fLogSevere("WebGUI: Requested layer '{}' is not a valid Vector layer.", layerName);
      }
      return vectorData;
    }
  }

  // Direct fallback if the target data IS the vector layer directly
  return std::dynamic_pointer_cast<VectorDataType>(targetData);
}

// A tiny struct to hold our binary image and its MIME type
struct TilePayload {
  std::vector<char> data;
  std::string       mimeType;
};

// Thread-Safe L1 RAM Cache
class TileLRUCache {
private:
  size_t capacity;
  std::mutex cacheMutex;
  std::list<std::string> lruList; // Tracks the least recently used keys
  std::unordered_map<std::string, std::pair<TilePayload, std::list<std::string>::iterator> > cacheMap;

public:
  TileLRUCache(size_t cap) : capacity(cap){ }

  bool
  get(const std::string& key, TilePayload& outPayload)
  {
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto it = cacheMap.find(key);

    if (it == cacheMap.end()) { return false; }

    // Cache Hit: Move this key to the front of the list (Most Recently Used)
    lruList.splice(lruList.begin(), lruList, it->second.second);
    outPayload = it->second.first;
    return true;
  }

  void
  put(const std::string& key, const TilePayload& payload)
  {
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto it = cacheMap.find(key);

    if (it != cacheMap.end()) {
      // Update existing entry and move to front
      lruList.splice(lruList.begin(), lruList, it->second.second);
      it->second.first = payload;
      return;
    }

    // Add new entry. If we hit the cap, evict the oldest tile!
    if (cacheMap.size() >= capacity) {
      std::string oldest = lruList.back();
      lruList.pop_back();
      cacheMap.erase(oldest);
    }

    lruList.push_front(key);
    cacheMap[key] = { payload, lruList.begin() };
  }
};

// Instantiate the global RAM cache (1000 tiles is roughly 20-50MB of RAM)
TileLRUCache g_tileCache(1000);
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
      fLogSevere("Checking subtype {}", full);
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
  fLogSevere("Checking root: {}", rootdisk);

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

std::string
RAPIOWebGUI::resolveDatasetId(const WebMessage& w)
{
  // 1. Explicitly requested via query parameter (e.g., ?dataset=my_file.nc)
  if (w.getMap().count("dataset") && !w.getMap().at("dataset").empty()) {
    return w.getMap().at("dataset");
  }

  // 2. Fallback to the initial startup file (-i argument)
  if (!myStartUpFile.empty()) {
    return myStartUpFile;
  }

  // 3. Fallback to the first available dataset in the memory cache
  std::lock_guard<std::mutex> lock(myCacheMutex);

  if (!myDataCache.empty()) {
    return myDataCache.begin()->first;
  }

  // No dataset found
  return "";
}

void
RAPIOWebGUI::handlePathUI(WebMessage& w, std::vector<std::string>& pieces)
{
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
      fLogSevere("Getting subtypes for {}", s);
      w.setMessage(getJSONSubtypeList(s), "application/json");
    } else {
      const std::string s = pieces[1];
      const std::string t = pieces[2];
      fLogSevere("Getting times for {}", s);
      w.setMessage(getJSONTimeList(s, t), "application/json");
    }
  }
  return;
}

void
RAPIOWebGUI::handleColorMap(WebMessage& w, std::vector<std::string>& pieces, std::map<std::string,
  std::string>& settings)
{
  std::string datasetId = resolveDatasetId(w);
  auto targetData       = getOrLoadDataset(datasetId);

  if (targetData != nullptr) {
    // This assumes the loaded data which at some point we're gonna have to change.
    // We might want to get color map by NAME actually.
    auto c     = targetData->getColorMap();
    auto units = targetData->getUnits();
    std::stringstream json;
    // Bleh need a json color map, right?  So we can apply it on front end side
    // Actually no, it can be a binary format.  It's just less flexible.
    // Also the web guys will want an svg generator as well.  Lots of color map work
    // to do.
    // Needs to be UTF-8 for text/plain.j
    // w.setMessage("This should work!", "text/plain");
    // We probably want a json version and a binary version.  Hate to say it.
    //
    c->toJSON(json, units);
    w.setMessage(json.str());
  } else {
    w.setError(400);
  }
}

void
RAPIOWebGUI::handleSVG(WebMessage& w, std::vector<std::string>& pieces, std::map<std::string,
  std::string>& settings)
{
  std::string datasetId = resolveDatasetId(w);
  auto targetData       = getOrLoadDataset(datasetId);

  if (targetData != nullptr) {
    // This assumes the loaded data which at some point we're gonna have to change.
    // We might want to get color map by NAME actually.
    auto c     = targetData->getColorMap();
    auto units = targetData->getUnits();
    std::stringstream svg;

    // Creating SVG on the fly, though guess we 'could' have a writer for it and cache files of them.
    c->toSVG(svg, units);
    w.setMessage(svg.str());
  } else {
    w.setError(400);
  }
}

void
RAPIOWebGUI::serveTile(WebMessage& w, std::shared_ptr<DataType> targetData, std::string& pathout, std::map<std::string,
  std::string>& settings)
{
  TilePayload payload;
  std::string suffix = settings["suffix"];

  // Resolve MIME type
  std::string mimeType = "image/" + suffix;

  if ((suffix == "jpg") || (suffix == "jpeg")) { mimeType = "image/jpeg"; } else if (suffix == "mrmstile") {
    mimeType = "application/octet-stream";
  } else if ((suffix == "pbf") || (suffix == "mvt") ) {
    mimeType = "application/vnd.mapbox-vector-tile";
  } else if ((suffix == "geojson") || (suffix == "json") ) { mimeType = "application/geo+json"; }
  fLogInfo("----->MIME TYPE IS {}", mimeType);

  // ==========================================
  // TIER 1: Check RAM Cache (L1)
  // ==========================================
  if (g_tileCache.get(pathout, payload)) {
    w.setMessage(std::string(payload.data.begin(), payload.data.end()), payload.mimeType);
    w.setError(200);
    return;
  }

  // ==========================================
  // TIER 2: Check Disk Cache (L2)
  // ==========================================
  if (OS::isRegularFile(pathout)) {
    std::ifstream file(pathout, std::ios::binary);
    if (file) {
      payload.data.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
      payload.mimeType = mimeType;
      g_tileCache.put(pathout, payload);
      w.setMessage(std::string(payload.data.begin(), payload.data.end()), payload.mimeType);
      w.setError(200);
      return;
    }
  }

  // ==========================================
  // TIER 3: Cache Miss (Generate the Tile)
  // ==========================================
  std::vector<char> tileBuffer;

  // -- VECTOR TILE GENERATION --
  if ((suffix == "pbf") || (suffix == "mvt") || (suffix == "geojson") || (suffix == "json")) {
    std::string layerName = settings.count("layer") ? settings["layer"] : "";
    auto vectorData       = getCachedVectorLayer(targetData, layerName);

    // If we asked for vector data but have raster data, return gracefully
    if (!vectorData) {
      w.setError(204);
      return;
    }

    double minLon = 0, minLat = 0, maxLon = 0, maxLat = 0;
    int x = 0, y = 0, z = 0;

    // Extract coordinates from settings
    if (settings.count("x") && settings.count("y") && settings.count("z")) {
      x      = std::stoi(settings["x"]);
      y      = std::stoi(settings["y"]);
      z      = std::stoi(settings["z"]);
      minLon = tilex2long(x, z);
      maxLat = tiley2lat(y, z);
      maxLon = tilex2long(x + 1, z);
      minLat = tiley2lat(y + 1, z);
    } else if (settings.count("BBOX")) {
      std::vector<std::string> bbox;
      Strings::splitWithoutEnds(settings["BBOX"], ',', &bbox);
      if (bbox.size() == 4) {
        minLon = std::stod(bbox[0]);
        minLat = std::stod(bbox[1]);
        maxLon = std::stod(bbox[2]);
        maxLat = std::stod(bbox[3]);
      }
    }

    std::string vData = (suffix == "pbf" || suffix == "mvt") ?
      vectorData->getTileMVT(minLon, minLat, maxLon, maxLat, z, x, y) :
      vectorData->getTileGeoJSON(minLon, minLat, maxLon, maxLat);

    if (vData.empty() || (vData == "{}") ) {
      w.setError(204); // No Content (empty tile)
      return;
    }
    tileBuffer.assign(vData.begin(), vData.end());
  }
  // -- RASTER TILE GENERATION --
  else {
    auto tileGrid = DataProjection::createResampledTile(targetData, settings);

    // If we asked for raster data but have vector data (or out of bounds), return gracefully
    if (!tileGrid) {
      w.setError(204);
      return;
    }

    settings["index"] = "true";
    size_t bytes = IODataType::writeBuffer(tileGrid, tileBuffer, settings, "image");
    if (bytes == 0) {
      w.setError(204);
      return;
    }
  }

  // ==========================================
  // Finalize payload, serve to browser, and cache it
  // ==========================================
  payload.data     = std::move(tileBuffer);
  payload.mimeType = mimeType;

  w.setMessage(std::string(payload.data.begin(), payload.data.end()), payload.mimeType);
  w.setError(200);

  g_tileCache.put(pathout, payload);

  try {
    size_t lastSlash = pathout.find_last_of('/');
    if (lastSlash != std::string::npos) {
      OS::mkdirp(pathout.substr(0, lastSlash));
    }
    std::ofstream outFile(pathout, std::ios::binary);
    if (outFile) { outFile.write(payload.data.data(), payload.data.size()); }
  } catch (const std::exception& e) {
    fLogSevere("WebGUI: Failed to write disk cache for {}: {}", pathout, e.what());
  }
} // RAPIOWebGUI::serveTile

void
RAPIOWebGUI::handlePathWMS(WebMessage& w, std::vector<std::string>& pieces, std::map<std::string,
  std::string>& settings)
{
  // 1. Determine which dataset the frontend is requesting
  std::string datasetId = resolveDatasetId(w);

  // 2. Fetch it from our thread-safe cache
  auto targetData = getOrLoadDataset(datasetId);

  if (!targetData) {
    w.setError(400); // Bad Request (Dataset not found)
    return;
  }

  std::string cache = settings["tilecachefolder"];
  std::vector<std::string> bbox;

  Strings::splitWithoutEnds(settings["BBOX"], ',', &bbox);
  if (bbox.size() != 4) {
    fLogSevere("Expected 4 parameters in BBOX for WMS server request, got {}", bbox.size());
    w.setError(400);
    return;
  }
  for (auto a:bbox) {
    try{
      std::stof(a);
    }catch (const std::exception& e) {
      fLogSevere("Non number passed for BBOX parameter? {}", a);
      w.setError(400);
      return;
    }
  }

  std::string suffix = settings["suffix"];
  std::string safeDatasetName = OS::validatePathCharacters(datasetId);

  // 2. Build dataset-specific WMS path
  std::string pathout = cache + "/wms/" + safeDatasetName + "/T" + bbox[0] + "/T" + bbox[1] + "/T" + bbox[2] + "/T"
    + bbox[3] + "/tile." + suffix;

  settings["TILETEXT"] = "";
  if (w.getMap().count("layer")) { settings["layer"] = w.getMap().at("layer"); }

  // 3. Serve with target data
  serveTile(w, targetData, pathout, settings);
} // RAPIOWebGUI::handlePathWMS

void
RAPIOWebGUI::handlePathMVT(WebMessage& w, std::vector<std::string>& pieces, std::map<std::string,
  std::string>& settings)
{
  // 1. Determine which dataset the frontend is requesting
  std::string datasetId = resolveDatasetId(w);

  // 2. Fetch it from our thread-safe cache
  auto targetData = getOrLoadDataset(datasetId);

  if (!targetData) {
    w.setError(400); // Bad Request (Dataset not found)
    return;
  }

  double minLon, minLat, maxLon, maxLat;
  int x, y, z;

  if (!getTileBBoxFromPath(pieces, minLon, minLat, maxLon, maxLat, x, y, z)) {
    w.setError(400);
    return;
  }

  settings["suffix"] = "pbf";
  // Keep caches separated by dataset!
  std::string pathout = settings["tilecachefolder"] + "/mvt/" + datasetId + "/"
    + std::to_string(z) + "/" + std::to_string(x) + "/" + std::to_string(y) + ".pbf";

  settings["x"] = std::to_string(x);
  settings["y"] = std::to_string(y);
  settings["z"] = std::to_string(z);
  if (w.getMap().count("layer")) { settings["layer"] = w.getMap().at("layer"); }

  // Pass the targetData into serveTile
  serveTile(w, targetData, pathout, settings);
} // RAPIOWebGUI::handlePathMVT

void
RAPIOWebGUI::handlePathTMS(WebMessage& w, std::vector<std::string>& pieces, std::map<std::string,
  std::string>& settings)
{
  // 1. Determine which dataset the frontend is requesting
  std::string datasetId = resolveDatasetId(w);

  // 2. Fetch it from our thread-safe cache
  auto targetData = getOrLoadDataset(datasetId);

  if (!targetData) {
    w.setError(400); // Bad Request (Dataset not found)
    return;
  }

  double minLon, minLat, maxLon, maxLat;
  int x, y, z;

  if (!getTileBBoxFromPath(pieces, minLon, minLat, maxLon, maxLat, x, y, z)) {
    w.setError(400); // Bad Request (Invalid tile coordinates)
    return;
  }

  // 3. Prevent disk-cache collisions: Build a unique path per dataset.
  // Note: If datasetId is a full absolute path (e.g., "/home/user/data.nc"),
  // you might want to sanitize it (e.g., hash it or replace '/' with '_')
  // so it doesn't create weird nested folders in your cache directory.
  std::string safeDatasetName = OS::validatePathCharacters(datasetId);

  std::string pathout = settings["tilecachefolder"] + "/tms/" + safeDatasetName + "/"
    + std::to_string(z) + "/" + std::to_string(x) + "/" + std::to_string(y) + "." + settings["suffix"];

  settings["BBOX"] = std::to_string(minLon) + "," + std::to_string(minLat) + "," + std::to_string(maxLon) + ","
    + std::to_string(maxLat);
  settings["BBOXSR"] = "4326";
  settings["x"]      = std::to_string(x);
  settings["y"]      = std::to_string(y);
  settings["z"]      = std::to_string(z);

  // Optional: If the user requested a specific sub-layer (like a GDAL raster band)
  if (w.getMap().count("layer")) { settings["layer"] = w.getMap().at("layer"); }

  // 4. Pass the explicitly loaded dataset into the serving logic
  serveTile(w, targetData, pathout, settings);
} // RAPIOWebGUI::handlePathTMS

void
RAPIOWebGUI::handlePathGeoJSON(WebMessage& w, std::vector<std::string>& pieces, std::map<std::string,
  std::string>& settings)
{
  // 1. Determine which dataset the frontend is requesting
  std::string datasetId = resolveDatasetId(w);

  // 2. Fetch it from our thread-safe cache
  auto targetData = getOrLoadDataset(datasetId);

  if (!targetData) {
    w.setError(400); // Bad Request (Dataset not found)
    return;
  }

  double minLon, minLat, maxLon, maxLat;
  int x, y, z;

  if (!getTileBBoxFromPath(pieces, minLon, minLat, maxLon, maxLat, x, y, z)) {
    w.setError(400);
    return;
  }

  settings["suffix"] = "geojson";
  std::string safeDatasetName = OS::validatePathCharacters(datasetId);

  // 2. Build dataset-specific GeoJSON cache path
  std::string pathout = settings["tilecachefolder"] + "/geojson/" + safeDatasetName + "/"
    + std::to_string(z) + "/" + std::to_string(x) + "/" + std::to_string(y) + ".geojson";

  settings["x"] = std::to_string(x);
  settings["y"] = std::to_string(y);
  settings["z"] = std::to_string(z);
  if (w.getMap().count("layer")) { settings["layer"] = w.getMap().at("layer"); }

  // 3. Serve with target data
  serveTile(w, targetData, pathout, settings);
} // RAPIOWebGUI::handlePathGeoJSON

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
    fLogSevere("HOSTNAME is {} and {}", hostname, portStr);
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
      // else if (trimS == "application/vnd.mapbox-vector-tile") trimS = "pbf"; // Vector!
      //      else if (trimS == "application/geo+json") trimS = "geojson";

      Strings::removePrefix(trimS, "image/");
      settings["suffix"] = trimS;
    } else if (a.first == "depth") {
      settings["depth"] = a.second; // Pass the 8/16 bit depth setting along!
    }
  }
  if (settings["suffix"] == "") {
    settings["suffix"] = "png";
  }
  if ((settings["suffix"] == "png") && (settings.count("quality") == 0)) { settings["quality"] = "10"; }
} // RAPIOWebGUI::handleOverrides

void
RAPIOWebGUI::processWebMessage(std::shared_ptr<WebMessage> wsp)
{
  fLogInfo("Request: {}", wsp->getPath());
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
      fLogInfo("Received capabilities request but can't find webguicapabilities.xml");
      w.setMessage("Error");
      return;
    } else {
      fLogInfo("Request for capabilities succeeded.");
    }

    std::ifstream file(url.toString());
    std::ostringstream buffer;
    buffer << file.rdbuf();
    w.setMessage(buffer.str(), "application/xml");
    return;
  } else {
    bool service = (settings["service"] == "WMS");
    // fLogInfo("Settings?  {}", service);
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
  } else if (type == "mvt") {
    // handlePathVectorTMS(w, pieces, settings);
    handlePathMVT(w, pieces, settings);
  } else if (type == "geojson") {
    handlePathGeoJSON(w, pieces, settings);
  } else if (type == "colormap") {
    handleColorMap(w, pieces, settings);
  } else if (type == "svg") {
    handleSVG(w, pieces, settings);
  } else if (type == "proxy") {
    handleProxy(w, pieces);
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
RAPIOWebGUI::handleProxy(WebMessage& w, std::vector<std::string>& pieces)
{
  auto params = w.getMap();

  if (params.count("url") == 0) {
    std::cout << "Returning on no url error\n";
    w.setMessage("Error: No 'url' parameter provided", "text/plain");
    w.setError(400);
    return;
  }

  std::string targetUrl = params.at("url");
  std::vector<char> buffer;

  std::cout << "Reading '" << targetUrl << "'\n";
  // Fetch the data server-side (bypasses browser CORS)
  int bytesRead = Network::read(targetUrl, buffer);

  std::cout << "Read '" << targetUrl << "' size is " << bytesRead << "\n";
  if (bytesRead > 0) {
    std::string payload(buffer.begin(), buffer.end());

    // Quick MIME type guess so the browser handles the image correctly
    std::string mime = "application/octet-stream";
    if (Strings::endsWith(targetUrl, ".png")) {
      mime = "image/png";
    } else if (Strings::endsWith(targetUrl, ".jpg") || Strings::endsWith(targetUrl, ".jpeg")) { mime = "image/jpeg"; }

    // setMessage ensures it goes through the CORS-friendly branch in rWebServer.cc
    w.setMessage(payload, mime);
  } else {
    w.setMessage("Error: Failed to fetch target URL", "text/plain");
    w.setError(502); // Bad Gateway
  }
} // RAPIOWebGUI::handleProxy

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
    std::string aBuilder;
    std::string aFile;
    IODataType::iPathParse(myStartUpFile, aFile, aBuilder);
    fLogInfo("Attempting to load given start up file {}", aFile);
    auto d = IODataType::readDataType(aFile, aBuilder);

    if (d != nullptr) {
      // FIX: Put it in the new cache instead of myTileData!
      std::lock_guard<std::mutex> lock(myCacheMutex);
      myDataCache[myStartUpFile] = d;
    }
  }

  // Time until end of program.  Make it dynamic memory instead of heap or
  // the compiler will kill it too early (too smartz)
  static std::shared_ptr<ProcessTimer> fulltime(
    new ProcessTimer("Web server total runtime"));

  EventLoop::doEventLoop();
} // RAPIOWebGUI::execute

std::shared_ptr<DataType>
RAPIOWebGUI::getOrLoadDataset(const std::string& layerId)
{
  // 1. Guard against empty requests
  if (layerId.empty()) {
    fLogSevere("WebGUI: No dataset requested, no startup file, and cache is empty.");
    return nullptr;
  }

  // 2. Check memory cache first (Thread-safe read)
  {
    std::lock_guard<std::mutex> lock(myCacheMutex);
    auto it = myDataCache.find(layerId);
    if (it != myDataCache.end()) {
      return it->second;
    }
  }

  // 3. If not in cache, load it from disk
  std::string aBuilder;
  std::string aFile;

  IODataType::iPathParse(layerId, aFile, aBuilder);

  fLogInfo("WebGUI: Cache miss. Lazy-loading dataset '{}'", aFile);
  auto dataset = IODataType::readDataType(aFile, aBuilder);

  // 4. Store successful loads in the cache (Thread-safe write)
  if (dataset != nullptr) {
    std::lock_guard<std::mutex> lock(myCacheMutex);
    myDataCache[layerId] = dataset;
    return dataset;
  }

  // 5. Fail gracefully
  fLogSevere("WebGUI: Failed to load requested dataset: {}", layerId);
  return nullptr;
} // RAPIOWebGUI::getOrLoadDataset

int
main(int argc, char * argv[])
{
  // Create and run a tile alg
  RAPIOWebGUI alg = RAPIOWebGUI();

  alg.executeFromArgs(argc, argv);
}
