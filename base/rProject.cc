#include "rProject.h"

#include "rError.h"
#include "rLatLonGrid.h"
#include "rXYZ.h"

#include <cmath>

using namespace rapio;

bool
Project::initialize()
{
  return false;
}

LengthKMs
Project::attenuationHeightKMs(
  LengthKMs stationHeightKMs,
  LengthKMs rangeKMs,
  AngleDegs elevDegs)
{
  // Radar formula approimation. p 232, Radar Equations for Modern Radar
  // this is good to 0.4% error at 1000 km.

  const double EarthRadius = Constants::EarthRadiusM / 1000.0;
  // const double EarthRadius=6371000.0;
  const double IR2 = (4. / 3.) * EarthRadius * 2;
  double elevRad   = elevDegs * DEG_TO_RAD;

  const double top    = rangeKMs * cos(elevRad);
  LengthKMs heightKMs = rangeKMs * sin(elevRad) + (top * top / IR2) + stationHeightKMs;

  return heightKMs;
}

void
Project::LatLonToAzRange(
  const AngleDegs &cLat,
  const AngleDegs &cLon,
  const AngleDegs &tLat,
  const AngleDegs &tLon,
  AngleDegs       &azDegs,
  float           &rangeMeters) // FIXME: do more work to make units consistent

{
  const auto meterDeg = Constants::EarthRadiusM * DEG_TO_RAD; // 2nr/360 amazingly

  // Below equator we flip lat I think
  auto Y = (tLat > 0) ? (tLat - cLat) * meterDeg : (cLat - tLat) * meterDeg;
  auto X = ((tLon - cLon) * meterDeg) * cos((cLat + tLat) / 2.0 * DEG_TO_RAD);

  // Final range/azimuth guess
  rangeMeters = sqrt(X * X + Y * Y);
  azDegs      = atan2(X, Y) * RAD_TO_DEG;
  if (azDegs < 0) {
    azDegs = 360.0 + azDegs;
  }
}

namespace {
double
asin_bugworkaround(double arg)
{
  double result = asin(arg);

  if (std::isnan(result) ) {
    // for some reason asin(1) returns
    if (arg < -0.99999999) {
      result = (-0.5 * M_PI);
    }
    if (arg > 0.999999999) {
      result = (0.5 * M_PI);
    }
  }
  return result;
}
}

void
Project::BeamPath_AzRangeToLatLon(
  const AngleDegs &stationLatDegs,
  const AngleDegs &stationLonDegs,

  const AngleDegs &azimuthDegs,
  const LengthKMs &rangeKMs,
  const AngleDegs &elevAngleDegs,

  LengthKMs       &heightKMs,
  AngleDegs       &latDegs,
  AngleDegs       &lonDegs)
{
  const auto elevAngleRad      = elevAngleDegs * DEG_TO_RAD;
  const auto stationLatRad     = (90. - stationLatDegs) * DEG_TO_RAD;
  const auto azimuthRad        = azimuthDegs * DEG_TO_RAD;
  const auto rangeM            = rangeKMs * 1000;
  const double EarthRadius     = Constants::EarthRadiusM;
  const double earthRefraction = (4. / 3.) * EarthRadius;

  const auto heightM = pow(pow(rangeM, 2.) + pow(earthRefraction, 2.)
      + 2. * rangeM * earthRefraction
      * sin(elevAngleRad), 0.5) - earthRefraction;

  const double great_circle_distance = earthRefraction
    * asin_bugworkaround( ( rangeM * cos(elevAngleRad) )
      / ( (earthRefraction) + heightM) );
  const auto gcdEarth = great_circle_distance / EarthRadius;
  const auto SIN      = sin(gcdEarth); // Make sure this sin only called once

  // Output
  latDegs = 90. - (acos( (cos(gcdEarth))
    * (cos(stationLatRad)) + ( SIN
    * (sin(stationLatRad)) * (cos(azimuthRad) )))) / DEG_TO_RAD;

  const double deltaLonDegs = ( asin_bugworkaround(SIN
    * (sin(azimuthRad)) / (sin(DEG_TO_RAD * (90. - latDegs)) ) ) ) / DEG_TO_RAD;

  heightKMs = heightM / 1000.0;
  lonDegs   = stationLonDegs + deltaLonDegs;
}

void
Project::BeamPath_LLHtoAzRangeElev(
  const AngleDegs & targetLatDegs,
  const AngleDegs & targetLonDegs,
  const LengthKMs & targetHeightKMs,

  const AngleDegs & stationLatDegs,
  const AngleDegs & stationLonDegs,
  const LengthKMs & stationHeightKMs,

  AngleDegs       & elevAngleDegs,
  AngleDegs       & azimuthDegs,
  LengthKMs       & rangeKMs)
{
  // Called SO much during Terrain algorithm, I think we could cache
  // the math and optimize.  The amount of sin/cos etc is crazy here.
  // FIXME: These functions would work better if passing in RADIANS
  const auto stationLatRad = (90. - stationLatDegs) * DEG_TO_RAD;
  const auto targetLatRad  = (90. - targetLatDegs) * DEG_TO_RAD;

  const AngleDegs deltaLonDegs = targetLonDegs - stationLonDegs;

  const auto deltaLonRad = deltaLonDegs * DEG_TO_RAD;

  const double EarthRadius = Constants::EarthRadiusM;
  // const double EarthRadius=6371000.0;
  const double IR = (4. / 3.) * EarthRadius;

  const double great_circle_distance =
    EarthRadius * acos(cos(stationLatRad)
      * cos(targetLatRad)
      + sin(stationLatRad)
      * sin(targetLatRad) * cos(deltaLonRad));

  #if 0
  // Doing azimuth in 3D isn't really necessary..azimuth aligns to lat/lon axis
  // and this saves a LOT of time vs 3D vector math
  const auto meterDeg = Constants::EarthRadiusM * DEG_TO_RAD; // 2nr/360 amazingly
  // Below equator we flip lat I think
  auto Y =
    (targetLatDegs > 0) ? (targetLatDegs - stationLatDegs) * meterDeg : (stationLatDegs - targetLatDegs) * meterDeg;
  auto X = (deltaLonDegs * meterDeg) * cos((stationLatDegs + targetLatDegs) / 2.0 * DEG_TO_RAD);
  azimuthDegs = atan2(X, Y) * RAD_TO_DEG;
  if (azimuthDegs < 0) {
    azimuthDegs = 360.0 + azimuthDegs;
  }
  #endif // if 0

  LLH target(targetLatDegs, targetLonDegs, targetHeightKMs);
  LLH station(stationLatDegs, stationLonDegs, stationHeightKMs);

  XYZ O   = XYZ(0, 0, 0); // O -- origin ( earth center )
  XYZ R   = XYZ(station); // R -- radar point
  XYZ C   = XYZ(target);
  IJK RC  = C - R;
  IJK OR  = R - O;
  IJK ORu = OR.unit();
  IJK Z(0.0, 0.0, 1.0);
  IJK oz        = ORu;
  IJK ox        = (Z % oz).unit();
  IJK oy        = (oz % ox ).unit();
  double azycos = RC.unit() * oy;
  double azxcos = RC.unit() * ox;
  double az     = atan( ( RC.norm() * azxcos ) / ( RC.norm() * azycos ) );

  if (azycos >= 0) {
    if (azxcos <= 0) { az = M_PI * 2 + az; }
  } else {
    az = M_PI + az;
  }

  // Convert the azimuth from radians to degrees
  azimuthDegs = az * RAD_TO_DEG; // humm combine with the M_PI math above?

  // reverse calculate elev_angle from height difference
  // FIXME: Can we do this in kilometers get correct values
  const double heightM = (targetHeightKMs - stationHeightKMs) * 1000.0;

  const double sinGcdIR = sin(great_circle_distance / IR);

  const double elevAngleRad = atan(
    (cos(great_circle_distance / IR) - (IR / (IR + heightM)))
    / sinGcdIR
  );

  rangeKMs      = (( sinGcdIR ) * (IR + heightM) / cos(elevAngleRad)) / 1000.0;
  elevAngleDegs = elevAngleRad * RAD_TO_DEG;
} // Project::BeamPath_LLHtoAzRangeElev

void
Project::Cached_BeamPath_LLHtoAzRangeElev(
  const AngleDegs & targetLatDegs,
  const AngleDegs & targetLonDegs,
  const LengthKMs & targetHeightKMs,

  const AngleDegs & stationLatDegs,
  const AngleDegs & stationLonDegs,
  const LengthKMs & stationHeightKMs,

  const double    sinGcdIR, // Cached sin/cos from regular version, this speeds stuff up
  const double    cosGcdIR,

  AngleDegs       & elevAngleDegs,
  AngleDegs       & azimuthDegs,
  LengthKMs       & rangeKMs)
{
  const double EarthRadius = Constants::EarthRadiusM;
  // const double EarthRadius=6371000.0;
  const double IR = (4. / 3.) * EarthRadius;

  LLH target(targetLatDegs, targetLonDegs, targetHeightKMs);
  LLH station(stationLatDegs, stationLonDegs, stationHeightKMs);

  XYZ O   = XYZ(0, 0, 0); // O -- origin ( earth center )
  XYZ R   = XYZ(station); // R -- radar point
  XYZ C   = XYZ(target);
  IJK RC  = C - R;
  IJK OR  = R - O;
  IJK ORu = OR.unit();
  IJK Z(0.0, 0.0, 1.0);
  IJK oz        = ORu;
  IJK ox        = (Z % oz).unit();
  IJK oy        = (oz % ox ).unit();
  double azycos = RC.unit() * oy;
  double azxcos = RC.unit() * ox;
  double az     = atan( ( RC.norm() * azxcos ) / ( RC.norm() * azycos ) );

  if (azycos >= 0) {
    if (azxcos <= 0) { az = M_PI * 2 + az; }
  } else {
    az = M_PI + az;
  }

  // Convert the azimuth from radians to degrees
  azimuthDegs = az * RAD_TO_DEG; // humm combine with the M_PI math above?

  // reverse calculate elev_angle from height difference
  const double heightM = (targetHeightKMs - stationHeightKMs) * 1000.0;

  const double elevAngleRad = atan(
    (cosGcdIR - (IR / (IR + heightM)))
    / sinGcdIR
  );

  rangeKMs      = (( sinGcdIR ) * (IR + heightM) / cos(elevAngleRad)) / 1000.0;
  elevAngleDegs = elevAngleRad * RAD_TO_DEG;
} // Project::Cached_BeamPath_LLHtoAzRangeElev

void
Project::BeamPath_LLHtoAttenuationRange(
  const AngleDegs & targetLatDegs,
  const AngleDegs & targetLonDegs,
  // const LengthKMs & targetHeightKMs, output now

  const AngleDegs & stationLatDegs,
  const AngleDegs & stationLonDegs,
  const LengthKMs & stationHeightKMs,

  const AngleDegs & elevAngleDegs, // need angle now
  //  AngleDegs       & azimuthDegs,  Doesn't matter which direction

  LengthKMs & targetHeightKMs,
  LengthKMs & rangeKMs)
{
  const auto stationLatRad = (90. - stationLatDegs) * DEG_TO_RAD;
  const auto targetLatRad  = (90. - targetLatDegs) * DEG_TO_RAD;

  const AngleDegs deltaLonDegs = targetLonDegs - stationLonDegs;

  const auto deltaLonRad = deltaLonDegs * DEG_TO_RAD;

  const double EarthRadius = Constants::EarthRadiusM;
  // const double EarthRadius=6371000.0;
  const double IR = (4. / 3.) * EarthRadius;

  const double great_circle_distance =
    EarthRadius * acos(cos(stationLatRad)
      * cos(targetLatRad)
      + sin(stationLatRad)
      * sin(targetLatRad) * cos(deltaLonRad));

  const double sinGcdIR = sin(great_circle_distance / IR);
  const double cosGcdIR = cos(great_circle_distance / IR);

  // Assuming Lak's formula correct and solving for a new elevation should give me the height
  // above and below.  I'll compare to radar book attentuation functions.  I prefer this
  // version of formula because it will 'match' the math of Lak/Krause.
  // Algebra:
  // const double elevAngleRad = atan( (cos(great_circle_distance / IR) - (IR / (IR + heightM))) / sinGcdIR);
  // double newElevRad = 20 * RAD_TO_DEG;  Say I have another elevation...
  // tan(newElevRad) =  (cos(great_circle_distance / IR) - (IR / (IR + heightM))) / sinGcdIR;
  // sinGcdIR*tan(newElevRad) = cos(great_circle_distance / IR) - (IR / (IR + heightM));
  // sinGcdIR*tan(newElevRad) - cos(great_circle_distance / IR) = - (IR / (IR + heightM));
  // Z  = - (C / (C + H));
  // H = C/-Z - C
  // -----------------------------------------------------------------------------------
  // Verified reverse formula from new elev to height here
  // heightM = (IR/(-sinGcdIR*tan(newElevRad) + cos(great_circle_distance/IR))) - IR;
  // targetHeightKMs = (heightM/1000.0) + stationHeightKMs;
  // -----------------------------------------------------------------------------------
  const double newElevRad = elevAngleDegs * DEG_TO_RAD;
  const double newHeightM = (IR / (-sinGcdIR * tan(newElevRad) + cosGcdIR)) - IR;

  targetHeightKMs = (newHeightM / 1000.0) + stationHeightKMs;

  rangeKMs = (( sinGcdIR ) * (IR + newHeightM) / cos(newElevRad)) / 1000.0;
} // Project::BeamPath_LLHtoAttenuationRange

void
Project::stationLatLonToTarget(
  const AngleDegs & targetLatDegs,
  const AngleDegs & targetLonDegs,

  const AngleDegs & stationLatDegs,
  const AngleDegs & stationLonDegs,

  double          & sinGcdIR,
  double          & cosGcdIR)
{
  // Common math for lat lon of a station to a target lat lon
  const auto stationLatRad = (90. - stationLatDegs) * DEG_TO_RAD;
  const auto targetLatRad  = (90. - targetLatDegs) * DEG_TO_RAD;

  const AngleDegs deltaLonDegs = targetLonDegs - stationLonDegs;

  const auto deltaLonRad = deltaLonDegs * DEG_TO_RAD;

  // FIXME: Move to a single constant
  const double EarthRadius = Constants::EarthRadiusM;
  // const double EarthRadius=6371000.0;
  const double IR = (4. / 3.) * EarthRadius;

  const double great_circle_distance =
    EarthRadius * acos(cos(stationLatRad)
      * cos(targetLatRad)
      + sin(stationLatRad)
      * sin(targetLatRad) * cos(deltaLonRad));

  sinGcdIR = sin(great_circle_distance / IR);
  cosGcdIR = cos(great_circle_distance / IR);
}

void
Project::Cached_BeamPath_LLHtoAttenuationRange(

  const LengthKMs & stationHeightKMs, // need to shift up/down based on station height

  const double    sinGcdIR, // Cached sin/cos from regular version, this speeds stuff up
  const double    cosGcdIR,

  const double    tanElev, // Cached tangent of the elevation angle
  const double    cosElev, // Cached cosine of the elevation angle

  LengthKMs       & targetHeightKMs,
  LengthKMs       & rangeKMs)
{
  const double EarthRadius = Constants::EarthRadiusM;
  // const double EarthRadius=6371000.0;
  const double IR = (4. / 3.) * EarthRadius;

  // -----------------------------------------------------------------------------------
  // Verified reverse formula from new elev to height here
  // heightM = (IR/(-sinGcdIR*tan(newElevRad) + cos(great_circle_distance/IR))) - IR;
  // targetHeightKMs = (heightM/1000.0) + stationHeightKMs;
  // -----------------------------------------------------------------------------------
  const double newHeightM = (IR / (-sinGcdIR * tanElev + cosGcdIR)) - IR;

  targetHeightKMs = (newHeightM / 1000.0) + stationHeightKMs;

  rangeKMs = (( sinGcdIR ) * (IR + newHeightM) / cosElev) / 1000.0;
}

void
Project::LLBearingDistance(
  const AngleDegs startLatDegs,
  const AngleDegs startLonDegs,
  const AngleDegs bearingDegs,
  const LengthKMs distance,
  AngleDegs       & outLat,
  AngleDegs       & outLon
)
{
  const double sinLat = sin(startLatDegs * DEG_TO_RAD);
  const double cosLat = cos(startLatDegs * DEG_TO_RAD);

  const double angDR      = distance / (Constants::EarthRadiusM / 1000.0);
  const double cosDR      = cos(angDR);
  const double sinDR      = sin(angDR);
  const double bearingRad = bearingDegs * DEG_TO_RAD;

  const double outLatRad = asin(sinLat * cosDR + cosLat * sinDR * cos(bearingRad));

  outLat = outLatRad * RAD_TO_DEG;
  outLon = startLonDegs + (atan2(sin(bearingRad) * sinDR * cosLat,
    cosDR - sinLat * sin(outLatRad))) * RAD_TO_DEG;
}

void
Project::createLatLonGrid(
  const float  centerLatDegs,
  const float  centerLonDegs,
  const float  degreeOut,
  const size_t numRows,
  const size_t numCols,
  float        & topDegs,
  float        & leftDegs,
  float        & deltaLatDegs,
  float        & deltaLonDegs)
{
  auto lon = centerLonDegs;

  leftDegs = lon - degreeOut;
  auto rightDegs = lon + degreeOut;
  auto width     = rightDegs - leftDegs;

  deltaLonDegs = width / numCols;

  // To keep aspect ratio per cell, use deltaLon to back calculate
  deltaLatDegs = -deltaLonDegs; // keep the same square per pixel
  auto lat = centerLatDegs;

  topDegs = lat - (deltaLatDegs * numRows / 2.0);
}

bool
Project::getXYCenter(double& centerXKm, double& centerYKm)
{
  return false;
}

bool
Project::xyToLatLon(double& x, double&y, double &lat, double&lon)
{
  return false;
}

bool
Project::LatLonToXY(double& lat, double&lon, double &x, double&y)
{
  return false;
}

int
Project::createLookup(
  // Output information
  int imageRows, // rows of output projection
  int imageCols, // cols of output projection
  int mCell,     // Kilometers per cell (sample rate)

  // Input lat lon grid information for projecting lat/lon to index
  // Should we pass the lat lon grid?
  float topleftlat,
  float topleftlon,
  float numlat,
  float numlon,
  float latres,
  float lonres)
{
  auto resultArray = Arrays::CreateFloat2D(imageRows, imageCols);
  auto& result     = resultArray->ref();

  int hits   = 0;
  int misses = 0;

  double centerXKm, centerYKm;

  getXYCenter(centerXKm, centerYKm);

  // negative delta, so ADD mcell
  int startX = centerXKm - ((imageCols / 2.0) * mCell);

  // positive delta, so subtract mcell
  int startY = centerYKm + ((imageRows / 2.0) * mCell);

  LogInfo("Projection: (" << centerXKm << ", " << centerYKm << "), ("
                          << startX << ", " << startY << ") ("
                          << mCell << "\n");
  // In kilometers from CENTER projected
  int currentY = startY;

  for (int i = 0; i < imageRows; ++i) {
    int currentX = startX;
    for (int j = 0; j < imageCols; ++j) {
      // Projection KM to Lat Lon
      double atX = currentX;
      double atY = currentY;
      double inx, iny;

      xyToLatLon(atX, atY, inx, iny);

      // Projection from Lat Lon to LatLonGrid indexes
      int x = int( (topleftlat - inx) / latres);
      int y = int( (iny - topleftlon) / lonres);

      // LogSevere("PRE: " << atX<<", " <<atY<<" == " << inx << ", " << iny << "\n");
      //   LogSevere(atX<<", " <<atY<<" == " << inx << ", " << iny << " ("<<x <<","<<y<<") " << numlat << "< " << numlon <<"\n");

      if (( x >= 0) && ( y >= 0) && ( x < numlat) && ( y < numlon) ) {
        // This has to be in array order
        result[i][j] = x * numlon + y;
        hits++;
      } else {
        misses++;
      }
      currentX += mCell;
    }
    currentY -= mCell;
  }
  LogInfo("Projection hit/miss % " << hits << "," << misses << " == "
                                   << ((float) (hits)) / ((float) (misses + hits)) << "\n");

  // Actually project.  Granted want to cache the above stuff
  //     const float* gridstart = &( grid[0][0]) ;

  /*
   * int count = 0;
   *   for (int i=0; i < imageRows; ++i){
   *     for (int j=0; j < imageCols; ++j){
   *       int lookup = indexIntoGrid[i][j];
   *       if ( lookup >= 0 ) {
   *          result[i][j] = *(gridstart + lookup);
   *       }else{
   *         if (lookup == -5){
   *           result[i][j] = 20;
   *     count++;
   *         }else if (lookup == -6){
   *           result[i][j] = 35;
   *         }
   *       }
   *     }
   *   }
   * std::cout << "COUNT WAS " << count << "\n";
   */

  // exit(1);
  return hits;
} // Project::createLookup

ProjLibProject::ProjLibProject(const std::string& src, const std::string& dst)
  : mySrc(src), myDst(dst), myP(nullptr)
{ }

bool
ProjLibProject::initialize()
{
  bool success = true;

  /* NOTE: the use of PROJ strings to describe CRS is strongly discouraged
   * in PROJ 6, as PROJ strings are a poor way of describing a CRS, and
   * more precise its geodetic datum.
   * Use of codes provided by authorities (such as "EPSG:4326", etc...)
   * or WKT strings will bring the full power of the "transformation
   * engine" used by PROJ to determine the best transformation(s) between
   * two CRS.
   */
  myP = proj_create_crs_to_crs(PJ_DEFAULT_CTX,
      mySrc.c_str(), // xy web merc
      myDst.c_str(), // mrms/wsr84/etc
      NULL);

  if (myP == 0) {
    LogSevere("Couldn't create Proj Library projection.\n");
    success = false;
  } else {
    /* For that particular use case, this is not needed.
     * proj_normalize_for_visualization() ensures that the coordinate
     * order expected and returned by proj_trans() will be longitude,
     * latitude for geographic CRS, and easting, northing for projected
     * CRS. If instead of using PROJ strings as above, "EPSG:XXXX" codes
     * had been used, this might had been necessary.
     */
    PJ * P_for_GIS = proj_normalize_for_visualization(PJ_DEFAULT_CTX, myP);
    if (0 == P_for_GIS) {
      proj_destroy(myP);
      success = false;
    } else {
      proj_destroy(myP);
      myP = P_for_GIS;
    }
  }

  return success;
} // ProjLibProject::initialize

bool
ProjLibProject::getXYCenter(double& centerXKm, double& centerYKm)
{
  //  We're storing the center shift in the proj4 x, y.  So our center is 0,0
  centerXKm = 0; // long
  centerYKm = 0; // lat
  return true;
}

bool
ProjLibProject::xyToLatLon(double& x, double&y, double &lat, double&lon)
{
  /* For reliable geographic <--> geocentric conversions, z shall not */
  /* be some random value. Also t shall be initialized to HUGE_VAL to */
  /* allow for proper selection of time-dependent operations if one of */
  /* the CRS is dynamic. */
  // Not sure why they have a PJ_XY since this leaves fields undefined, it's just a 4 array of double
  // Eh?  This could break if they change the union later...
  PJ_COORD c{ x, y, 0.0, HUGE_VAL };
  // c.xyzt.x = x;
  // c.xyzt.y = y;
  // c.xyzt.z = 0.0;
  // c.xyzt.t = HUGE_VAL; // time
  PJ_COORD c_out = proj_trans(myP, PJ_FWD, c);

  lon = c_out.lpzt.lam; // * 180.0 / M_PI;  It's giving me degrees not radians currently
  lat = c_out.lpzt.phi; // * 180.0 / M_PI;
  return (!proj_errno(myP));
}

bool
ProjLibProject::LatLonToXY(double& lat, double&lon, double &x, double&y)
{
  PJ_COORD c{ lon, lat, 0.0, HUGE_VAL }; // as PJ_LPZT
  // c.lpzt.lam = lon; //  * M_PI/180.0;
  // c.lpzt.phi = lat; // * M_PI/180.0;
  // c.lpzt.z = 0.0;
  // c.lpzt.t = HUGE_VAL;
  PJ_COORD c_out = proj_trans(myP, PJ_INV, c);

  x = c_out.xy.x;
  y = c_out.xy.y;
  return (!proj_errno(myP));
}

void
ProjLibProject::toLatLonGrid(std::shared_ptr<Array<float, 2> > ina,
  std::shared_ptr<LatLonGrid>                                  out)
{
  auto data2DFA = out->getFloat2D(Constants::PrimaryDataName); // size could be nice right?
  auto& data2DF = data2DFA->ref();

  // ----------------------------------------------------------
  // source projection system (non geometric)
  // We have to know the resolution of the input data to
  // map it correctly.  The mCell only affects the scaling not
  // the projection (other than shifting of course if wrong)
  // If your image looks too small/too big and shifted,
  // chances are the resolution per cell is not in units.
  // For example: hrrr data is 3km, so proj library should be
  // using 'units=km' and you should have an mCell of 3 (km)
  int mCell = 3;

  auto dims = ina->dims();
  auto& in  = ina->ref();
  // auto imageCols = in->shape()[0];
  // auto imageRows = in->shape()[1];
  auto imageCols = dims[0];
  auto imageRows = dims[1];

  size_t num_lats = out->getNumLats();
  size_t num_lons = out->getNumLons();
  // The XY 'center' of the data.  Projection usually zero
  // in the center?
  double centerXKm, centerYKm;

  getXYCenter(centerXKm, centerYKm);

  const double halfC = (imageCols / 2.0) * mCell;
  const double halfR = (imageRows / 2.0) * mCell;
  double startX      = centerXKm - halfC;
  double endX        = centerXKm + halfC;
  double startY      = centerYKm - halfR;
  double endY        = centerYKm + halfR;
  double rangeX      = endX - startX;
  double rangeY      = endY - startY;

  if (rangeX < 0) { rangeX = -rangeX; }
  if (rangeY < 0) { rangeY = -rangeY; }
  double scaleX = rangeX / (imageCols); // per cell size
  double scaleY = rangeY / (imageRows);

  LogSevere("StartX StartY = " << startX << ", " << startY << "\n");
  LogSevere("EndX EndY = " << endX << ", " << endY << "\n");
  LogSevere("rangeX rangeY = " << rangeX << ", " << rangeY << "\n");
  LogSevere("scaleX scaleY = " << scaleX << ", " << scaleY << "\n");
  LogSevere("num_lat num_lon = " << num_lats << ", " << num_lons << "\n");

  // ----------------------------------------------------------
  // destination projection information
  // FIXME: I think we should be dealing with DataGrid + projectInfo
  float lat_spacing = out->getLatSpacing();
  float lon_spacing = out->getLonSpacing();
  LLH location      = out->getLocation();
  double nwLat      = location.getLatitudeDeg();
  double nwLon      = location.getLongitudeDeg();

  double lowestX  = 100000000.0;
  double lowestY  = 100000000.0;
  double highestX = -100000000.0;
  double highestY = -100000000.0;

  // For each point in output lat lon
  double atLat = nwLat;

  for (size_t x = 0; x < num_lats; ++x) { // x,y in the space of LatLonGrid
    double atLon = nwLon;
    for (size_t y = 0; y < num_lons; ++y) {
      // Border/band debugging
      if (x <= 10) { data2DF[x][y] = 53; continue; }
      if ((x <= num_lats - 1) && (x >= num_lats - 10)) { data2DF[x][y] = 53; continue; }
      if (y <= 11) { data2DF[x][y] = 53; continue; }
      if ((y <= num_lons - 1) && (y >= num_lons - 10)) { data2DF[x][y] = 53; continue; }

      // double radLat = atLat * DEG_TO_RAD;
      // double radLon = atLon * DEG_TO_RAD;
      // int k         = pj_transform(pj_dst, pj_src, 1, 1, &radLon, &radLat, NULL);
      // LatLonToXY
      // PJ_COORD c;
      PJ_COORD c{ atLon, atLat, 0.0, HUGE_VAL }; // as PJ_LPZT
      // c.lpzt.lam = atLon; //  * M_PI/180.0;
      // c.lpzt.phi = atLat; //  * M_PI/180.0;
      // c.lpzt.z = 0.0;
      // c.lpzt.t = HUGE_VAL;
      PJ_COORD c_out = proj_trans(myP, PJ_INV, c);
      double radLat  = c_out.xy.x; // Argh..should be x,y values
      double radLon  = c_out.xy.y;

      if (proj_errno(myP)) {
        data2DF[x][y] = Constants::MissingData;
      } else {
        if (radLat < lowestX) { lowestX = radLat; }
        if (radLon < lowestY) { lowestY = radLon; }
        if (radLat > highestX) { highestX = radLat; }
        if (radLon > highestY) { highestY = radLon; }

        // Linear transform to array space
        int binX = (radLon - startX) / scaleX;
        int binY = (radLat - startY) / scaleY;

        if ( (binX >= 0) && (binX < int(imageCols)) && (binY >= 0) && (binY < int(imageRows))) {
          double value = in[binX][binY];
          //   if (value > 400){ value = Constants::MissingData; } // ULWRF
          data2DF[x][y] = value;
        } else {
          /* Quad test, should be centerd
           *            if (radLat < 0){
           *              if (radLon < 0){
           *                data2DF[x][y] = 20;
           *              }else{
           *                data2DF[x][y] = 40;
           *              }
           *            }else{
           *              if (radLon < 0){
           *                data2DF[x][y] = 30;
           *              }else{
           *                data2DF[x][y] = 50;
           *              }
           *            }
           *     if ((atLat > 33) && (atLat < 35)){ data2DF[x][y] = 20; }
           */
        }
      }
      atLon += lon_spacing;
    }
    atLat -= lat_spacing;
  }

  LogSevere("FINAL STATS: " << lowestX << " " << highestX << " " << lowestY << " " << highestY << "\n");
} // ProjLibProject::toLatLonGrid
