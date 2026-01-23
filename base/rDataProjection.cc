#include "rDataProjection.h"

#include "rPTreeData.h"
#include "rStrings.h"
#include "rProject.h"
#include "rError.h"

#include <iostream>

using namespace rapio;
using namespace std;

std::shared_ptr<ProjLibProject> DataProjection::theWebMercToLatLon = nullptr;

ostream&
rapio::operator << (ostream& os, const LLCoverage& p)
{
  return os << fmt::format("{}", p);
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
    fLogSevere("Unrecognized rows/cols settings, using defaults");
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
      fLogSevere("Require zoom, centerLatDegs and centerLonDegs to create image.");
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
      fLogSevere("Malformed BBOX? {}", bbox);
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
      // WMS capabilities xml we have <CRS>EPSG:3857</CRS>
      // fLogSevere("Unknown projection requested: {}", bboxsr);
      return theWebMercToLatLon;
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
    fLogSevere("Unrecognized settings, using defaults");
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
      fLogSevere("Missing degrees, using default of {}", degreeOut);
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
      fLogSevere("Unrecognized settings, using defaults");
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
    fLogSevere("Unknown projection mode {} specified.", mode);
  }

  if (!optionSuccess) {
    fLogSevere("Don't know how to create a coverage grid for this datatype using mode '{}'.", mode);
  }
  return optionSuccess;
} // DataProjection::getLLCoverage
