#include "rGribAction.h"
#include "rGribDatabase.h"
#include "rError.h"

#include <iomanip>
#include <iostream>
#include <sstream>

using namespace rapio;

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
     << std::setfill('0') << std::setw(2) << mySection1[8]
     << "(" << std::setfill('0') << std::setw(2) << mySection1[9] << ")"
     << "(" << std::setfill('0') << std::setw(2) << mySection1[10] << ")";
  // minute/second ignored here
  return ss.str();
}

bool
GribCatalog::action(gribfield * gfld, size_t fieldNumber)
{
  std::string productName;
  std::string levelName;

  // A Wgrib2 style dump.
  GribDatabase::toCatalog(gfld, productName, levelName);
  std::cout << myMessageNum << ":" << myBufferAt << ":"
            << "d=" << getDateString() << ":" << productName << ":" << levelName << "\n";

  // testing if date is de-facto ever different in our files.  Each message can have a date,
  // so technically it doesn't have to match right?  We can make the 'global' date of the file
  // equal to it if it's always the same
  #if 0
  static bool firstDate = true;
  static std::string firstDateStr;
  if (firstDate) {
    firstDateStr = getDateString();
    firstDate    = false;
  } else {
    if (firstDateStr != getDateString()) {
      LogSevere("Got a different date: " << myMessageNum << ":" << getDateString() << " != " << firstDateStr << "\n");
      exit(1);
    }
  }
  #endif // if 0

  return true; // Keep going, do all messages
}

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
  bool match = GribDatabase::match(gfld, myKey, myLevelStr);

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

bool
GribScanFirstMessage::
action(gribfield * gfld, size_t fieldNumber)
{
  Time theTime = Time(mySection1[5], // year
      mySection1[6],                 // month
      mySection1[7],                 // day
      mySection1[8],                 // hour
      mySection1[9],                 // minute
      mySection1[10],                // second
      0.0                            // fractional
  );

  myCaller->setTime(theTime);

  return false; // only one field please
}
