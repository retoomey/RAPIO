#include "rDataProjection.h"

#include "rRadialSet.h"
#include "rLatLonGrid.h"
#include "rLatLonHeightGrid.h"
#include "rStrings.h"
#include "rProject.h"

#include <iostream>

using namespace rapio;
using namespace std;

// FIXME: Bunch of classes here...I'm still finalized how this part works
// and I want to refactor before dividing up into final files/classes.
//
std::shared_ptr<ProjLibProject> DataProjection::theWebMercToLatLon = nullptr;

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

std::shared_ptr<ProjLibProject>
DataProjection::getBBOX(
  std::map<std::string, std::string>& keys,
  size_t                            & rows,
  size_t                            & cols,
  double                            & left,
  double                            & bottom,
  double                            & right,
  double                            & top)
{
  left = bottom = right = top = 0;

  // Look for the standard bbox, bboxsr, rows, cols
  std::string bbox   = keys["BBOX"];
  std::string bboxsr = keys["BBOXSR"];

  // Is this used?
  // std::string mode = keys["mode"];
  // if (mode.empty()) {
  //  mode         = "tile";
  //  keys["mode"] = mode;
  // }

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
    // FIXME: I should rewrite this to use mercator meters from the 'center'
    // to create the bounding box.  Currently I'm focused on web tile generation
    // which typically wants webmerc

    // Try zoom, center ability based on web mercator
    size_t zoom;
    double centerLatDegs, centerLonDegs;

    try{
      zoom = std::stoi(keys["zoom"]);
      centerLatDegs = std::stod(keys["centerLatDegs"]);
      centerLonDegs = std::stod(keys["centerLonDegs"]);
    }catch (const std::exception& e) {
      LogSevere("Require zoom, centerLatDegs and centerLonDegs to create image.\n");
      return nullptr;
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

    bboxsr = "4326"; // Note converted to webmerc below?  Should we?
    left   = raw_lonW;
    bottom = raw_latS;
    right  = raw_lonE;
    top    = raw_latN;
  } else {
    // FIXME: more checks?
    std::vector<std::string> pieces;
    Strings::splitWithoutEnds(bbox, ',', &pieces);
    if (pieces.size() == 4) {
      left   = std::stod(pieces[0]); // lon
      bottom = std::stod(pieces[1]);
      right  = std::stod(pieces[2]);
      top    = std::stod(pieces[3]);
    } else {
      LogSevere("Malformed BBOX? " << bbox << "\n");
      return nullptr;
    }

    // For the tile engine...
    // We need to march in webmerc or distortions will get too big when zooming out,
    // marching in LatLon is equal angle but webmerc is conformal
    if (theWebMercToLatLon == nullptr) {
      theWebMercToLatLon = std::make_shared<ProjLibProject>(
        "+proj=webmerc +datum=WGS84 +units=m +resolution=1", // Web mercator
        "+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs"   // Lat Lon
      );
      theWebMercToLatLon->initialize();
    }

    if (bboxsr == "4326") {
      // Box is in Lat Lon, we want web mercator marching
      // Note: lat/lon is swapped order...probably should change it
      // 'Could' just have in/out share..humm
      theWebMercToLatLon->LatLonToXY(bottom, left, left, bottom);
      theWebMercToLatLon->LatLonToXY(top, right, right, top);
      return theWebMercToLatLon;
    } else if (bboxsr == "3857") { // Coordinates are already in webmerc
      return theWebMercToLatLon;
    } else {
      LogSevere("Unknown projection requested: " << bboxsr << "\n");
    }
  }

  return nullptr;
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

LatLonGridProjection::LatLonGridProjection(const std::string& layer, LatLonGrid * owner) : DataProjection(layer)
{
  // Grab everything we need from the LatLonGrid
  // Note changing LatLonGrid, etc. will invalidate the projection
  auto& l = *owner;

  // Weak pointer to layer to use.  We only exist as long as RadialSet is valid
  my2DLayer = l.getFloat2D(layer)->ptr();
  // my2DLayer    = l.getFloat2D(layer.c_str());
  myLatNWDegs  = l.myLocation.getLatitudeDeg();
  myLonNWDegs  = l.myLocation.getLongitudeDeg();
  myLatSpacing = l.myLatSpacing;
  myLonSpacing = l.myLonSpacing;
  myNumLats    = l.getNumLats();
  myNumLons    = l.getNumLons();
}

LatLonHeightGridProjection::LatLonHeightGridProjection(const std::string& layer,
  LatLonHeightGrid *                                                    owner) : DataProjection(layer)
{
  // Grab everything we need from the LatLonGrid
  // Note changing LatLonGrid, etc. will invalidate the projection
  auto& l = *owner;

  my3DLayer = l.getFloat3D(layer)->ptr();
  // my3DLayer    = l.getFloat3D(layer.c_str());
  myLatNWDegs  = l.myLocation.getLatitudeDeg();
  myLonNWDegs  = l.myLocation.getLongitudeDeg();
  myLatSpacing = l.myLatSpacing;
  myLonSpacing = l.myLonSpacing;
  myNumLats    = l.getNumLats();
  myNumLons    = l.getNumLons();
}

double
LatLonGridProjection::getValueAtLL(double latDegs, double lonDegs)
{
  // Try to do this quick and efficient, called a LOT
  const double x = (myLatNWDegs - latDegs) / myLatSpacing;

  if ((x < 0) || (x > myNumLats)) {
    return Constants::DataUnavailable;
  }
  const double y = (lonDegs - myLonNWDegs) / myLonSpacing;

  if ((y < 0) || (y > myNumLons)) {
    return Constants::DataUnavailable;
  }
  return (*my2DLayer)[size_t(x)][size_t(y)];
}

double
LatLonHeightGridProjection::getValueAtLL(double latDegs, double lonDegs)
{
  // Try to do this quick and efficient, called a LOT
  const double x = (myLatNWDegs - latDegs) / myLatSpacing;

  if ((x < 0) || (x > myNumLats)) {
    return Constants::DataUnavailable;
  }
  const double y = (lonDegs - myLonNWDegs) / myLonSpacing;

  if ((y < 0) || (y > myNumLons)) {
    return Constants::DataUnavailable;
  }

  return (*my3DLayer)[size_t(x)][size_t(y)][0];
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
LatLonHeightGridProjection::LLCoverageCenterDegree(const float degreeOut, const size_t numRows, const size_t numCols,
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
LatLonHeightGridProjection::LLCoverageFull(size_t& numRows, size_t& numCols,
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

bool
LatLonHeightGridProjection::LLCoverageTile(
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
