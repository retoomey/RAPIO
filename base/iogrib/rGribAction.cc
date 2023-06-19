#include "rGribAction.h"
#include "rGribDatabase.h"
#include "rError.h"

#include <iostream>
#include <sstream>

using namespace rapio;

bool
GribCatalog::action(std::shared_ptr<GribMessageImp>& mp, size_t fieldNumber)
{
  // For now at least return a true grib2 file pointer
  // I could see wrapping this later, also action might handle N fields instead
  auto& m = *mp;
  gribfield * gfld = m.readFieldInfo(fieldNumber);

  if (gfld != nullptr) {
    // Look up in database.  This could be internal to GribMessage I think
    // when we implement .idx ability
    std::string productName;
    std::string levelName;
    GribDatabase::toCatalog(gfld, productName, levelName);

    std::cout << m.getMessageNumber() << ":" << m.getFileOffset() << ":"
              << "d=" << m.getDateString() << ":" << productName << ":" << levelName << "\n";
    g2_free(gfld);
  }
  return true; // Keep going, do all messages
}

#if 0
GribMessageMatcher::GribMessageMatcher(g2int d, g2int c, g2int p) : myDisciplineNumber(d), myCategoryNumber(c),
  myParameterNumber(p)
{
  myMode = 0;
}

bool
GribMessageMatcher::action(std::shared_ptr<GribMessageImp>& mp, size_t fieldNumber)
{
  auto& m = *mp;
  gribfield * gfld = m.readFieldInfo(fieldNumber);

  if (gfld == nullptr) { return true; } // stop on failure

  // Key1: Grib2 discipline is also in the header, but we can get it here
  g2int disc = gfld->discipline;

  if (disc != myDisciplineNumber) { g2_free(gfld); return false; }
  // FIXME: is this always valid?

  // Grib2 PRODUCT definition template number.  Tells you which Template 4.x defines
  // the stuff in the ipdtmpl.  Technically we would have to check all of these to
  // make sure the Product Category is the first number...
  // int pdtn = gfld->ipdtnum;
  // Key 2: Category  Section 4, Octet 10 (For all Templates 4.X)
  g2int pcat = gfld->ipdtmpl[0];

  if (pcat != myCategoryNumber) { g2_free(gfld); return false; }

  // Key 3: Parameter Number.  Section 4, Octet 11
  g2int pnum = gfld->ipdtmpl[1];

  if (pnum != myParameterNumber) { g2_free(gfld); return false; }

  // Grib2 Grid Definition Template (GDT) number.  This tells how the grid is projected...
  // const g2int gdtn = gfld->igdtnum;
  return true; // stop scanning
}

#endif // if 0

GribMatcher::GribMatcher(const std::string& key,
  const std::string                       & levelstr) : myKey(key), myLevelStr(levelstr),
  myMatchedFieldNumber(0)
{ }

bool
GribMatcher::action(std::shared_ptr<GribMessageImp>& mp, size_t fieldNumber)
{
  auto& m = *mp;
  gribfield * gfld = m.readFieldInfo(fieldNumber);

  if (gfld == nullptr) { return true; } // stop on failure

  bool match = GribDatabase::match(gfld, myKey, myLevelStr);

  if (match) {
    LogInfo("Found '" << myKey << "' and '" << myLevelStr << "' single match for 2D.\n");
    myMatchedMessage     = mp; // Hold onto this message
    myMatchedFieldNumber = fieldNumber;
  }
  g2_free(gfld);
  return !match; // Continue searching if no match
}

GribNMatcher::GribNMatcher(const std::string& key,
  std::vector<std::string>                  & levels) : myKey(key), myLevels(levels)
{
  myMatchedFieldNumbers.resize(myLevels.size());
  myMatchedMessages.resize(myLevels.size());
  myMatchedCount = 0;
}

bool
GribNMatcher::action(std::shared_ptr<GribMessageImp>& mp, size_t fieldNumber)
{
  auto& m = *mp;
  gribfield * gfld = m.readFieldInfo(fieldNumber);

  if (gfld == nullptr) { return true; } // stop on failure

  // Search a match for each level we have...
  for (size_t i = 0; i < myLevels.size(); i++) {
    bool match = GribDatabase::match(gfld, myKey, myLevels[i]);
    if (match) {
      LogInfo("Found '" << myKey << "' and '" << myLevels[i] << "'\n");
      if (myMatchedMessages[i] == nullptr) {
        myMatchedMessages[i]     = mp; // Hold onto this message
        myMatchedFieldNumbers[i] = fieldNumber;
        myMatchedCount++;
      } else { // Double match we ignored
        LogSevere("Double match seen for '" << myKey << "' and '" << myLevels[i] << "', ignoring.\n")
      }
    }
  }
  g2_free(gfld);

  const bool keepgoing = (myMatchedCount != myLevels.size());

  return (keepgoing);
}

bool
GribNMatcher::checkAllLevels()
{
  bool found = true;
  auto mv    = getMatchedMessages();

  for (size_t i = 0; i < mv.size(); i++) {
    if (mv[i] == nullptr) {
      LogSevere("Couldn't find '" << myKey << "' and level '" << myLevels[i] << "'\n");
      found = false;
    }
  }
  return found;
}

bool
GribScanFirstMessage::
action(std::shared_ptr<GribMessageImp>& m, size_t fieldNumber)
{
  myCaller->setTime(m->getTime());
  return false; // No fields we don't care
}
