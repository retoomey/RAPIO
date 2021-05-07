#pragma once

#include "rIODataType.h"
#include "rDataGrid.h"
#include "rIO.h"

extern "C" {
#include <grib2.h>
}

#include <iomanip>

namespace rapio {
/** Root class of an action taken on each message of a Grib2 file.
 * FIXME: Maybe have a rIOGribUtil, organize better, etc.
 *
 * @author Robert Toomey */
class GribAction : public IO
{
public:
  virtual bool
  action(gribfield * gfld, size_t fieldNumber)
  {
    return false; // Keep going?
  }

  /** Info sent at start of a valid Grib2 message.  This information can be used in the action method.
   * Debated copying this vs passing with each action call */
  virtual void
  setG2Info(size_t messageNum, size_t at, g2int listsec0[3], g2int listsec1[13], g2int numlocal);

  /** Return a wgrib2 like date string 2019082023 from current g2info */
  virtual std::string
  getDateString();

protected:
  /** Current message number in Grib2 data */
  size_t myMessageNum;

  /** Current location in buffer */
  size_t myBufferAt;

  /** Current Section 0 fields */
  g2int mySection0[3];

  /** Current Section 1 fields */
  g2int mySection1[13];

  /** Current number of local */
  g2int myNumLocal;
};

class GribCatalog : public GribAction
{
public:
  virtual bool
  action(gribfield * gfld, size_t fieldNumber) override;

protected:
  std::vector<std::string> myList;
};

/** First attempt at a grib matcher class.  This simple one matches
 * Discipline, Category, Parameter for data */
class GribMessageMatcher : public GribAction
{
public:
  // Match Grid data by numbers
  g2int myDisciplineNumber;
  g2int myCategoryNumber;
  g2int myParameterNumber;
  int myMode;

  GribMessageMatcher(g2int d, g2int c, g2int p);

  virtual bool
  action(gribfield * gfld, size_t fieldNumber) override;
};

class GribMatcher : public GribAction
{
  std::string myKey;
  std::string myLevelStr;
  bool myMatched;
  size_t myMatchAt;
  size_t myMatchFieldNumber;
public:
  GribMatcher(const std::string& key, const std::string& levelstr);
  virtual bool
  action(gribfield * gfld, size_t fieldNumber) override;

  /** Get match at and field number for grib reading */
  bool
  getMatch(size_t& at, size_t& fieldNumber);
};

/**
 * The base class of all Grib/2/3/etc formatters.
 *
 * @author Robert Toomey
 */
class IOGrib : public IODataType {
public:

  // FIXME: We might have a bunch of strings not just name and
  // level.

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

  /** Help for grib module */
  virtual std::string
  getHelpString(const std::string& key);

  // Registering of classes ---------------------------------------------

  virtual void
  initialize() override;

  /** Get grib2 error into string. */
  static std::string
  getGrib2Error(g2int ierr);

  /** Get grib2 error into string. */
  static bool
  scanGribData(std::vector<char>& b, GribAction * a);

  // READING ------------------------------------------------------------
  //

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(const std::string& param) override;

  /** Do a buffer read of a 2D field */
  static std::shared_ptr<Array<float, 2> >
  get2DData(std::vector<char>& b, size_t at, size_t fieldNumber, size_t& x, size_t& y);

  /** Do a buffer read of a 3D field using 2D and input vector of levels */
  static std::shared_ptr<Array<float, 3> >
  get3DData(std::vector<char>& b, const std::string& key, const std::vector<std::string>& levelsStringVec, size_t& x,
    size_t& y, size_t& z, float missing = -999.0);

  /** Do a full read from a param list */
  static std::shared_ptr<DataType>
  readGribDataType(const URL& path);

  // WRITING ------------------------------------------------------------

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    std::map<std::string, std::string>     & params
  ) override;

  virtual
  ~IOGrib();
};
}
