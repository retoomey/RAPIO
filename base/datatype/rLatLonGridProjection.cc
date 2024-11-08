#include "rLatLonGridProjection.h"
#include "rLatLonGrid.h"

using namespace rapio;
using namespace std;

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

double
LatLonGridProjection::getValueAtLL(double latDegs, double lonDegs)
{
  // FIXME: This code now matches the array algorithm,
  // nearest. We could possibly merge that API
  // Try to do this quick and efficient, called a LOT
  const double xd = (myLatNWDegs - latDegs) / myLatSpacing;
  const int x     = std::round(xd);

  if ((x < 0) || (x >= myNumLats)) {
    return Constants::DataUnavailable;
  }
  const double yd = (lonDegs - myLonNWDegs) / myLonSpacing;
  const int y     = std::round(yd);

  if ((y < 0) || (y >= myNumLons)) {
    return Constants::DataUnavailable;
  }
  return (*my2DLayer)[size_t(x)][size_t(y)];
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
