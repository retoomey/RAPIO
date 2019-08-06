#include "rDataType.h"

#include "rError.h"
#include "rTimeDuration.h"
#include "rTime.h"
#include "rUnit.h"

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

std::string
DataType::getDataAttributeString(const std::string& key) const
{
  auto at = myAttributes2.find(key);

  if (at != myAttributes2.end()) {
    return (at->second.getValue());
  }
  return ("");
}

bool
DataType::getRawDataAttribute(const std::string& key,
  std::string& value, std::string& unit) const
{
  auto at = myAttributes2.find(key);

  if (at != myAttributes2.end()) {
    value = at->second.getValue();
    unit  = at->second.getUnit();
    return (true);
  }
  return (false);
}

bool
DataType::getDataAttributeFloat(
  const std::string& key, float& value, const std::string& unit) const
{
  auto at = myAttributes2.find(key);

  if (at != myAttributes2.end()) {
    const auto d = at->second;

    if (unit.empty()) {
      value = atof(d.getValue().c_str());
    } else {
      float newV = Unit::value(d.getUnit(),
          unit,
          atof(d.getValue().c_str()));
      value = newV;
    }
    return (true);
  }
  return (false);
}

void
DataType::setDataAttributeValue(
  const std::string& key,
  const std::string& value,
  const std::string& unit
)
{
  // Don't allow empty keys or values at moment
  if (value.empty() || key.empty()) {
    return;
  }

  // Store replacement data attribute
  myAttributes2[key] = DataAttribute(value, unit);
}

void
DataType::clearAttribute(const std::string& key)
{
  myAttributes2.erase(key);
}

TimeDuration
DataType::getExpiryInterval() const
{
  double d;
  TimeDuration t;
  auto at = myAttributes2.find(Constants::ExpiryInterval);

  if (at != myAttributes2.end()) {
    d = atof(at->second.getValue().c_str());
    t = TimeDuration::Minutes(d);
  } else {
    // LogInfo( "No ExpiryInterval in data. Use default 15 minutes\n" );
    t = TimeDuration::Minutes(15);
  }
  return (t);
}

Time
DataType::getFilenameDateTime() const
{
  auto at = myAttributes2.find(Constants::FilenameDateTime);

  if (at != myAttributes2.end()) {
    return (Time(at->second.getValue()));
  }
  return (Time("00000000-000000"));
}

void
DataType::setFilenameDateTime(rapio::Time dateTime)
{
  setDataAttributeValue(Constants::FilenameDateTime,
    dateTime.getFileNameString(), "text");
}

std::string
DataType::getUnits()
{
  // Noticed it's either Units or units (bug).  Better to check for both then
  // for maximum compatibility with data.
  std::string unit = getDataAttributeString(Constants::Unit);

  if (!unit.empty()) {
    return (unit); // Could actually be ""
  }

  unit = getDataAttributeString("units"); // units

  if (!unit.empty()) { return (unit); }

  // Report units missing in file or not?
  std::string typeName = getTypeName();
  static size_t count  = 0;

  if (count == 0) {
    LogSevere("WARNING! " << typeName << " (" << Constants::Unit << ")"
                          << " does not have the Unit attribute set.\n");
  }
  count++;

  if (count > 1000) { count = 0; }

  // A default units for a given typename...for legacy datasets
  if (typeName == "Reflectivity") { return ("dBZ"); }

  if (typeName == "Velocity") { return ("MetersPerSecond"); }

  if (typeName == "DEM") { return ("Meters"); } return ("SI");
}
