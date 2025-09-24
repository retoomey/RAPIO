#include "rGribDatabase.h"
#include "rConfig.h"
#include "rError.h"
#include "rStrings.h"
#include "rIOURL.h"

using namespace rapio;

extern "C" {
#include <grib2.h>
}

// Lookup tables for moment for getting numbers into strings
NumberToStringLookup GribDatabase::ncep;
NumberToStringLookup GribDatabase::levels;
NumberToStringLookup GribDatabase::table4dot4;
std::vector<GribLookup> GribDatabase::myLookup;

/** Simple add */
void
NumberToStringLookup::add(int v, const std::string& s)
{
  for (auto& i:myValues) {
    if (i == v) {
      // LogSevere("-------------------->DUPLICATE ADD OF VALUE " << i << "\n");
      return; // Can't duplicate
    }
  }
  myValues.push_back(v);
  myStrings.push_back(s);
}

/** Simple get */
std::string
NumberToStringLookup::get(int v)
{
  for (size_t i = 0; i < myValues.size(); ++i) {
    if (myValues[i] == v) {
      return myStrings[i];
    }
  }
  return "";
}

/** Should be from disk probably */
void
GribDatabase::readNCEPLocalLevels()
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
GribDatabase::readLevels()
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
  levels.add(106, "%g m below ground");
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
GribDatabase::readTable4dot4()
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

GribLookup *
GribDatabase::huntDatabase(int d, int m, int c, int l, int pc, int pn,
  const std::string& k, const std::string& des)
{
  // GribLookup* l = huntDatabase(disc, 0, 0, 0, pcat, pnum, "", "");
  for (auto &i:myLookup) {
    bool match = true;
    match &= ((d == -1) || (d == i.disc));
    match &= ((pc == -1) || (pc == i.pcat));
    match &= ((pn == -1) || (pn == i.pnum));
    if (match) {
      // LogSevere("MATCHED " << d <<", " << pc << ", " << "m in " << m << " == " << i.mtab << "\n");
      return &i;
    }
  }
  return nullptr;
}

std::string
GribDatabase::getProductName(gribfield * gfld)
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
} // GribDatabase::getProductName

std::string
GribDatabase::getLevelName(gribfield * gfld)
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
GribDatabase::readGribDatabase()
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
    LogInfo("Read the gribtab.dat wgrib style file...no .idx support (todo).\n");

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
} // readGribDatabase

void
GribIDXField::print(const size_t messageNumber, bool printFieldNumber)
{
  // std::string theTime = myTime.getString("%Y%m%d%H");
  std::string theTime = myDateString;

  std::cout << messageNumber;
  if (printFieldNumber) {
    std::cout << "." << myFieldNumber;
  }
  std::cout << ":" << myOffset << ":" << theTime << ":" << myProduct << ":" << myLevel << ":" << myType << ":\n";
}

void
GribIDXMessage::print()
{
  bool printFieldNumber = (myFields.size() > 1);

  for (auto& f:myFields) {
    f.print(myMessageNumber, printFieldNumber);
  }
}

bool
GribDatabase::parseIDXLine(const std::string& line, GribIDXField& idx, size_t& messageNumber)
{
  // Recover from bad data, etc.
  try {
    // -----------------------------------
    // Now process the line fields
    // 0    1           2        3     4     5    6
    // 105:3102971:d=2024042900:RWMR:275 mb:anl:
    // message: offset: date: productname levelName type
    //
    std::vector<std::string> out;
    Strings::split(line, ':', &out);

    // Size should be at least 5...
    if (out.size() < 5) {
      return false;
    }

    // ---------
    // Handle the message field.. (105.1) [0]
    std::vector<std::string> message;
    Strings::split(out[0], '.', &message);
    if (message.size() > 0) {
      messageNumber     = std::stoull(message[0]);
      idx.myFieldNumber = (message.size() > 1) ? std::stoull(message[1]) : 1;
    } else {
      return false;
    }

    // ---------
    // Handle the offset field.. (3102971) [1]
    idx.myOffset = std::stol(out[1]); // can throw
    // ---------
    // Handle the date field [2]
    // FIXME: Issue here is that wgrib2 seems to have many versions of this
    // in the field (see wgrib2/Sec1.c) So for now, we'll just store the
    // string.
    #if 0
    // One format I've see is d=YYYYMMDDHH
    std::vector<std::string> outdate;
    Strings::split(out[2], '=', &outdate);
    if ((outdate.size() == 2) && (outdate[0] == "d")) {
      // LogDebug("DATE: " << outdate[1] << "\n");
      const std::string convert = "%Y%m%d%H"; // One of the Grib formats it seems
      idx.myTime = Time(outdate[1], convert);
      // LogDebug("Converted Time is " << idx.myTime.getString(convert) << "\n");
    } else {
      break;
      // bad date?
    }
    #endif // if 0
    idx.myDateString = out[2];

    // ---------
    // Handle the product and field names [3, 4]
    idx.myProduct = out[3];
    idx.myLevel   = out[4];
    idx.myType    = out[5];
  }catch (const std::exception& e)
  {
    // Any failure, get out
    LogSevere("Error reading IDX file:\n");
    LogSevere(e.what());
    LogSevere("Ignoring IDX file catalog and using internal catalog.\n");
    return false;
  }
  return true;
} // GribDatabase::parseIDXLine

bool
GribDatabase::readIDXFile(const URL& url, std::vector<GribIDXMessage>& messages)
{
  std::vector<char> buf;

  IOURL::read(url, buf);

  if (buf.empty()) {
    LogSevere("Couldn't read the idx file at " << url << "\n");
    return false;
  }

  bool haveAtLeastOne = false;
  bool success        = true;
  size_t atMessage    = 1;

  messages.push_back(GribIDXMessage(atMessage));

  auto it  = buf.begin();
  auto end = buf.end();
  size_t lineCounter = 0;

  while (it != end) {
    // Gather a line from the char buffer
    std::string line;
    while (it != end && *it != '\n') {
      line.push_back(*it);
      ++it;
    }

    // Parse the line sections.  Chicken egg here we don't
    // know field number until after reading it.
    GribIDXField idx;
    size_t messageNumber;
    if (parseIDXLine(line, idx, messageNumber)) {
      // Moving forward to next message, create it...
      if (messageNumber == atMessage + 1) {
        messages.push_back(GribIDXMessage(atMessage + 1));
        atMessage++;
        // ...otherwise better match current message
      } else if (messageNumber != atMessage) {
        LogSevere(
          "Expected message " << atMessage << " or " << atMessage + 1 << " and got " << messageNumber <<
            " in IDX file.\n");
        return false;
      }
      // Add field to the current message slot
      messages[atMessage - 1].myFields.push_back(idx);

      haveAtLeastOne = true;
    } else {
      LogSevere("Error reading line " << lineCounter << " of IDX file:\n");
      LogSevere("Ignoring IDX file catalog and using internal.\n");
      return false;
    }

    // Skip the newline character if present
    if ((it != end) && (*it == '\n')) {
      ++it;
    }
    lineCounter++;
  }

  // If no errors and at least one record, we'll take it
  if (success && haveAtLeastOne) {
    LogInfo("Total IDX lines parsed: " << lineCounter << "\n");
  } else {
    messages.clear(); // Not gonna be used
  }
  return true;
} // GribDatabase::readIDXFile
