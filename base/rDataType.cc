#include "rDataType.h"

#include "rError.h"
#include "rTimeDuration.h"
#include "rTime.h"
#include "rUnit.h"
#include "rLLH.h"
#include "rStrings.h"

using namespace rapio;

DataType::DataType() : myTypeName("not set")
{ }

std::string
DataType::formatString(float spec,
  size_t                     tot,
  size_t                     prec) const
{
  char buf[100];

  std::string format("%f");

  if (prec > 0) {
    sprintf(buf, "0%d.%df", int(tot), int(prec)); // e.g:  06.3f
    format = "%" + std::string(buf);
  }

  // unfortunately, sprintf can randomly put either -0.00 or 0.00
  // this will cause problems to downstream applications.
  if (fabs(spec) < 0.0001) {
    sprintf(buf, format.c_str(), fabs(spec)); // force
                                              // +ve
                                              // value
  } else { sprintf(buf, format.c_str(), spec); }
  return (buf);
}

const std::string&
DataType::getTypeName() const
{
  if (myTypeName.empty()) {
    LogSevere("Type Name unset" << "\n");
  }
  return (myTypeName);
}

void
DataType::setTypeName(const std::string& typeName)
{
  myTypeName = typeName;
}

bool
DataType::getSubType(std::string& result) const
{
  std::string subtype;
  auto f = myAttributes.get<std::string>("SubType");
  if (f) {
    subtype = *f;
  }

  if (!subtype.empty()) {
    if (subtype != "NoPRI") { // Special case
      result = subtype;
      return (true);
    }
  }

  result = getGeneratedSubtype();
  return (false);
}

void
DataType::setDataAttributeValue(
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
  myAttributes.put<std::string>(one, unit);
  myAttributes.put<std::string>(two, value);
}

bool
DataType::initFromGlobalAttributes()
{
  return true;
}

void
DataType::updateGlobalAttributes(const std::string& encoded_type)
{
  // FIXME: Actually general DataGrids might not want to stick all
  // this stuff in there.  Maybe a 'wdssii' flag or something here..
  // the class tree might not be good design

  myAttributes.put<std::string>(Constants::TypeName, getTypeName());

  // This 'could' be placed by encoders, but we pass it in
  myAttributes.put<std::string>(Constants::sDataType, encoded_type);

  // Store special time/location into global attributes
  // Should we just do in in set time and location?
  Time aTime    = getTime();
  LLH aLocation = getLocation();

  myAttributes.put<double>(Constants::Latitude, aLocation.getLatitudeDeg());
  myAttributes.put<double>(Constants::Longitude, aLocation.getLongitudeDeg());
  myAttributes.put<double>(Constants::Height, aLocation.getHeightKM() * 1000.0);

  myAttributes.put<long>(Constants::Time, aTime.getSecondsSinceEpoch());
  myAttributes.put<double>(Constants::FractionalTime, aTime.getFractional());

  // Standard missing/range
  myAttributes.put<float>("MissingData", Constants::MissingData);
  myAttributes.put<float>("RangeFolded", Constants::RangeFolded);

  // Regenerate attributes global attribute string, representing
  // all the -unit -value pairs.  This is used by netcdf, etc...legacy storage
  std::string attributes;
  for (auto& a:myAttributes) {
    auto name = a.getName();
    if (Strings::removeSuffix(name, "-value")) {
      auto unit = name + "-unit";
      if (myAttributes.index(unit) > -1) {
        // Found the pair, add to attribute string
        attributes += (" " + name);
      }
    }
  }
  myAttributes.put<std::string>("attributes", attributes);
} // DataType::updateGlobalAttributes
