#include "rDataTypeGroup.h"
#include "rRAPIOAlgorithm.h"

using namespace rapio;

DataTypeGroup::DataTypeGroup(const std::string& k)
  : myKey(k), myUseCustomWindow(false)
{ }

DataTypeGroup::DataTypeGroup(const std::string& k, const TimeDuration& customWindow)
  : myKey(k), myCustomWindow(customWindow), myUseCustomWindow(true)
{ }

void
DataTypeGroup::purgeTimeWindow(const Time& currentTime)
{
  TimeDuration window = myUseCustomWindow ? myCustomWindow : RAPIOAlgorithm::getMaximumHistory();

  for (size_t i = 0; i < myItems.size(); i++) {
    if ((currentTime - myItems[i]->getTime()) > window) {
      myItems.erase(myItems.begin() + i);
      i--;
    }
  }
}

std::shared_ptr<DataType>
DataTypeGroup::getDataType(const std::string& key) const
{
  for (const auto& item : myItems) {
    if (generateItemKey(item) == key) {
      return item;
    }
  }
  return nullptr;
}

bool
DataTypeGroup::removeDataType(const std::string& key)
{
  for (size_t i = 0; i < myItems.size(); ++i) {
    if (generateItemKey(myItems[i]) == key) {
      myItems.erase(myItems.begin() + i);
      return true;
    }
  }
  return false;
}

void
DataTypeGroup::clearGroup()
{
  myItems.clear();
}

std::string
DataTypeGroup::getKey() const
{
  return myKey;
}

const std::vector<std::shared_ptr<DataType> >&
DataTypeGroup::getItems() const
{
  return myItems;
}

// ---------------------------------------------------------
// ProductGroup Implementation
// ---------------------------------------------------------

std::string
ProductGroup::generateItemKey(const std::shared_ptr<DataType>& dt) const
{
  return dt->getTypeName();
}

void
ProductGroup::addDataType(std::shared_ptr<DataType> dt)
{
  if (!dt) { return; }

  const std::string targetKey = generateItemKey(dt);
  bool found = false;

  for (auto& item : myItems) {
    if (generateItemKey(item) == targetKey) {
      item  = dt;
      found = true;
      break;
    }
  }

  if (!found) {
    myItems.push_back(dt);
  }
}
