#include "rAttributeDataType.h"

using namespace rapio;

AttributeDataType::AttributeDataType() : myAttributes(std::make_shared<DataAttributeList>())
{ }

std::shared_ptr<AttributeDataType>
AttributeDataType::Clone() const
{
  auto nsp = std::make_shared<AttributeDataType>();

  AttributeDataType::deep_copy(nsp);
  return nsp;
}

void
AttributeDataType::deep_copy(const std::shared_ptr<AttributeDataType>& nsp) const
{
  // Copy attributes
  auto & n = *nsp;

  n.myAttributes = myAttributes->Clone();
}

void
AttributeDataType::setDataAttributeValue(
  std::string_view key,
  std::string_view value,
  std::string_view unit
)
{
  // Don't allow empty keys or values
  if (value.empty() || key.empty()) {
    return;
  }

  // Stick as a pair into attributes.
  std::string attrName(key);

  attrName += "-unit";
  setString(attrName, unit);

  attrName.replace(key.size(), 5, "-value");
  setString(attrName, value);
}
