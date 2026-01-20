#include "rGribAction.h"
#include "rError.h"

#include <iostream>
#include <sstream>

using namespace rapio;

bool
GribCatalog::action(std::shared_ptr<GribMessage>& mp, size_t fieldNumber)
{
  if (fieldNumber == 1) {
    std::cout << *mp; // This will go ahead and print all fields
  } else {
    // We already printed all fields for this message...
  }
  return true; // Keep going, do all messages
}

GribMatcher::GribMatcher(const std::string& key,
  const std::string                       & levelstr) : myKey(key), myLevelStr(levelstr)
{ }

bool
GribMatcher::action(std::shared_ptr<GribMessage>& mp, size_t fieldNumber)
{
  auto& m = *mp;
  auto f  = m.getField(fieldNumber);

  if (f == nullptr) { return true; } // stop on failure
  bool match = f->matches(myKey, myLevelStr);

  if (match) {
    fLogInfo("Found '{}' and '{}' at message number: {}", myKey, myLevelStr, f->getMessageNumber());
    myMatchedMessages.push_back(mp);
    myMatchedFieldNumbers.push_back(fieldNumber);

    #if 0
    // Debug print out a few values
    auto array2D   = f->getFloat2D();
    auto maxX      = array2D->getX();
    auto maxY      = array2D->getY();
    auto& ref      = array2D->ref(); // or (*ref)[x][y]
    size_t counter = 0;
    for (size_t x = 0; x < maxX; ++x) {
      for (size_t y = 0; y < maxY; ++y) {
        if (ref[x][y] != 0) {
          if (++counter < 50) {
            std::cout << ref[x][y] << ",  ";
          }
        }
      }
    }
    std::cout << "\n";
    #endif // if 0
  }
  // We're matching whole file now, not stop on first
  return true;
  // Here we stop on the first successful match
  // return !match; // Continue searching if no match
} // GribMatcher::action

std::shared_ptr<GribMessage>
GribMatcher::getMatchedMessage()
{
  // Return the last match found for now.  This is how old legacy
  // code handled the field match.
  const size_t s = myMatchedMessages.size();

  if (s > 0) {
    return myMatchedMessages[s - 1];
  }
  return nullptr;
}

size_t
GribMatcher::getMatchedFieldNumber()
{
  const size_t s = myMatchedFieldNumbers.size();

  if (s > 0) {
    return myMatchedFieldNumbers[s - 1];
  }
  return 0;
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
      fLogInfo("Found '{}' and '{}'", myKey, myLevels[i]);
      if (myMatchedMessages[i] == nullptr) {
        myMatchedMessages[i]     = mp; // Hold onto this message
        myMatchedFieldNumbers[i] = fieldNumber;
        myMatchedCount++;
      } else { // Double match we ignored
        fLogSevere("Double match seen for '{}' and '{}', ignoring.", myKey, myLevels[i])
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
      fLogSevere("Couldn't find '{}' and level '{}'", myKey, myLevels[i]);
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
