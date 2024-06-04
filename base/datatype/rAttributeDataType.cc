#include "rAttributeDataType.h"

using namespace rapio;

AttributeDataType::AttributeDataType() : myAttributes(std::make_shared<DataAttributeList>())
{ }

std::shared_ptr<AttributeDataType>
AttributeDataType::Clone()
{
  auto nsp = std::make_shared<AttributeDataType>();

  AttributeDataType::deep_copy(nsp);
  return nsp;
}

void
AttributeDataType::deep_copy(std::shared_ptr<AttributeDataType> nsp)
{
  // Copy attributes
  auto & n = *nsp;

  n.myAttributes = myAttributes->Clone();
}

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
