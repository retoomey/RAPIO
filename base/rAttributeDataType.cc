#include "rAttributeDataType.h"

using namespace rapio;

AttributeDataType::AttributeDataType() : myAttributes(std::make_shared<DataAttributeList>())
{ }

void
AttributeDataType::setDataAttributeValue(
  const std::string& key,
  const std::string& value,
  const std::string& unit
)
{
  // Don't allow empty keys or values
  if (value.empty() || key.empty()) {
    return;
  }

  // Stick as a pair into attributes.
  auto one = key + "-unit";
  auto two = key + "-value";

  setString(one, unit);
  setString(two, value);
}
