#pragma once

#include "rUtility.h"
#include "rData.h"
#include "rIO.h"
#include "rURL.h"
#include "rTime.h"

#include <string>
#include <vector>

extern "C" {
#include <grib2.h>
}

namespace rapio {
/** Simple holder for field information from an idx file */
class GribIDXField : public Data {
public:
  size_t myFieldNumber; ///< Field number
  size_t myOffset;      ///< Offset in the grib data
  // Time myTime;          ///< Time unimplemented due to multiple IDX formats (for now)
  std::string myDateString; ///< Date string such as d=YYYYMMDDHH
  std::string myProduct;    ///< Product name
  std::string myLevel;      ///< Level name
  std::string myType;       ///< Type name such as anl

  /** Print the field */
  void
  print(const size_t messageNumber, bool printFieldNumber);
};

/** Simple holder for a message from an idx file.
 * Note, for lookup we want fields grouped, for example
 * the idx file can have 100.1 100.2 for multiple fields
 * of message 100 */
class GribIDXMessage : public Data {
public:

  /** Create a grib IDX message.  Since we'll vector this I'm
   * not sure we 'need' to store the message number.  We'll keep
   * it for now, but for memory could maybe remove this */
  GribIDXMessage(size_t m) : myMessageNumber(m){ }

  size_t myMessageNumber; ///< Message number

  std::vector<GribIDXField> myFields; ///< Fields for thie message

  /** Print the message fields */
  void
  print();
};

/** A simple lookup from number to string.
 * Using a vector which actually tends to be quicker than map in practice.
 * This is a two column table sorted as vectors.
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

  /** a quick dirty wgrib2 gribtab.dat reader to make a lookup database.
   * Usually we look up by name and level and strings are easier for
   * the user.
   */
  static void
  readGribDatabase();

  /** Called by GribField.  Return a wgrib2 style product name generated from table */
  static std::string
  getProductName(gribfield * gfld);

  /** Called by GribField.  Return a wgrib2 style level name generated from table */
  static std::string
  getLevelName(gribfield * gfld);

  /** Read a line of a idx file, returning field info and the read message number */
  static bool
  parseIDXLine(const std::string& line, GribIDXField& idx, size_t& messageNumber);

  /** Read a grib2 idx file into a GribIDXMessage ordered vector */
  static bool
  readIDXFile(const URL& url, std::vector<GribIDXMessage>& messages);

private:

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
};
}
