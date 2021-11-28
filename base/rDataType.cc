#include "rDataType.h"

#include "rError.h"
#include "rTimeDuration.h"
#include "rTime.h"
#include "rUnit.h"
#include "rLLH.h"
#include "rStrings.h"
#include "rColorMap.h"
#include "rDataProjection.h"

using namespace rapio;

// Example: dirName/Reflectivity/00.50/TIMESTRING plus writer suffix such as .netcdf
std::string DataType::DATATYPE_PREFIX = "{base}/{datatype}{/subtype}/{time}";

// Example: dirName/TIMESTRING_Reflectivity_00.50 plus writer suffix (the subdirs flat flag from WDSS2)
std::string DataType::DATATYPE_PREFIX_FLAT = "{base}/{time}_{datatype}{_subtype}";

DataType::DataType() : myAttributes(std::make_shared<DataAttributeList>()), myTime(Time::CurrentTime()), myReadFactory(
    "default"), myTypeName("not set")
{ }

std::string
DataType::formatString(float spec,
  size_t                     tot,
  size_t                     prec)
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

std::string
DataType::getSubType() const
{
  std::string subtype;
  getString("SubType-value", subtype);

  if (!subtype.empty()) {
    if (subtype != "NoPRI") { // Special case
      return subtype;
    }
  }

  return (getGeneratedSubtype());
}

void
DataType::setSubType(const std::string& s)
{
  setDataAttributeValue("SubType", s);
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
  setString(one, unit);
  setString(two, value);
}

bool
DataType::initFromGlobalAttributes()
{
  return true;
}

void
DataType::updateGlobalAttributes(const std::string& encoded_type)
{
  setString(Constants::TypeName, getTypeName());

  // This 'could' be placed by encoders, but we pass it in
  setString(Constants::sDataType, encoded_type);

  // Location
  LLH aLocation = getLocation();
  setDouble(Constants::Latitude, aLocation.getLatitudeDeg());
  setDouble(Constants::Longitude, aLocation.getLongitudeDeg());
  setDouble(Constants::Height, aLocation.getHeightKM() * 1000.0);

  // Time
  Time aTime = getTime();
  setLong(Constants::Time, aTime.getSecondsSinceEpoch());
  setDouble(Constants::FractionalTime, aTime.getFractional());

  // Standard missing/range
  setFloat("MissingData", Constants::MissingData);
  setFloat("RangeFolded", Constants::RangeFolded);

  // Regenerate attributes global attribute string, representing
  // all the -unit -value pairs.  This is used by netcdf, etc...legacy storage
  std::string attributes;
  for (auto& a:*myAttributes) {
    auto name = a.getName();
    if (Strings::removeSuffix(name, "-value")) {
      auto unit = name + "-unit";
      if (myAttributes->index(unit) > -1) {
        // Found the pair, add to attribute string
        attributes += (" " + name);
      }
    }
  }
  setString("attributes", attributes);
} // DataType::updateGlobalAttributes

std::shared_ptr<ColorMap>
DataType::getColorMap()
{
  // First try the ColorMap-value attribute...Some MRMS netcdf files have this
  // if not found use the type name such as Reflectivity
  std::string colormap = "";
  if (!getString("ColorMap-value", colormap)) {
    colormap = getTypeName();
  }
  return ColorMap::getColorMap(colormap);
}
