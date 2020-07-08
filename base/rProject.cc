#include "rProject.h"
#include "rError.h"
#include "rArray.h"

using namespace rapio;

bool
Project::initialize()
{
  return false;
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

int
Project::createLookup(
  // Output information
  int   imageRows, // rows of output projection
  int   imageCols, // cols of output projection
  int   mCell,     // Kilometers per cell (sample rate)

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
  // Proj4:
  bool success = true;

  if (!(pj_src = pj_init_plus(mySrc.c_str())) ) {
    LogSevere("Couldn't create Proj Library source coordinate system.\n");
    LogSevere("'" << mySrc << "'\n");
    success = false;
  }

  if (!(pj_dst = pj_init_plus(myDst.c_str())) ) {
    LogSevere("Couldn't create Proj Library destination coordinate system.\n");
    LogSevere("'" << myDst << "'\n");
    success = false;
  }

  /* NOTE: the use of PROJ strings to describe CRS is strongly discouraged
   * in PROJ 6, as PROJ strings are a poor way of describing a CRS, and
   * more precise its geodetic datum.
   * Use of codes provided by authorities (such as "EPSG:4326", etc...)
   * or WKT strings will bring the full power of the "transformation
   * engine" used by PROJ to determine the best transformation(s) between
   * two CRS.
   */
  myP = proj_create_crs_to_crs(PJ_DEFAULT_CTX,
      mySrc.c_str(),
      myDst.c_str(),
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
  int k = pj_transform(pj_src, pj_dst, 1, 1, &x, &y, NULL); // Change to Lat/Lon.

  if (k != 0) { return false; }
  lon = x * RAD_TO_DEG;
  lat = y * RAD_TO_DEG;
  return true;
}

void
ProjLibProject::toLatLonGrid(std::shared_ptr<Array<float, 2> > ina,
  std::shared_ptr<LatLonGrid>                           out)
{
  auto data2DFA = out->getFloat2D("ULWRF"); // size could be nice right?
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
      if ((x >= 0) && (x <= 10)) { data2DF[x][y] = 53; continue; }
      if ((x <= num_lats - 1) && (x >= num_lats - 10)) { data2DF[x][y] = 53; continue; }
      if ((y >= 0) && (y <= 11)) { data2DF[x][y] = 53; continue; }
      if ((y <= num_lons - 1) && (y >= num_lons - 10)) { data2DF[x][y] = 53; continue; }

      double radLat = atLat * DEG_TO_RAD;
      double radLon = atLon * DEG_TO_RAD;
      int k         = pj_transform(pj_dst, pj_src, 1, 1, &radLon, &radLat, NULL);

      if (k != 0) {
        data2DF[x][y] = Constants::MISSING_DATA;
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
          //   if (value > 400){ value = Constants::MISSING_DATA; } // ULWRF
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
