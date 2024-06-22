#include "rGribAction.h"
#include "rError.h"

#include <iostream>
#include <sstream>

using namespace rapio;

bool
GribCatalog::action(std::shared_ptr<GribMessage>& mp, size_t fieldNumber)
{
  if (fieldNumber == 1){
     std::cout << *mp;  // This will go ahead and print all fields
  }else{
    // We already printed all fields for this message...
  } 
  return true; // Keep going, do all messages
}

GribMatcher::GribMatcher(const std::string& key,
  const std::string                       & levelstr) : myKey(key), myLevelStr(levelstr),
  myMatchedFieldNumber(0)
{ }

bool
GribMatcher::action(std::shared_ptr<GribMessage>& mp, size_t fieldNumber)
{
  auto& m = *mp;
  auto f  = m.getField(fieldNumber);

  if (f == nullptr) { return true; } // stop on failure
  bool match = f->matches(myKey, myLevelStr);

  if (match) {
    LogInfo("Found '" << myKey << "' and '" << myLevelStr << "' single match for 2D.\n");
    myMatchedMessage     = mp; // Hold onto this message
    myMatchedFieldNumber = fieldNumber;
  }
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
GribNMatcher::action(std::shared_ptr<GribMessage>& mp, size_t fieldNumber)
{
  auto& m = *mp;
  auto f  = m.getField(fieldNumber);

  if (f == nullptr) { return true; } // stop on failure

  // Search a match for each level we have...
  for (size_t i = 0; i < myLevels.size(); i++) {
    const bool match = f->matches(myKey, myLevels[i]);
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
action(std::shared_ptr<GribMessage>& m, size_t fieldNumber)
{
  myCaller->setTime(m->getTime());
  return false; // No fields we don't care
}
