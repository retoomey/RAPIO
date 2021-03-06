#include "rDataProjection.h"

#include "rRadialSet.h"
#include "rRadialSetLookup.h"
#include "rLatLonGrid.h"
#include "rProject.h"
#include "rStrings.h"

#include <iostream>

using namespace rapio;
using namespace std;

ostream&
rapio::operator << (ostream& os, const LLCoverage& p)
{
  os << "mode: " << p.mode << ", cols: " << p.cols << ", rows: " << p.rows;
  if (p.mode == "full") { } else if (p.mode == "degrees") {
    os << ", degreeOut: " << p.degreeOut;
  } else if (p.mode == "tile") {
    os << ", zoomlevel: " << p.zoomlevel << ", centerLatDegs: " << p.centerLatDegs
       << ", centerLonDegs: " << p.centerLonDegs << ", deltaLatDegs: " << p.deltaLatDegs
       << ", deltaLonDegs: " << p.deltaLonDegs;
  }

  return (os);
}

bool
DataProjection::getBBOX(
  std::map<std::string, std::string>& keys,
  size_t& rows,
  size_t& cols,
  std::string& bbox,
  std::string& bboxsr)
{
  // Look for the standard bbox, bboxsr, rows, cols
  bbox   = keys["BBOX"];
  bboxsr = keys["BBOXSR"];
  std::string mode = keys["mode"];
  if (mode.empty()) {
    mode         = "tile";
    keys["mode"] = mode;
  }

  try{
    rows = std::stoi(keys["rows"]); // could except
    cols = std::stoi(keys["cols"]);
  }catch (const std::exception& e) {
    LogSevere("Unrecognized rows/cols settings, using defaults\n");
    rows = 256;
    cols = 256;
  }

  // ----------------------------------------------------------------------
  // Calculate bounding box for generating image
  //

  // Check if center, width, height set, create bbox from extent?
  if (bbox.empty()) { // New auto tile mode
    // Try zoom, center ability based on web mercator
    size_t zoom;
    double centerLatDegs, centerLonDegs;

    try{
      zoom = std::stoi(keys["zoom"]);
      centerLatDegs = std::stod(keys["centerLatDegs"]);
      centerLonDegs = std::stod(keys["centerLonDegs"]);
    }catch (const std::exception& e) {
      LogSevere("Require zoom, centerLatDegs and centerLonDegs to create image.\n");
      return false;
    }

    // MRMS tile math for moment for 'giant' tiles.
    // Probably giving up on this for a while...bounding box works best
    // for generating images, not zoom/centers
    zoom += 2;
    const double deg_to_rad = (2.0 * M_PI) / 360.0; // should be in math somewhere
    const double rad_to_deg = 360 / (2.0 * M_PI);
    const double powz       = pow(2, zoom);
    const size_t pix_c      = 256 * powz;
    const double pix_radius = 256 * powz / (2.0 * M_PI);

    double raw_lonW = (centerLonDegs + 360 * ((-1.0 * cols / 2.0) / pix_c));
    double raw_lonE = (centerLonDegs + 360 * ((1.0 * cols / 2.0) / pix_c));
    if (raw_lonW > 180) {    raw_lonW = raw_lonW - 360; }
    if (raw_lonW <= -180) {  raw_lonW = raw_lonW + 360; }
    if (raw_lonE > 180) {    raw_lonE = raw_lonE - 360; }
    if (raw_lonE <= -180) {  raw_lonE = raw_lonE + 360; }

    double cLatRad       = centerLatDegs * deg_to_rad;
    double old_pix_to_eq = pix_radius * log((1.0 + sin(cLatRad)) / cos(cLatRad));
    double pixN     = rows / 2.0;
    double raw_latN = (rad_to_deg) * (2.0 * atan(pow(M_E, (old_pix_to_eq + pixN) / pix_radius))) - 90.0;
    double raw_latS = (rad_to_deg) * (2.0 * atan(pow(M_E, (old_pix_to_eq - pixN) / pix_radius))) - 90.0;

    bbox = std::to_string(raw_lonW) + "," + std::to_string(raw_latS) + ","
      + std::to_string(raw_lonE) + "," + std::to_string(raw_latN);
    bboxsr = "4326"; // translate to web merc
  } else {
    LogInfo(
      "Image writer tile:" << bbox << "\n");
  }

  return false;
} // DataProjection::getBBOX

bool
DataProjection::getLLCoverage(const PTreeNode& fields, LLCoverage& c)
{
  // Retiring?

  // Factory for reading our XML projection settings
  // a bit messy this first pass, probably will refactor more
  bool optionSuccess = false;

  // Fallbacks
  std::string mode = c.mode;
  size_t cols      = c.cols;
  size_t rows      = c.rows;
  try{
    mode   = fields.getAttr("mode", mode);
    c.mode = mode;
    cols   = fields.getAttr("cols", cols);
    rows   = fields.getAttr("rows", rows);
  }catch (const std::exception& e) {
    LogSevere("Unrecognized settings, using defaults\n");
  }

  float top, left, deltaLat, deltaLon;
  if (mode == "full") {
    optionSuccess = LLCoverageFull(rows, cols, top, left, deltaLat, deltaLon);
    // Calculated back.  Full will generate the rows/cols based on data resolution
    c.rows         = rows;
    c.cols         = cols;
    c.topDegs      = top;
    c.leftDegs     = left;
    c.deltaLatDegs = deltaLat;
    c.deltaLonDegs = deltaLon;
  } else if (mode == "degrees") {
    // Extra settings for degrees mode
    size_t degreeOut = c.degreeOut;
    try{
      degreeOut = fields.getAttr("degrees", size_t(degreeOut));
    }catch (const std::exception& e) {
      LogSevere("Missing degrees, using default of " << degreeOut << "\n");
    }
    optionSuccess  = LLCoverageCenterDegree(degreeOut, rows, cols, top, left, deltaLat, deltaLon);
    c.rows         = rows;
    c.cols         = cols;
    c.topDegs      = top;
    c.leftDegs     = left;
    c.deltaLatDegs = deltaLat;
    c.deltaLonDegs = deltaLon;
    // Extra
    c.degreeOut = degreeOut;
  } else if (mode == "tile") {
    // Extra settings for zoom mode
    size_t zoomLevel    = c.zoomlevel;
    float centerLatDegs = 35.22; // Norman, OK
    float centerLonDegs = -97.44;
    try{
      zoomLevel     = fields.getAttr("zoom", size_t(zoomLevel));
      centerLatDegs = fields.getAttr("centerLatDegs", float(centerLatDegs));
      centerLonDegs = fields.getAttr("centerLonDegs", float(centerLonDegs));
    }catch (const std::exception& e) {
      LogSevere("Unrecognized settings, using defaults\n");
    }
    optionSuccess = LLCoverageTile(zoomLevel, rows, cols,
        centerLatDegs, centerLonDegs, top, left, deltaLat, deltaLon);
    // Calculated back:
    c.rows         = rows;
    c.cols         = cols;
    c.topDegs      = top;
    c.leftDegs     = left;
    c.deltaLatDegs = deltaLat;
    c.deltaLonDegs = deltaLon;
    // Extra
    c.zoomlevel     = zoomLevel;
    c.centerLatDegs = centerLatDegs;
    c.centerLonDegs = centerLonDegs;
  } else {
    LogSevere("Unknown projection mode " << mode << " specified.\n")
  }

  if (!optionSuccess) {
    LogSevere("Don't know how to create a coverage grid for this datatype using mode '" << mode << "'.\n");
  }
  return optionSuccess;
} // DataProjection::getLLCoverage

RadialSetProjection::RadialSetProjection(const std::string& layer, RadialSet * owner)
{
  auto& r = *owner;

  my2DLayer = r.getFloat2D(layer.c_str());

  // We can cache for getting data values I 'think'
  if (r.myLookup == nullptr) {
    r.myLookup = std::make_shared<RadialSetLookup>(r); // Bin azimuths.
  }
  myLookup        = r.myLookup;
  myCenterLatDegs = r.myLocation.getLatitudeDeg();
  myCenterLonDegs = r.myLocation.getLongitudeDeg();
}

double
RadialSetProjection::getValueAtLL(double latDegs, double lonDegs, const std::string& layer)
{
  // Translate from Lat Lon to az/range
  float azDegs, rangeMeters;
  int radial, gate;

  Project::LatLonToAzRange(myCenterLatDegs, myCenterLonDegs, latDegs, lonDegs, azDegs, rangeMeters);
  if (myLookup->getRadialGate(azDegs, rangeMeters, &radial, &gate)) {
    const auto& data = my2DLayer->ref();
    return data[radial][gate];
  } else {
    return Constants::MissingData;
  }
}

bool
RadialSetProjection::LLCoverageCenterDegree(const float degreeOut, const size_t numRows, const size_t numCols,
  float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs)
{
  Project::createLatLonGrid(myCenterLatDegs, myCenterLonDegs, degreeOut, numRows, numCols, topDegs, leftDegs,
    deltaLatDegs, deltaLonDegs);
  return true;
}

bool
RadialSetProjection::LLCoverageFull(size_t& numRows, size_t& numCols,
  float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs)
{
  // There isn't really a 'full' for radialset...so use some defaults
  numRows = 1000;
  numCols = 1000;
  double degreeOut = 10;

  // Our center location is the standard location
  Project::createLatLonGrid(myCenterLatDegs, myCenterLonDegs, degreeOut, numRows, numCols, topDegs, leftDegs,
    deltaLatDegs, deltaLonDegs);
  return true;
}

LatLonGridProjection::LatLonGridProjection(const std::string& layer, LatLonGrid * owner)
{
  // Grab everything we need from the LatLonGrid
  // Note changing LatLonGrid, etc. will invalidate the projection
  auto& l = *owner;

  my2DLayer    = l.getFloat2D(layer.c_str());
  myLatNWDegs  = l.myLocation.getLatitudeDeg();
  myLonNWDegs  = l.myLocation.getLongitudeDeg();
  myLatSpacing = l.myLatSpacing;
  myLonSpacing = l.myLonSpacing;
  myNumLats    = l.getNumLats();
  myNumLons    = l.getNumLons();
}

double
LatLonGridProjection::getValueAtLL(double latDegs, double lonDegs, const std::string& layer)
{
  // Try to do this quick and efficient, called a LOT
  const double x = (myLatNWDegs - latDegs) / myLatSpacing;

  if ((x < 0) || (x > myNumLats)) {
    return Constants::MissingData;
  }
  const double y = (lonDegs - myLonNWDegs) / myLonSpacing;
  if ((y < 0) || (y > myNumLons)) {
    return Constants::MissingData;
  }
  const auto& data = my2DLayer->ref();
  return data[int(x)][int(y)];
}

bool
LatLonGridProjection::LLCoverageCenterDegree(const float degreeOut, const size_t numRows, const size_t numCols,
  float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs)
{
  // location is the top left, find the absolute center (for x, y > 0)
  const auto lonc = myLonNWDegs + (myLonSpacing * myNumLons / 2.0);
  const auto latc = myLatNWDegs - (myLatSpacing * myNumLats / 2.0);

  Project::createLatLonGrid(latc, lonc, degreeOut, numRows, numCols, topDegs, leftDegs, deltaLatDegs, deltaLonDegs);
  return true;
}

bool
LatLonGridProjection::LLCoverageFull(size_t& numRows, size_t& numCols,
  float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs)
{
  numRows      = myNumLats;
  numCols      = myNumLons;
  topDegs      = myLatNWDegs;
  leftDegs     = myLonNWDegs;
  deltaLatDegs = -myLatSpacing;
  deltaLonDegs = myLonSpacing;
  return true;
}

bool
LatLonGridProjection::LLCoverageTile(
  const size_t zoomLevel, const size_t& numRows, const size_t& numCols,
  const float centerLatDegs, const float centerLonDegs,
  float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs)
{
  // Zoom level is based on the earth size
  // https://wiki.openstreetmap.org/wiki/Zoom_levels
  // Open street map uses earth radius of 6372.7982 km

  // Use half degree calculation as the 'degreeout'
  // Ok this should be correct even for spherical mercator..since the east-west
  // matches geodetic.  North-south is where the stretching is.
  const double degWidth  = 360.0 / pow(2, zoomLevel); // lol I wrote 2 ^ zoomlevel which is not c++
  const double halfDeg   = degWidth / 2.0;
  const double degreeOut = halfDeg;

  //  Project::createLatLonGrid(centerLatDegs, centerLonDegs, degreeOut, numRows, numCols, topDegs, leftDegs, deltaLatDegs,
  //    deltaLonDegs);
  auto lon = centerLonDegs;

  // These 'should' project back to x1/x2
  leftDegs = lon - degreeOut;
  auto rightDegs = lon + degreeOut;

  // All these are meaningless I think...we need to project left and
  // right to X, then use _that_ width I think
  auto width = rightDegs - leftDegs;
  deltaLonDegs = width / numCols;

  // To keep aspect ratio per cell, use deltaLon to back calculate
  deltaLatDegs = -deltaLonDegs; // keep the same square per pixel
  auto lat = centerLatDegs;

  topDegs = lat - (deltaLatDegs * numRows / 2.0);

  return true;
} // LatLonGridProjection::LLCoverageTile
