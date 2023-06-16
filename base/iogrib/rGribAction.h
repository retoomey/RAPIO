#pragma once

#include "rIO.h"
#include "rDataType.h"

#include <vector>
#include <string>

extern "C" {
#include <grib2.h>
}

namespace rapio {
/** Root class of an action taken on each message of a Grib2 file.
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

/** Grib that snags the time info out of the very first field.
 * This is a time hack since for the data we typically read the time
 * is the same for every message.  We'll use it as a global time for the
 * datatype for now. Note: If there are no fields, eh..guess we'll use
 * current time?
 * The issue is that units won't be global at a minimum so our API
 * will have to change somewhat.*/
class GribScanFirstMessage : public GribAction
{
  /** The caller that want the time set */
  DataType * myCaller;

public:
  GribScanFirstMessage(DataType * caller) : myCaller(caller){ };

  virtual bool
  action(gribfield * gfld, size_t fieldNumber) override;
};
}
