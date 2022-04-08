#include "rLLCoverageArea.h"
#include "rProject.h"

#include <iostream>

using namespace rapio;
using namespace std;

ostream&
rapio::operator << (ostream& output, const LLCoverageArea& g)
{
  output << "(" << g.nwLat << "," << g.nwLon << "," << g.seLat << "," << g.seLon << "[" << g.latSpacing << "," <<
    g.lonSpacing << "]," << g.startX << "," << g.startY << "," << g.numX << "," << g.numY << ")";

  return (output);
}

LLCoverageArea
LLCoverageArea::insetRadarRange(
  AngleDegs cLat, AngleDegs cLon, LengthKMs rangeKMs
) const
{
  // Start with original grid before reducing
  LLCoverageArea out = *this;

  // Project range in each of four directions around the radar center
  LengthKMs outHeightKMs;
  AngleDegs outDegs, north, south, east, west;

  Project::BeamPath_AzRangeToLatLon(cLat, cLon, 0, rangeKMs, 0, outHeightKMs, north, outDegs);
  Project::BeamPath_AzRangeToLatLon(cLat, cLon, 90, rangeKMs, 0, outHeightKMs, outDegs, east);
  Project::BeamPath_AzRangeToLatLon(cLat, cLon, 180, rangeKMs, 0, outHeightKMs, south, outDegs);
  Project::BeamPath_AzRangeToLatLon(cLat, cLon, 270, rangeKMs, 0, outHeightKMs, outDegs, west);

  #if 0
  LogInfo("For lat to lat " << nwLat << " to " << seLat << "\n");
  LogInfo(" yields 0 degrees " << north << "\n");
  LogInfo(" yields 90 degrees " << east << "\n");
  LogInfo(" yields 180 degrees " << south << "\n");
  LogInfo(" yields 270 degrees " << west << "\n");
  #endif

  // Inset each side independently
  if (nwLat > north) { // Inset the top
    size_t deltaY = floor((out.nwLat - north) / latSpacing);
    out.startY = out.startY + deltaY; // Shift Y
    out.numY   = out.numY - deltaY;   // Reduce num y by the shift
    out.nwLat  = out.nwLat - (out.startY * out.latSpacing);
  }

  if (west > nwLon) { // Inset the left
    size_t deltaX = floor((west - out.nwLon) / lonSpacing);
    out.startX = out.startX + deltaX; // Shift X
    out.numX   = out.numX - deltaX;   // Reduce num x by the shift
    out.nwLon  = out.nwLon + (out.startX * out.lonSpacing);
  }

  if (south > seLat) {                                       // Inset the bottom
    size_t deltaY = floor((south - out.seLat) / latSpacing); //  - 1;
    out.numY  = out.numY - deltaY;                           // Reduce num y by the shift up
    out.seLat = out.nwLat - (out.numY * out.latSpacing);
  }

  if (seLon > east) {                                       // Inset the right
    size_t deltaX = floor((out.seLon - east) / lonSpacing); //  - 1;
    out.numX  = out.numX - deltaX;                          // Reduce num y by the shift
    out.seLon = out.nwLon + (out.numX * out.lonSpacing);
  }

  #if 0
  LogInfo(
    "OK: " << nwLon << " , " << out.nwLon << " , " << out.seLon << ", " << seLon << " Spacing: " << lonSpacing <<
      "\n");
  LogInfo("Grid  X range is:   0 to " << numX << "\n");
  LogInfo("Final X range is: " << out.startX << " to " << out.numX << "\n");

  LogInfo(
    "OK: " << nwLat << " , " << out.nwLat << " , " << out.seLat << ", " << seLat << " Spacing: " << latSpacing <<
      "\n");
  LogInfo("Grid  Y range is:   0 to " << numY << "\n");
  LogInfo("Final Y range is: " << out.startY << " to " << out.numY << "\n");

  LogInfo(
    "FULL COVERAGE (Not yet clipped to radar!) " << nwLat << ", " << nwLon << " to " << seLat << ", " << seLon <<
      "\n");
  LogInfo("Spacing " << latSpacing << ", " << lonSpacing << "\n");
  #endif // if 0

  return out;
} // LLCoverageArea::insetRadarRange
