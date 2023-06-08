#include "rIOGrib.h"

#include "rFactory.h"
#include "rIOURL.h"
#include "rStrings.h"
#include "rConfig.h"

#include "rGribDataTypeImp.h"

using namespace rapio;

// Library dynamic link to create this factory
extern "C"
{
void *
createRAPIOIO(void)
{
  auto * z = new IOGrib();

  z->initialize();
  return reinterpret_cast<void *>(z);
}
};

extern "C" {
#include <grib2.h>
}


/** A simple lookup from number to string.
 * Using a vector which actually tends to be quicker than map in practice.
 * This is a two column table sorted as vectors.
 * FIXME: this should go somewhere or become generic
 *
 * @author Robert Toomey */
namespace rapio {
class NumberToStringLookup : public IO
{
public:
  /** Column 1 **/
  std::vector<int> myValues;

  /** Column 2 **/
  std::vector<std::string> myStrings;

  /** Simple add */
  void
  add(int v, const std::string& s)
  {
    for (auto& i:myValues) {
      if (i == v) {
        LogSevere("-------------------->DUPLICATE ADD OF VALUE " << i << "\n");
        return; // Can't duplicate
      }
    }
    myValues.push_back(v);
    myStrings.push_back(s);
  }

  /** Simple get */
  std::string
  get(int v)
  {
    for (size_t i = 0; i < myValues.size(); ++i) {
      if (myValues[i] == v) {
        return myStrings[i];
      }
    }
    return "";
  }

  /** Print it out to see */
  void
  dump()
  {
    size_t s = myValues.size();

    for (size_t i = 0; i < s; ++i) {
      std::cout << myValues[i] << " == " << myStrings[i] << "\n";
    }
  }
};
}

// Tables for moment
NumberToStringLookup ncep;
NumberToStringLookup levels;
NumberToStringLookup table4dot4;

/** Should be from disk probably */
void
readNCEPLocalLevels()
{
  ncep.add(200, "entire atmosphere (considered as a single layer)");
  ncep.add(201, "entire ocean (considered as a single layer)");
  ncep.add(204, "highest tropospheric freezing level");
  ncep.add(206, "grid scale cloud bottom level");
  ncep.add(207, "grid scale cloud top level");
  ncep.add(209, "boundary layer cloud bottom level");
  ncep.add(210, "boundary layer cloud top level");
  ncep.add(211, "boundary layer cloud layer");
  ncep.add(212, "low cloud bottom level");
  ncep.add(213, "low cloud top level");
  ncep.add(214, "low cloud layer");
  ncep.add(215, "cloud ceiling");
  ncep.add(220, "planetary boundary layer");
  ncep.add(221, "layer between two hybrid levels");
  ncep.add(222, "middle cloud bottom level");
  ncep.add(223, "middle cloud top level");
  ncep.add(224, "middle cloud layer");
  ncep.add(232, "high cloud bottom level");
  ncep.add(233, "high cloud top level");
  ncep.add(234, "high cloud layer");
  ncep.add(235, "ocean isotherm level (1/10 deg C)");
  ncep.add(236, "layer between two depths below ocean surface");
  ncep.add(237, "bottom of ocean mixed layer");
  ncep.add(238, "bottom of ocean isothermal layer");
  ncep.add(239, "layer ocean surface and 26C ocean isothermal level");
  ncep.add(240, "ocean mixed layer");
  ncep.add(241, "ordered sequence of data");
  ncep.add(242, "convective cloud bottom level");
  ncep.add(243, "convective cloud top level");
  ncep.add(244, "convective cloud layer");
  ncep.add(245, "lowest level of the wet bulb zero");
  ncep.add(246, "maximum equivalent potential temperature level");
  ncep.add(247, "equilibrium level");
  ncep.add(248, "shallow convective cloud bottom level");
  ncep.add(249, "shallow convective cloud top level");
  ncep.add(251, "deep convective cloud bottom level");
  ncep.add(252, "deep convective cloud top level");
  ncep.add(253, "lowest bottom level of supercooled liquid water layer");
  ncep.add(254, "highest top level of supercooled liquid water layer");
} // readNCEPLocalLevels

void
readLevels()
{
  // Default is "reserved"
  levels.add(1, "surface");
  levels.add(2, "cloud base");
  levels.add(3, "cloud top");
  levels.add(4, "0C isotherm");
  levels.add(5, "level of adiabatic condensation from sfc");
  levels.add(6, "max wind");
  levels.add(7, "tropopause");
  levels.add(8, "top of atmosphere");
  levels.add(9, "sea bottom");
  levels.add(10, "entire atmosphere");
  levels.add(11, "cumulonimbus base");
  levels.add(12, "cumulonimbus top");

  levels.add(20, "%g K level");

  levels.add(100, "%g mb");
  levels.add(101, "mean sea level");
  levels.add(102, "%g m above mean sea level");
  levels.add(103, "%g m above ground");
  levels.add(104, "%g sigma level");
  levels.add(105, "%g hybrid level");
  levels.add(106, "%g m underground");
  levels.add(107, "%g K isentropic level");
  levels.add(108, "%g mb above ground");
  levels.add(109, "PV=%g (Km^2/kg/s) surface");

  levels.add(111, "%g Eta level");

  levels.add(117, "mixed layer depth");
  levels.add(118, "hybrid height level");
  levels.add(119, "hybrid pressure level");

  levels.add(160, "%g m below sea level");
} // readLevels

void
readTable4dot4()
{
  table4dot4.add(0, "minute");
  table4dot4.add(1, "hour");
  table4dot4.add(2, "day");
  table4dot4.add(3, "month");
  table4dot4.add(4, "year");
  table4dot4.add(5, "decade");
  table4dot4.add(6, "normal (30 years)");
  table4dot4.add(7, "century");
  table4dot4.add(10, "3 hours");
  table4dot4.add(11, "6 hours");
  table4dot4.add(12, "12 hours");
  table4dot4.add(13, "second");
  table4dot4.add(255, "missing");
}

namespace rapio {
class GribLookup : public IO
{
public:
  GribLookup(int d, int m, int c, int l, int pc, int pn,
    const std::string& k, const std::string& des,
    const std::string& u) : disc(d), mtab(m), cntr(c), ltab(l),
    pcat(pc), pnum(pn), key(k), description(des), units(u)
  { }

  int disc;
  int mtab;
  int cntr;
  int ltab;
  int pcat;
  int pnum;
  // Still debating if a map would be faster here
  std::string key;
  std::string description;
  std::string units;
};
}

static std::vector<GribLookup> myLookup;

GribLookup *
huntDatabase(int d, int m, int c, int l, int pc, int pn,
  const std::string& k, const std::string& des)
{
  // GribLookup* l = huntDatabase(disc, 0, 0, 0, pcat, pnum, "", "");
  for (auto &i:myLookup) {
    bool match = true;
    match &= ((d == -1) || (d == i.disc));
    //     match &= ((m == -1) || (m == i.mtab));
    //     match &= ((c == -1) || (c == i.cntr));
    match &= ((pc == -1) || (pc == i.pcat));
    match &= ((pn == -1) || (pn == i.pnum));

    //     match &= ((k.empty()) || (k == i.key));
    //     match &= ((des.empty()) || (des == i.description));

    if (match) {
      // LogSevere("MATCHED " << d <<", " << pc << ", " << "m in " << m << " == " << i.mtab << "\n");
      return &i;
    }
  }
  return nullptr;
}

std::string
IOGrib::getGrib2Error(g2int ierr)
{
  std::string err = "No Error";

  switch (ierr) {
      case 0:
        break;
      case 1: err = "Beginning characters GRIB not found";
        break;
      case 2: err = "Grib message is not Edition 2";
        break;
      case 3: err = "Could not find Section 1, where expected";
        break;
      case 4: err = "End string '7777' found, but not where expected";
        break;
      case 5: err = "End string '7777' not found at end of message";
        break;
      case 6: err = "Invalid section number found";
        break;
      default:
        err = "Unknown grib2 error: " + std::to_string(ierr);
        break;
  }
  return err;
}

void
GribAction::setG2Info(size_t messageNum, size_t at, g2int listsec0[3], g2int listsec1[13], g2int numlocal)
{
  myMessageNum = messageNum;
  myBufferAt   = at;
  for (size_t i = 0; i < 3; ++i) {
    mySection0[i] = listsec0[i];
  }
  for (size_t i = 0; i < 13; ++i) {
    mySection1[i] = listsec1[i];
  }
  myNumLocal = numlocal;
}

std::string
GribAction::getDateString()
{
  std::stringstream ss;

  //                listsec1[5]=Reference Time - Year (4 digits)
  //                listsec1[6]=Reference Time - Month
  //                listsec1[7]=Reference Time - Day
  //                listsec1[8]=Reference Time - Hour
  //                listsec1[9]=Reference Time - Minute
  //                listsec1[10]=Reference Time - Second
  ss << mySection1[5]
     << std::setfill('0') << std::setw(2) << mySection1[6]
     << std::setfill('0') << std::setw(2) << mySection1[7]
     << std::setfill('0') << std::setw(2) << mySection1[8];
  // minute/second ignored here
  return ss.str();
}

bool
GribCatalog::action(gribfield * gfld, size_t fieldNumber)
{
  std::string productName;
  std::string levelName;

  // A Wgrib2 style dump.
  IOGrib::toCatalog(gfld, productName, levelName);
  std::cout << myMessageNum << ":" << myBufferAt << ":"
            << "d=" << getDateString() << ":" << productName << ":" << levelName << "\n";

  return true; // Keep going, do all messages
}

bool
IOGrib::match(gribfield * gfld,
  const std::string       & productName,
  const std::string       & levelName)
{
  // Product name is simplest
  std::string pn = getProductName(gfld);

  if (pn != productName) {
    return false;
  }

  // Now go for level
  std::string level = getLevelName(gfld);

  if (level != levelName) {
    return false;
  }

  return true;
}

std::string
IOGrib::getProductName(gribfield * gfld)
{
  // Key1: Grib2 discipline is also in the header, but we can get it here
  g2int disc = gfld->discipline;
  // Grib2 Grid Definition Template (GDT) number.  This tells how the grid is projected...
  // const g2int gdtn = gfld->igdtnum;

  // Grib2 PRODUCT definition template number.  Tells you which Template 4.x defines
  // the stuff in the ipdtmpl.  Technically we would have to check all of these to
  // make sure the Product Category is the first number...
  // g2int pdtn = gfld->ipdtnum;

  // Identification of originating center Table C-1
  g2int cntr = gfld->idsect[0];

  // g2int subcntr = gfld->idsect[1];

  // GRIB master tables version number Code Table 1.0 (0 experimental, 1 initial version )
  // g2int master = gfld->idsect[2];

  // GRIB Local Tables Version Number (see Code Table 1.1)
  g2int localTable = gfld->idsect[3];
  // FIXME: Field has its own time

  // Key 2: Category  Section 4, Octet 10 (For all Templates 4.X)
  g2int pcat = gfld->ipdtmpl[0];

  // Key 3: Parameter Number.  Section 4, Octet 11
  g2int pnum = gfld->ipdtmpl[1];

  // We'll make a database.  Yay.
  // Not sure why master isn't matching..do they not match it?
  // LogSevere("MASTER TABLE " << master << "\n");
  GribLookup * l = huntDatabase(disc, -1, cntr, localTable, pcat, pnum, "", "");

  std::string pn;

  if (l != nullptr) {
    pn = l->key;
  } else {
    std::stringstream spn;
    spn << "var discipline=" << disc << " center=" << cntr << " local_table=" << localTable
        << " parmcat=" << pcat << " parm=" << pnum;
    pn = spn.str();
  }
  return pn;
} // IOGrib::getProductName

std::string
IOGrib::getLevelName(gribfield * gfld)
{
  // Grib2 PRODUCT definition template number.  Tells you which Template 4.x defines
  // the stuff in the ipdtmpl.  Technically we would have to check all of these to
  // make sure the Product Category is the first number...
  g2int pdtn = gfld->ipdtnum;

  // Identification of originating center Table C-1
  g2int cntr = gfld->idsect[0];

  // Get the level number? Section 4, Octet 23
  g2int level  = -1;
  g2int level2 = -1;
  g2int value1 = -1;

  std::string value1str;
  // g2int scale1 = -1;
  g2int value2 = -1;
  std::string value2str;

  // g2int scale2 = -1;

  // Humm we aren't using time units?
  // std::string timeunits = "";
  // g2int timeForecast = -1;

  // FIXME: We could make a class tree of the templates to enforce bounds
  // checking
  if (pdtn <= 15) {
    // First fixed surface
    level = gfld->ipdtmpl[9]; // Octet 23    Type of first fixed surface
    // scale1 = gfld->ipdtmpl[10]; // Octet 24    Scale factor of first fixed surface
    value1 = gfld->ipdtmpl[11]; // Octer 25-28 Scaled value of the first fixed surface

    // Second fixed surface
    level2 = gfld->ipdtmpl[12]; // Octet 29    Type of second fixed surface
    // scale2 = gfld->ipdtmpl[13]; // Octet 30    Scale factor of second fixed surface
    value2 = gfld->ipdtmpl[14]; // Octet 31-34 Scaled value of the second fixed surface

    // FIXME: use time units?
    // g2int timeUnit = gfld->ipdtmpl[7]; // Octet 18 Indicator of unit of time range table 4.4
    // std::string l = table4dot4.get(timeUnit);
    // if (!l.empty()) {
    //  timeunits = l;
    // }
    // timeForecast = gfld->ipdtmpl[8];  // Octet 19-22 Forecast time
  }

  // Fixed surface stuff only for template 0-15
  // No idea what they do outside this range.

  // We'll call this the level/layer string.
  std::string levelStr = "no_level";

  if (level < 192) {
    std::string l = levels.get(level);
    if (l.empty()) {
      l = "reserved";
    }
    levelStr = l;
  }

  // Wgrib does lots of extras for the layer strings, but they are just the
  // single layer with %g-%g and layer instead of level
  if (level2 == 255) { // MISSING
    // We have a LEVEL
    if (( cntr == 7) && ( level >= 192) && ( level <= 254) ) {
      if (level == 235) {
        //  levelStr = "%gC ocean isotherm";  Just use the table..kill me
        value1 /= 10;
      }
      // if (level1 == 241){
      //  levelStr = "%g in sequence";
      // }

      std::string l = ncep.get(level);
      if (l.empty()) {
        // FIXME: I don't check undef_val
        levelStr = "NCEP level type " + std::to_string(level) + " " + std::to_string(value1);
      } else {
        levelStr = l;
      }
    }
  } else {
    // We have a LAYER not a level, update the string
    Strings::replace(levelStr, "%g", "%g-%g");
    Strings::replace(levelStr, "level", "layer");

    // These are 'specials'
    // if NCEP  and 235, 235  ocean isotherm layer
    // if NCEP  and 236, 236  ocean layer
    if (cntr == 7) {
      if (( level == 235) && ( level2 == 235) ) {
        levelStr = "%g-%gC ocean isotherm layer";
        value1  /= 10;
        value2  /= 10;
      } else if (( level == 236) && ( level2 == 236) ) {
        levelStr = "%g-%g m ocean layer";
        value1  *= 10;
        value2  *= 10;
      }
    }
    if (( level == 1) && ( level2 == 8) ) { levelStr = "atmos col"; }
    if (( level == 9) && ( level2 == 1) ) { levelStr = "ocean column"; }
    // if (level == 255 && level2 == 255){ levelStr = "no_level"; } our default
  }

  // Scaling of the level values from Pa to mb.
  // FIXME: Another table could give a unit conversion factor, avoid all this.
  if (level == 100) {
    value1 /= 100;
  }
  if (level2 == 100) {
    value2 /= 100;
  }
  if (level == 108) {
    value1 /= 100;
  }
  if (level2 == 108) {
    value2 /= 100;
  }

  // Diff from wgrib2...DZDT in hrrr is decimal...humm
  if (level == 104) {
    // FIXME: definitely need to clean up/generalize all this
    // artifacts from using c strings and sprintf.
    std::ostringstream out;
    out.precision(2);
    out << std::fixed << ((double) value1) / 100.0;
    value1str = out.str();
    // value1 /= 100; Error goes to zero.
  }
  if (level2 == 104) {
    std::ostringstream out;
    out.precision(2);
    out << std::fixed << ((double) value2) / 100.0;
    value2str = out.str();
    // value2 /= 100; Error goes to zero
  }

  if (value1str.empty()) {
    value1str = std::to_string(value1);
  }
  if (value2str.empty()) {
    value2str = std::to_string(value2);
  }
  if (level2 == 255) { // MISSING
    Strings::replace(levelStr, "%g", value1str);
  } else {
    Strings::replace(levelStr, "%g-%g", value1str + "-" + value2str);
  }

  return levelStr;
} // IOGrib::getLevelName

void
IOGrib::toCatalog(gribfield * gfld,
  std::string                 & productName,
  std::string                 & levelName)
{
  productName = getProductName(gfld);
  levelName   = getLevelName(gfld);
}

std::shared_ptr<Array<float, 2> >
IOGrib::get2DData(std::vector<char>& b, size_t at, size_t fieldNumber, size_t& x, size_t& y)
{
  // size_t aSize       = b.size();
  unsigned char * bu = (unsigned char *) (&b[0]);
  // Reread found data, but slower now, unpack it
  gribfield * gfld = 0; // output
  int ierr         = g2_getfld(&bu[at], fieldNumber, 1, 1, &gfld);

  if (ierr == 0) {
    LogInfo("Grib2 field unpack successful\n");
  }
  const g2int * gds = gfld->igdtmpl;
  int tX      = gds[7]; // 31-34 Nx -- Number of points along the x-axis
  int tY      = gds[8]; // 35-38 Ny -- Number of points along te y-axis
  size_t numX = (tX < 0) ? 0 : (size_t) (tX);
  size_t numY = (tY < 0) ? 0 : (size_t) (tY);

  LogInfo("Grib2 2D field size " << numX << " * " << numY << "\n");
  x = numX;
  y = numY;
  g2float * g2grid = gfld->fld;

  auto newOne = Arrays::CreateFloat2D(numX, numY);
  auto& data  = newOne->ref();

  for (size_t xf = 0; xf < numX; ++xf) {
    for (size_t yf = 0; yf < numY; ++yf) {
      data[xf][yf] = (float) (g2grid[yf * numX + xf]);
    }
  }

  g2_free(gfld);

  return newOne;
} // IOGrib::get2DData

std::shared_ptr<Array<float, 3> >
IOGrib::get3DData(std::vector<char>& b, const std::string& key, const std::vector<std::string>& levelsStringVec,
  size_t& x, size_t&y, size_t& z, float missing)
{
  size_t zVecSize = levelsStringVec.size();
  size_t nz       = (zVecSize == 0) ? 40 : (size_t) zVecSize;

  size_t nx   = (x == 0) ? 1 : x;
  size_t ny   = (y == 0) ? 1 : y;
  auto newOne = Arrays::CreateFloat3D(nx, ny, nz);
  auto &data  = newOne->ref();

  // std::shared_ptr<RAPIO_3DF> newOne = std::make_shared<RAPIO_3DF>(RAPIO_DIM3(nx, ny, nz));
  // auto &data = *newOne;

  unsigned char * bu = (unsigned char *) (&b[0]);

  for (size_t k = 0; k < nz; ++k) {
    GribMatcher test(key, levelsStringVec[k]);
    IOGrib::scanGribData(b, &test);

    size_t at, fieldNumber;
    if (test.getMatch(at, fieldNumber)) {
      gribfield * gfld = 0; // output

      int ierr = g2_getfld(&bu[at], fieldNumber, 1, 1, &gfld);

      if (ierr == 0) {
        LogInfo("Grib2 field unpack successful\n");
      }
      const g2int * gds = gfld->igdtmpl;
      int tX      = gds[7]; // 31-34 Nx -- Number of points along the x-axis
      int tY      = gds[8]; // 35-38 Ny -- Number of points along te y-axis
      size_t numX = (tX < 0) ? 0 : (size_t) (tX);
      size_t numY = (tY < 0) ? 0 : (size_t) (tY);

      x = numX;
      y = numY;

      LogInfo("Grib2 3D field size " << numX << " * " << numY << " * " << nz << "\n");
      if ((nx != x) || (ny != y)) {
        LogInfo("Warning! sizes do not match" << nx << " != " << x << " and/or " << ny << " != " << y << "\n");
      }

      g2float * g2grid = gfld->fld;

      for (size_t xf = 0; xf < numX; ++xf) {
        for (size_t yf = 0; yf < numY; ++yf) {
          data[xf][yf][k] = (float) (g2grid[yf * numX + xf]);
        }
      }

      g2_free(gfld);
    } // test.getMatch
    else {
      LogSevere("No data for " << key << " at " << levelsStringVec[k] << "\n");

      // FIXME: The desired situation has to be confirmed - exit, which will help ensure the mistake is corrected, or a log
      // exit(1);

      for (size_t xf = 0; xf < nx; ++xf) {
        for (size_t yf = 0; yf < ny; ++yf) {
          data[xf][yf][k] = missing;
        }
      }
    }
  }
  // g2_free(gfld);
  z = nz;

  return newOne; // nullptr;
} // IOGrib::get3DData

/*
 *  // A Wgrib2 style dump.
 *  std::cout << myMessageNum << ":"<<myBufferAt << ":" <<
 *    "d=" <<  getDateString() << ":" << pn << ":" << levelStr <<
 * //         "[" << level << "," << value1  << ", " << scale1 << "]" <<
 * //         "[" << level2 << "," << value2  << ", " << scale2 << "]" <<
 * //     "("<<disc<<")("<<pdtn <<")("<<pcat<<")("<<pnum << ")" <<
 * // FIXME: Not same as wgrib2.  Not sure we need '3-4 hour max fcst' for instance...
 * ":" << timeForecast << " " << timeunits <<  // "[" << level << ", " << level2 << "]" <<
 *   "\n";
 */

/*
 *  LogSevere("Trying to read the data from it..\n");
 *      // Reread found data, but slower now, unpack it
 *  gribfield * gfld2 = 0; // output
 *  ierr         = g2_getfld(&bu[k], f, 1, 1, &gfld2);
 *  if (ierr == 0){
 *    LogSevere("UNPACK PASS WORKED?\n");
 *  }
 *  const g2int* gds = gfld->igdtmpl;
 *  int tX = gds[7]; // 31-34 Nx -- Number of points along the x-axis
 *  int tY = gds[8]; // 35-38 Ny -- Number of points along te y-axis
 *  size_t numX = (tX < 0)? 0 : (size_t)(tX);
 *  size_t numY = (tY < 0)? 0 : (size_t)(tY);
 *
 *  LogSevere("Grid size is " << numX << " * " << numY << "\n");
 *   g2float* g2grid =  gfld2->fld;
 *   DataStore2D<float> holder(numX, numY);
 *
 *  for(size_t x=0; x<numX; ++x){
 *    for(size_t y = 0; y<numY; ++y){
 *       holder.set(x,y,g2grid[y*numX+x]);
 *    }
 *  }
 *  for(size_t x = 0; x < 10; ++x){
 *    for(size_t y = 0; y < 10; ++y){
 *      std::cout << holder.get(x,y) << ",  ";
 *    }
 *    std::cout << "\n";
 *  }
 */
// return false;

GribMessageMatcher::GribMessageMatcher(g2int d, g2int c, g2int p) : myDisciplineNumber(d), myCategoryNumber(c),
  myParameterNumber(p)
{
  myMode = 0;
}

bool
GribMessageMatcher::action(gribfield * gfld, size_t fieldNumber)
{
  // Key1: Grib2 discipline is also in the header, but we can get it here
  g2int disc = gfld->discipline;

  if (disc != myDisciplineNumber) { return false; }
  // FIXME: is this always valid?

  // Grib2 PRODUCT definition template number.  Tells you which Template 4.x defines
  // the stuff in the ipdtmpl.  Technically we would have to check all of these to
  // make sure the Product Category is the first number...
  // int pdtn = gfld->ipdtnum;
  // Key 2: Category  Section 4, Octet 10 (For all Templates 4.X)
  g2int pcat = gfld->ipdtmpl[0];

  if (pcat != myCategoryNumber) { return false; }

  // Key 3: Parameter Number.  Section 4, Octet 11
  g2int pnum = gfld->ipdtmpl[1];

  if (pnum != myParameterNumber) { return false; }

  // Grib2 Grid Definition Template (GDT) number.  This tells how the grid is projected...
  // const g2int gdtn = gfld->igdtnum;
  return true; // stop scanning
}

GribMatcher::GribMatcher(const std::string& key,
  const std::string                       & levelstr) : myKey(key), myLevelStr(levelstr),
  myMatched(false), myMatchAt(0), myMatchFieldNumber(0)
{ }

bool
GribMatcher::action(gribfield * gfld, size_t fieldNumber)
{
  bool match = IOGrib::match(gfld, myKey, myLevelStr);

  if (match) {
    LogSevere("**********MATCHED " << myKey << " and " << myLevelStr << "\n");
    myMatched = true;
    myMatchAt = myBufferAt;
    myMatchFieldNumber = fieldNumber;
  }

  return true;
}

bool
GribMatcher::getMatch(size_t& at, size_t& fieldNumber)
{
  at = myMatchAt;
  fieldNumber = myMatchFieldNumber;
  return myMatched;
}

namespace {
/** Create my own copy of gbits and gbit.  It looks like the old implementation overflows
 * on large files since it counts number of bits so large files say 1 GB can overflow
 * the g2int size which is an int.
 * g2clib isn't that big and it appears not updated, so we may branch and do some work here.*/
void
_gbits(unsigned char * in, g2int * iout, unsigned long long iskip, g2int nbyte, g2int nskip,
  g2int n)

/*          Get bits - unpack bits:  Extract arbitrary size values from a
 * /          packed bit string, right justifying each value in the unpacked
 * /          iout array.
 * /           *in    = pointer to character array input
 * /           *iout  = pointer to unpacked array output
 * /            iskip = initial number of bits to skip
 * /            nbyte = number of bits to take
 * /            nskip = additional number of bits to skip on each iteration
 * /            n     = number of iterations
 * / v1.1
 */
{
  g2int i, tbit, bitcnt, ibit, itmp;
  unsigned long long nbit, index;
  static g2int ones[] = { 1, 3, 7, 15, 31, 63, 127, 255 };

  //     nbit is the start position of the field in bits
  nbit = iskip;
  for (i = 0; i < n; i++) {
    bitcnt = nbyte;
    index  = nbit / 8;
    ibit   = nbit % 8;
    nbit   = nbit + nbyte + nskip;

    //        first byte
    tbit = ( bitcnt < (8 - ibit) ) ? bitcnt : 8 - ibit; // find min

    itmp = (int) *(in + index) & ones[7 - ibit];

    if (tbit != 8 - ibit) { itmp >>= (8 - ibit - tbit); }
    index++;
    bitcnt = bitcnt - tbit;

    //        now transfer whole bytes
    while (bitcnt >= 8) {
      itmp   = itmp << 8 | (int) *(in + index);
      bitcnt = bitcnt - 8;
      index++;
    }

    //        get data from last byte
    if (bitcnt > 0) {
      itmp = ( itmp << bitcnt ) | ( ((int) *(in + index) >> (8 - bitcnt)) & ones[bitcnt - 1] );
    }

    *(iout + i) = itmp;
  }
} // _gbits

/** gbit just passes to gbit2 */
void
_gbit(unsigned char * in, g2int * iout, unsigned long long iskip, g2int nbyte)
{
  _gbits(in, iout, iskip, nbyte, (g2int) 0, (g2int) 1);
}
}

bool
IOGrib::scanGribData(std::vector<char>& b, GribAction * a)
{
  size_t aSize       = b.size();
  unsigned char * bu = (unsigned char *) (&b[0]);
  // LogSevere("GRIB2 incoming: buffer size " << aSize << "\n");

  // Seekgb is actually kinda simple, it finds a single grib2 message and its
  // length into a FILE*.  We want to use a buffer though..this gives us the
  // advantage of being able to use a URL/ram, etc.
  // seekdb.c has the c code for this from a FILE*.  The downside
  // to us doing this is that we need to check future versions of seekdb.c
  // for bug fixes/changes.
  size_t messageCount = 0;
  // size_t k = 0;
  unsigned long long k = 0;


  g2int lengrib = 0;

  while (k < aSize) {
    g2int start = 0;
    g2int vers  = 0;
    _gbit(bu, &start, (k + 0) * 8, 4 * 8);
    _gbit(bu, &vers, (k + 7) * 8, 1 * 8);

    if (( start == 1196575042) && (( vers == 1) || ( vers == 2) )) {
      //  LOOK FOR '7777' AT END OF GRIB MESSAGE
      if (vers == 1) {
        _gbit(bu, &lengrib, (k + 4) * 8, 3 * 8);
      }
      if (vers == 2) {
        _gbit(bu, &lengrib, (k + 12) * 8, 4 * 8);
      }
      //    ret=fseek(lugb,ipos+k+lengrib-4,SEEK_SET);
      const size_t at = k + lengrib - 4;
      if (at + 3 < aSize) {
        g2int k4 = 1;
        //    k4=fread(&end,4,1,lugb); ret fails overload
        int end = int((unsigned char) (b[at]) << 24
          | (unsigned char) (b[at + 1]) << 16
          | (unsigned char) (b[at + 2]) << 8
          | (unsigned char) (b[at + 3]));
        if (( k4 == 1) && ( end == 926365495) ) { // GRIB message found
          messageCount++;
          //   OUTPUT ARGUMENTS:
          //     listsec0 - pointer to an array containing information decoded from
          //                GRIB Indicator Section 0.
          //                Must be allocated with >= 3 elements.
          //                listsec0[0]=Discipline-GRIB Master Table Number
          //                            (see Code Table 0.0)
          //                listsec0[1]=GRIB Edition Number (currently 2)
          //                listsec0[2]=Length of GRIB message
          //     listsec1 - pointer to an array containing information read from GRIB
          //                Identification Section 1.
          //                Must be allocated with >= 13 elements.
          //                listsec1[0]=Id of orginating centre (Common Code Table C-1)
          //                listsec1[1]=Id of orginating sub-centre (local table)
          //                listsec1[2]=GRIB Master Tables Version Number (Code Table 1.0)
          //                listsec1[3]=GRIB Local Tables Version Number
          //                listsec1[4]=Significance of Reference Time (Code Table 1.1)
          //                listsec1[5]=Reference Time - Year (4 digits)
          //                listsec1[6]=Reference Time - Month
          //                listsec1[7]=Reference Time - Day
          //                listsec1[8]=Reference Time - Hour
          //                listsec1[9]=Reference Time - Minute
          //                listsec1[10]=Reference Time - Second
          //                listsec1[11]=Production status of data (Code Table 1.2)
          //                listsec1[12]=Type of processed data (Code Table 1.3)
          //     numfields- The number of gridded fields found in the GRIB message.
          //                That is, the number of occurences of Sections 4 - 7.
          //     numlocal - The number of Local Use Sections ( Section 2 ) found in
          //                the GRIB message.
          //

          g2int listsec0[3], listsec1[13], numlocal;
          g2int myNumFields = 0;
          int ierr = g2_info(&bu[k], listsec0, listsec1, &myNumFields, &numlocal);
          if (ierr > 0) {
            LogSevere(getGrib2Error(ierr));
          } else {
            if (a != nullptr) {
              a->setG2Info(messageCount, k, listsec0, listsec1, numlocal);
            }
            // Can check center first before info right?
            // g2int cntr = listsec1[0]; // Id of orginating centre
            // g2int mtab = listsec1[2]; // =GRIB Master Tables Version Number (Code Table 1.0)

            const size_t fieldNum = (myNumFields < 0) ? 0 : (size_t) (myNumFields);
            for (size_t f = 1; f <= fieldNum; ++f) {
              gribfield * gfld = 0; // output
              g2int unpack     = 0; // 0 do not unpack
              g2int expand     = 0; // 1 expand the data?
              ierr = g2_getfld(&bu[k], f, unpack, expand, &gfld);

              if (ierr > 0) {
                LogSevere(getGrib2Error(ierr));
              } else {
                if (a != nullptr) { // FIXME: if we're null what's the point of looping?
                  bool keepGoing = a->action(gfld, f);
                  if (keepGoing == false) {
                    g2_free(gfld);
                    return true;
                  }
                }
              }
              g2_free(gfld);
            }
          }

          // End of message match...
        } else { // No hit?  Forget it
          break;
        }
      } else {
        LogSevere("Out of bounds\n");
        break;
      }
    } else {
      LogSevere("No grib2 matched data in buffer\n");
      break;
    }
    k += lengrib; // next message...
  }

  LogInfo("Total messages: " << messageCount << "\n");
  return true;
} // myseekgbbuf

std::string
IOGrib::getHelpString(const std::string& key)
{
  std::string help;

  help += "builder that uses the grib/grib2 library to read grib data files.";
  return help;
}

void
IOGrib::initialize()
{ }

/** a quick dirty wgrib2 gribtab.dat reader to make a lookup database.
 * Usually we look up by name and level and strings are easier for
 * the user.  Grib2 really should have just used a *@#*%) string in
 * it's original design.
 * We'll convert the format at some point to csv probably
 */
void
readGribDatabase()
{
  static bool firstTime = true;

  if (!firstTime) { return; }
  firstTime = false;

  readLevels();
  readNCEPLocalLevels(); // center == 7
  readTable4dot4();      // Indicator of unit of time range

  const URL url = Config::getConfigFile("gribtab.dat");

  if (url.empty()) {
    LogSevere("Grib2 reader requires a gribtab.dat file in your configuration\n");
    exit(1);
  }
  std::vector<char> buf;

  IOURL::read(url, buf);

  if (!buf.empty()) {
    LogSevere("Read the dat file\n");

    size_t at       = 0;
    size_t size     = buf.size();
    size_t state    = 0;
    size_t fieldnum = 0;
    std::stringstream ss;

    // What we're getting per line
    int disc = -1;
    int mtab = -1;
    int cntr = -1;
    int ltab = -1;
    int pcat = -1;
    int pnum = -1;
    std::string key, description, units;

    /*
     * int disc;   Section 0 Discipline // field
     * int mtab;   Section 1 Master Tables Version Number
     * int cntr;   Section 1 originating centre, used for local tables // higher
     * int ltab;   Section 1 Local Tables Version Number
     * int pcat;   Section 4 Template 4.0 Parameter category // field
     * int pnum;   Section 4 Template 4.0 Parameter number // field
     *              //   { 0, 1, 0, 0, 5, 4, "ULWRF", "Upward Long-Wave Rad. Flux", "W/m^2"},
     *              //   { 0, 0, 7, 1, 5, 193, "ULWRF", "Upward Long-Wave Rad. Flux", "W/m^2"}
     */
    // Stupid quick 10 min DFA.  Yay computer science classes, lol
    // Of course if we end up using tables we'll make a better
    // format.  This _will_ break easily with a bad data file
    while (at < size) {
      const char c = buf[at];
      switch (state) {
          case 0: /* begin */
            if ((c == '{') | (c == '}') || (c == ' ') | (c == ',') || (c == '\n')) { break; } // keep eating
            // otherwise start a field...
            if (c == '"') {
              state = 1; // now in a word
            } else {     // assume number
              ss << c;   // add to field data
              state = 2; // now in a number
            }
            break;
          case 1: /* inside a word, waiting for end quote */
            if (c != '"') {
              ss << c;
            } else {
              //  LogSevere(fieldnum << ": Word: '" << ss.str() << "'\n");
              if (fieldnum == 6) {
                key = ss.str();
              } else if (fieldnum == 7) {
                description = ss.str();
              } else if (fieldnum == 8) {
                units = ss.str();
              }
              ss.str("");
              ss.clear();
              state = 0;
              if (++fieldnum > 8) {
                fieldnum = 0;
                //           LogSevere("Final: " << disc <<"," << mtab <<"," << cntr << "," << ltab << "," << pcat
                //             << "," << pnum << " " << key << ":"<<description<<":"<<units<< "\n");
                myLookup.push_back(GribLookup(disc, mtab, cntr, ltab, pcat, pnum, key, description, units));
              }
            }
            break;
          case 2: /* in a number */
            if (c != ',') {
              ss << c;
            } else {
              //  LogSevere(fieldnum << ": Number: '" << ss.str() << "'\n");
              int f = std::stoi(ss.str());
              // int disc, mtab, cntr, ltab, pcat, pnum;
              if (fieldnum == 0) {
                disc = f;
              } else if (fieldnum == 1) {
                mtab = f;
              } else if (fieldnum == 2) {
                cntr = f;
              } else if (fieldnum == 3) {
                ltab = f;
              } else if (fieldnum == 4) {
                pcat = f;
              } else if (fieldnum == 5) {
                pnum = f;
              }
              ss.str("");
              ss.clear();
              state = 0;
              fieldnum++;
              if (fieldnum > 8) { fieldnum = 0; }
            }
            break;
          default:
            break;
      }
      ++at;
    }
  }

  // Ok the levels?

  // exit(1);
} // readGribDatabase

std::shared_ptr<DataType>
IOGrib::readGribDataType(const URL& url)
{
  // HACK IN MY GRIB.data thing for moment...
  // Could lazy read only on string matching...
  readGribDatabase();

  // Note, in RAPIO we can read a grib file remotely too
  std::vector<char> buf;

  IOURL::read(url, buf);

  if (!buf.empty()) {
    // FIXME: Validate the format of the GRIB2 file first, then
    // pass it onto the DataType object?
    //  GribSanity test;
    //  if scanGribData(buf, &test) good to go;
    std::shared_ptr<GribDataTypeImp> g = std::make_shared<GribDataTypeImp>(buf);
    return g;
  } else {
    LogSevere("Couldn't read data for grib2 at " << url << "\n");
  }

  return nullptr;
} // IOGrib::readGribDataType

std::shared_ptr<DataType>
IOGrib::createDataType(const std::string& params)
{
  // virtual to static, we only handle file/url
  return (IOGrib::readGribDataType(URL(params)));
}

bool
IOGrib::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>             & params
)
{
  return false;
}

IOGrib::~IOGrib(){ }
