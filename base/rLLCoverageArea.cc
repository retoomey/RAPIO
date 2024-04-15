#include "rLLCoverageArea.h"
#include "rProject.h"
#include "rStrings.h"
#include "rLL.h"

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

  // Seems slightly more accurate to use ground hugging vs the 0 angle..though it's
  // so close might not matter
  //  Project::BeamPath_AzRangeToLatLon(cLat, cLon, 0, rangeKMs, 0, outHeightKMs, north, outDegs);
  Project::LLBearingDistance(cLat, cLon, 0, rangeKMs, north, outDegs);
  Project::LLBearingDistance(cLat, cLon, 90, rangeKMs, outDegs, east);
  Project::LLBearingDistance(cLat, cLon, 180, rangeKMs, south, outDegs);
  Project::LLBearingDistance(cLat, cLon, 270, rangeKMs, outDegs, west);

  #if 0
  LogInfo("For lat to lat " << nwLat << " to " << seLat << "\n");
  LogInfo(" yields 0 degrees " << north << "\n");
  LogInfo(" yields 90 degrees " << east << "\n");
  LogInfo(" yields 180 degrees " << south << "\n");
  LogInfo(" yields 270 degrees " << west << "\n");
  #endif
  // Inset each side independently
  if (nwLat > north) { // Inset the top
    const size_t deltaY = floor((out.nwLat - north) / latSpacing);
    out.startY = out.startY + deltaY; // Shift Y
    if (out.numY >= deltaY) {
      out.numY = out.numY - deltaY; // Reduce num y by the shift
    } else {
      out.numY = 0; // Too far away
    }
    out.nwLat = out.nwLat - (out.startY * out.latSpacing);
  }

  if (west > nwLon) { // Inset the left
    const size_t deltaX = floor((west - out.nwLon) / lonSpacing);
    out.startX = out.startX + deltaX; // Shift X
    if (out.numX >= deltaX) {
      out.numX = out.numX - deltaX; // Reduce num x by the shift
    } else {
      out.numX = 0; // Too far away
    }
    out.nwLon = out.nwLon + (out.startX * out.lonSpacing);
  }

  if (south > seLat) {                                             // Inset the bottom
    const size_t deltaY = floor((south - out.seLat) / latSpacing); //  - 1;
    if (out.numY >= deltaY) {
      out.numY = out.numY - deltaY; // Reduce num y by the shift up
    } else {
      out.numY = 0; // Too far away
    }
    out.seLat = out.nwLat - (out.numY * out.latSpacing);
  }

  if (seLon > east) {                                             // Inset the right
    const size_t deltaX = floor((out.seLon - east) / lonSpacing); //  - 1;
    if (out.numX >= deltaX) {
      out.numX = out.numX - deltaX; // Reduce num y by the shift
    } else {
      out.numX = 0; // Too far away
    }
    out.seLon = out.nwLon + (out.numX * out.lonSpacing);
  }

  out.sync();

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

void
LLCoverageArea::sync()
{
  // ---------------------------------------------------------------
  // Calculated distances
  // Use the cross mid point of the entire area
  // We're using ground at moment which differs from w2merger which
  // uses air height straight view distance.  Don't think it matters
  // but we can check it later.
  const AngleDegs midLat = (nwLat - seLat) / 2.0;
  const AngleDegs midLon = (seLon - nwLon) / 2.0;
  LL midTop(nwLat, midLon);
  LL midBot(seLat, midLon);
  LL leftMid(midLat, nwLon);
  LL rightMid(midLat, seLon);

  LengthKMs d1 = midTop.getSurfaceDistanceToKMs(midBot);
  LengthKMs d2 = leftMid.getSurfaceDistanceToKMs(rightMid);

  latKMPerPixel = fabs(d1 / numY);
  lonKMPerPixel = fabs(d2 / numX);
}

void
LLCoverageArea::set(AngleDegs north, AngleDegs west, AngleDegs south, AngleDegs east, AngleDegs southDelta,
  AngleDegs eastSpacing, size_t aNumX, size_t aNumY)
{
  // ---------------------------------------------------------------
  // Validate the lat, lon values
  if (north < south) {
    LogSevere("Nw corner is south of se corner, swapping...\n");
    std::swap(north, south);
  }
  if (east < west) {
    LogSevere("Nw corner is east of se corner, swapping...\n");
    std::swap(east, west);
  }

  // ---------------------------------------------------------------
  // Simple assigned values
  nwLat      = north;
  nwLon      = west;
  seLat      = south;
  seLon      = east;
  latSpacing = southDelta;
  lonSpacing = eastSpacing;
  startX     = 0;
  startY     = 0;
  numX       = aNumX;
  numY       = aNumY;

  sync();
}

namespace {
/** Add t or b or s parameters to the cooresponding grid language */
std::string
addTBS(const std::string& in, const std::string& prefix, std::string& heightList)
{
  std::vector<std::string> p;
  std::string out;

  Strings::split(in, &p);
  if (p.size() >= 2) { // at least two such as "55 -130"
    out = prefix + p[0] + "," + p[1] + ")";
    if (p.size() > 2) { // One more..so height value specified such as t = "55 -130 20"
      heightList = heightList.empty() ? "h(" + p[2] : heightList + "," + p[2];
    }
  }
  return out;
}
}

bool
LLCoverageArea::parseDegrees(const std::string& label, const std::string& p, AngleDegs& lat, AngleDegs& lon)
{
  bool failed = false;
  std::vector<std::string> pieces;

  Strings::split(p, ',', &pieces);
  if (pieces.size() != 2) {
    failed = true;
  } else {
    try{
      lat = std::stof(pieces[0]);
      lon = std::stof(pieces[1]);
    }catch (const std::exception& e) {
      LogSevere("Failed to convert to number " << pieces[0] << " and/or " << pieces[1] << "\n");
      failed = true;
    }
    // FIXME: check
  }
  if (failed) {
    LogSevere("Failed to parse '" << label << "' grid parameters: '" << p << "'\n");
  }
  return failed;
}

namespace {
/** Generate height list in meters from a low/high and incr/upto vector array set*/
bool
generateHeightList(double low, double high, std::vector<int>& incr, std::vector<int>& upto,
  std::vector<double>& heights)
{
  // Construct height list using a W2 incr/upto structure
  const size_t MAX_HEIGHTS_ALLOWED = 100;
  double atHeight = low;
  bool done       = false;
  size_t count    = 0; // integrity check

  while (!done) {
    if (++count >= MAX_HEIGHTS_ALLOWED) { //
      LogSevere("Generated more than " << count << " heights from grid spec, aborting.\n");
      return false;
    }
    heights.push_back(atHeight);
    for (size_t i = 0; i < incr.size(); i++) {
      if (atHeight < upto[i]) { // This one valid (could have dups though)
        atHeight += incr[i];
        if (atHeight >= high) { // end if broke high
          done = true;
        }
        break;
      }
    }
  }
  return true;
}
}

bool
LLCoverageArea::lookupIncrUpto(const std::string& key, std::vector<int>& incr, std::vector<int>& upto)
{
  // ---------------------------------------
  // In WDSS2 we have the w2merger.xml file with the various
  // defined heightlevels which we could read later if we need to.
  // For now, we'll hardcore them, but leave room for this later if needed
  bool found = true;

  if (key == "ARPS") {
    incr = { 250, 500, 1000, 1000 };
    upto = { 4000, 9000, 18000, 99999 };
  } else if (key == "WISH") {
    incr = { 250, 500, 1000, 2000 };
    upto = { 3000, 9000, 16000, 99999 };
  } else if (key == "NMQWD") {
    incr = { 250, 500, 1000 };
    upto = { 3000, 9000, 99999 };
  } else if (key == "Uniform1Km") {
    incr = { 1000 };
    upto = { 99999 };
  } else if (key == "XVision") {
    incr = { 500, 1000 };
    upto = { 5000, 99999 };
  } else {
    found = false;
  }
  if (found) {
    LogInfo("Found height description key '" << key << "'\n");
  }
  return found;
}

bool
LLCoverageArea::parseHeights(const std::string& label, const std::string& list, std::vector<double>& heightsMs)
{
  bool failed = false;

  // ---------------------------------------
  // Parse the height string into the three fields
  std::vector<std::string> pieces;

  Strings::split(list, ',', &pieces);
  if (pieces.size() != 3) {
    failed = true;
  } else {
    float low;
    float high;
    try{
      low  = std::stof(pieces[0]) * 1000.0; // Meters
      high = std::stof(pieces[1]) * 1000.0;
    }catch (const std::exception& e) {
      LogSevere("Failed to convert to number '" << pieces[0] << "' and/or '" << pieces[1] << "'\n");
      failed = true;
      return failed;
    }
    if (high < low) { // Make sure heights in order
      float temp = low;
      low  = high;
      high = temp;
    }

    // Create a string from the original arguments
    // This needs to be fairly unique.  The only way to do that 100% is to list all the
    // heights.
    const char D = '_';
    std::stringstream hs;
    hs << low << D << high << D;

    // ---------------------------------------
    // Get the incr/upto list from the height description
    std::vector<int> incr;
    std::vector<int> upto;
    std::string str = pieces[2];

    if (!lookupIncrUpto(str, incr, upto)) {
      // If lookup fails, try as a number
      try{
        float up    = std::stof(str);
        int incrInt = (int) (up * 1000.0);
        incr = { incrInt };
        upto = { 99999 };
        hs << incrInt << D << 99999;
      }catch (const std::exception& e) {
        LogSevere("Failed to find a key in heights or convert to number '" << str << "'\n");
        failed = true;
        return failed;
      }
    } else {
      // Assume the key is stable from config xml
      hs << str;
    }

    myHeightParse = hs.str();

    // ---------------------------------------
    // Finally generate full list of heights
    failed |= !generateHeightList(low, high, incr, upto, heightsMs);
    if (!failed) {
      std::stringstream ss;
      for (size_t i = 0; i < heightsMs.size(); i++) {
        ss << heightsMs[i] / 1000.0;
        heightsMs[i] = heightsMs[i] / 1000.0; // Make final in KMs
        if (i != heightsMs.size() - 1) {
          ss << " ";
        }
      }
      LogInfo("Generated heights (KMs): " << ss.str() << "\n");
    }
  }
  if (failed) {
    LogSevere("Failed to parse '" << label << "' height parameters: '" << list << "'\n");
  }
  return failed;
} // LLCoverageArea::parseHeights

bool
LLCoverageArea::parse(const std::string& grid, const std::string& t, const std::string& b, const std::string& s)
{
  // --------------------------------------------------
  // Convert t, b, s to the grid language if they exist (legacy support)
  std::stringstream ss;
  std::string finalGrid;

  if (t.empty() && b.empty() && s.empty()) {
    finalGrid = grid;
  } else {
    std::string heightList;
    ss << addTBS(t, "nw(", heightList) << " ";
    ss << addTBS(b, "se(", heightList) << " ";
    ss << addTBS(s, "s(", heightList) << " ";
    ss << heightList;
    if (!heightList.empty()) {
      ss << ")";
    }
    finalGrid = ss.str();
  }
  LogInfo("Grid declared: '" << finalGrid << "'\n");

  // --------------------------------------------------
  // Process the grid language, which consists of functions and params:
  // function(params)

  // Simple DFA process into function/params
  // FIXME:This ability might be useful for other params, Strings?
  std::map<std::string, std::string> functions;
  std::string function = "";
  std::string params   = "";
  size_t state         = 0;
  size_t at = 0;

  while (at < finalGrid.size()) {
    const char& c = finalGrid[at++];
    switch (state) {
        case 0: // outside function
          if (c == '(') { state = 1; } else { function += c; } // gather function
          break;
        case 1: // inside function
          if (c == ')') {
            state = 0;
            Strings::trim(function);
            Strings::trim(params);
            functions[function] = params;
            function = params = "";
          } else { params += c; } // gather params
          break;
        default:
          break;
    }
  }
  if (state == 1) {
    // we'll force close a last ')'
    functions[function] = params;
  }

  // --------------------------------------------------
  // Parse the grid function/param list
  AngleDegs nwLatDegs  = 55.0;
  AngleDegs nwLonDegs  = -130.0;
  AngleDegs seLatDegs  = 20.0;
  AngleDegs seLonDegs  = -60.0;
  AngleDegs latSpacing = 0.01;
  AngleDegs lonSpacing = 0.01;
  bool failed = false;
  bool haveNW = false;
  bool haveSE = false;
  bool haveS  = false;
  bool haveH  = false;

  std::vector<double> theHeightsKM;

  if (functions.size() < 1) {
    LogSevere("Unrecognized grid language: '" << finalGrid << "'\n");
    failed = true;
  }

  for (auto entry: functions) {
    std::string f = entry.first;
    std::string p = entry.second;
    if (f == "nw") {
      haveNW = !parseDegrees(f, p, nwLatDegs, nwLonDegs);
    } else if (f == "se") {
      haveSE = !parseDegrees(f, p, seLatDegs, seLonDegs);
    } else if (f == "s") {
      haveS = !parseDegrees(f, p, latSpacing, lonSpacing);
    } else if (f == "h") {
      haveH = !parseHeights(f, p, theHeightsKM);
    } else {
      LogSevere("Unrecognized grid language: '" << f << "' unrecognized.\n");
      failed = true;
    }
  }

  // Have to have these to be valid. Heights are optional
  if (!haveNW) {
    LogSevere("Missing nw() grid corner.\n");
    failed = true;
  }
  if (!haveSE) {
    LogSevere("Missing se() grid corner.\n");
    failed = true;
  }
  if (!haveS) {
    LogSevere("Missing s() grid spacing.\n");
    failed = true;
  }
  if (!haveH) { // 2D
    theHeightsKM = std::vector<double>{ 0.0 };
  }

  // Check nw and se corner ordering...we could fix/flip them probably or stop
  if (!failed) {
    if (nwLatDegs <= seLatDegs) {
      LogSevere("Grid nw latitude should be more than the se latitude\n");
      failed = true;
    }
    if (nwLonDegs >= seLonDegs) {
      LogSevere("Grid nw longitude should be less than the se longitude\n");
      failed = true;
    }
  }
  if (failed) {
    LogSevere("Couldn't parse grid options.\n");
    return false;
  }

  // Ok calculate the GRID dimensions, we do the x y first and get any spacing from that
  int x = (seLonDegs - nwLonDegs) / lonSpacing;

  if (x < 0) { x = -x; }
  int y = (nwLatDegs - seLatDegs) / latSpacing;

  if (y < 0) { y = -y; }

  set(nwLatDegs, nwLonDegs,
    seLatDegs, seLonDegs,
    latSpacing, lonSpacing,
    x,
    y);
  heightsKM = theHeightsKM;

  return true;
} // LLCoverageArea::parse

std::string
LLCoverageArea::getParseUniqueString() const
{
  const char D = '_';
  std::stringstream ss;

  ss << getNWLat() << D;
  ss << getNWLon() << D;
  ss << getSELat() << D;
  ss << getSELon() << D;
  ss << getLatSpacing() << D;
  ss << getLonSpacing() << D;
  ss << getStartX() << D;
  ss << getStartY() << D;
  ss << getNumX() << D;
  ss << getNumY() << D;
  ss << getNumZ() << D;
  ss << myHeightParse;
  return ss.str();
}
