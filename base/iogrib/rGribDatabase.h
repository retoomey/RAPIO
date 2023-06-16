#pragma once

#include "rUtility.h"
#include "rData.h"
#include "rIO.h"

#include <string>
#include <vector>

extern "C" {
#include <grib2.h>
}

namespace rapio {
/** A simple lookup from number to string.
 * Using a vector which actually tends to be quicker than map in practice.
 * This is a two column table sorted as vectors.
 * FIXME: this should go somewhere or become generic
 *
 * @author Robert Toomey */
class NumberToStringLookup : public Data
{
public:
  /** Column 1 **/
  std::vector<int> myValues;

  /** Column 2 **/
  std::vector<std::string> myStrings;

  /** Simple add */
  void
  add(int v, const std::string& s);

  /** Simple get */
  std::string
  get(int v);

  /** Print it out to see */
  #if 0
  // I'm so lazy, make it operator<<
  void
  dump()
  {
    size_t s = myValues.size();

    for (size_t i = 0; i < s; ++i) {
      std::cout << myValues[i] << " == " << myStrings[i] << "\n";
    }
  }

  #endif // if 0
};

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

/**
 * Class for storage of all the string/index stuff for grib2 files.
 * We did some stuff for the .dat format with wgrib2 so I'm putting it here.
 * Newer grib2 has the .idx thing which if it exists, we can also use
 * for similar effect
 *
 * @author Robert Toomey
 */
class GribDatabase : public Utility {
public:

  // --------------------------------------------
  // Some internal tables similar to wgrib2
  // We can fall back on these for output or cases
  // where we can't find the modern .idx file

  static NumberToStringLookup ncep;
  static NumberToStringLookup levels;
  static NumberToStringLookup table4dot4;
  static std::vector<GribLookup> myLookup;

  /** Read NCEP local levels into lookup table */
  static void
  readNCEPLocalLevels();

  /** Read levels into lookup table */
  static void
  readLevels();

  /** Read Table 4.4 into lookup table */
  static void
  readTable4dot4();

  /** Hunt tables for information */
  static GribLookup *
  huntDatabase(int d, int m, int c, int l, int pc, int pn,
    const std::string& k, const std::string& des);

  /** a quick dirty wgrib2 gribtab.dat reader to make a lookup database.
   * Usually we look up by name and level and strings are easier for
   * the user.  Grib2 really should have just used a *@#*%) string in
   * it's original design.
   * We'll convert the format at some point to csv probably
   */
  static void
  readGribDatabase();

  /** Return a wgrib2 style product name generated from table */
  static std::string
  getProductName(gribfield * gfld);

  /** Return a wgrib2 style level name generated from table */
  static std::string
  getLevelName(gribfield * gfld);

  /** Match a field to given product and level */
  static bool
  match(gribfield * gfld, const std::string& productName,
    const std::string& levelName);

  /** Do a full convert to name, level etc.
   * FIXME: Matching might be quicker if we break back on name immediately */
  static void
  toCatalog(gribfield * gfld,
    std::string& productName, std::string& levelName);
};
}
