#pragma once

#include "rIO.h"
#include "rGribDataType.h"

// #include <vector>
#include <string>

extern "C" {
#include <grib2.h>
}

namespace rapio {
// FIXME: Might move all this in to core since these now only
// rely on the general interface.

/** An action that simply prints the wgrib2 or idx format out
 * for each message in the grib2 data */
class GribCatalog : public GribAction
{
public:
  virtual bool
  action(std::shared_ptr<GribMessage>& m, size_t fieldNumber) override;
};

#if 0

/** First attempt at a grib matcher class.  This simple one matches
 * Discipline, Category, Parameter for data.
 * FIXME: Nothing calls this.  I'm leaving it for now it might
 * not even work anymore */
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
  action(std::shared_ptr<GribMessage>& m, size_t fieldNumber) override;
};
#endif // if 0

class GribMatcher : public GribAction
{
  /** The key we look for */
  std::string myKey;

  /** The level we look for */
  std::string myLevelStr;

  /** Matched message, or nullptr */
  std::shared_ptr<GribMessage> myMatchedMessage;

  /** Match field number iff matched message */
  size_t myMatchedFieldNumber;

public:

  /** Create a single field first come matcher */
  GribMatcher(const std::string& key, const std::string& levelstr);

  /** Matched message */
  virtual bool
  action(std::shared_ptr<GribMessage>& m, size_t fieldNumber) override;

  /** The messsage we matched if any, and other info. */
  std::shared_ptr<GribMessage> getMatchedMessage(){ return myMatchedMessage; }

  /** The field number we matched if any */
  size_t getMatchedFieldNumber(){ return myMatchedFieldNumber; }
};

/** Matcher matching N fields in the grib2 source.  Used for our 3D creation */
class GribNMatcher : public GribAction
{
  /** The key we look for */
  std::string myKey;

  /** The vector of levels we look for */
  std::vector<std::string> myLevels;

  /** Matched message, or nullptr */
  std::vector<std::shared_ptr<GribMessage> > myMatchedMessages;

  /** Matched field numbers */
  std::vector<size_t> myMatchedFieldNumbers;

  /** Count of matched to know we got them all */
  size_t myMatchedCount;

public:
  /** Create a N level, single pass matcher */
  GribNMatcher(const std::string& key, std::vector<std::string>& levels);

  /** Action to take on a GribMessage */
  virtual bool
  action(std::shared_ptr<GribMessage>& m, size_t fieldNumber) override;

  /** Return true if we found all levels and tell what are missing */
  bool
  checkAllLevels();

  /** The messages we matched if any. */
  std::vector<std::shared_ptr<GribMessage> >& getMatchedMessages(){ return myMatchedMessages; }

  /** The field numbers we matched if any. */
  std::vector<size_t>& getMatchedFieldNumbers(){ return myMatchedFieldNumbers; }
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
  /** The caller that wants the time set */
  DataType * myCaller;

public:
  GribScanFirstMessage(DataType * caller) : myCaller(caller){ };

  virtual bool
  action(std::shared_ptr<GribMessage>& m, size_t fieldNumber) override;
};
}
