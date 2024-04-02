#include "rDataType.h"

#include "rError.h"
#include "rTimeDuration.h"
#include "rTime.h"
#include "rUnit.h"
#include "rLLH.h"
#include "rStrings.h"
#include "rColorMap.h"
#include "rDataProjection.h"
#include "rRecord.h"

using namespace rapio;

// Example: dirName/Reflectivity/00.50/TIMESTRING plus writer suffix such as .netcdf
std::string DataType::DATATYPE_PREFIX = "{source}/{datatype}{/subtype}/{time}";

// Example: dirName/TIMESTRING_Reflectivity_00.50 plus writer suffix (the subdirs flat flag from WDSS2)
std::string DataType::DATATYPE_PREFIX_FLAT = "{time}_{source}_{datatype}{_subtype}";

DataType::DataType() : myTime(Time::CurrentTime()), myReadFactory(
    "default"), myTypeName("not set")
{ }

URL
DataType::generateFileName(
  const std::string & rootFolder,
  const std::string & basepatternin)
{
  const std::string basepattern = basepatternin.empty() ? DataType::DATATYPE_PREFIX : basepatternin;
  const rapio::Time rsTime      = getTime();
  const std::string dataType    = getTypeName();
  const std::string subType     = getSubType();

  // Get absolute path in input, make sure it's in directory format
  // This might break for non-existent paths need to check
  URL forFull = URL(rootFolder + "/up");

  std::string dirbase = forFull.getDirName();

  // Allow a source in the datatype attributes to prevent data overwrite
  // stomping from multiple sources
  std::string sourcename = "";

  getString("Sourcename", sourcename);
  if (sourcename.empty()) {
    getString("radarName-value", sourcename); // Some have this
  }

  // Example: dirName/Reflectivity/00.50/TIMESTRING.netcdf

  // Should be able to keep constant
  const std::vector<std::string> tokens =
  { "{datatype}", "{/subtype}", "{_subtype}", "{subtype}", "{time}", "{source}", "{/source}" };
  const std::string sub1       = subType.empty() ? ("") : ('/' + subType);
  const std::string sub2       = subType.empty() ? ("") : ('_' + subType);
  const std::string time       = rsTime.getString(Record::RECORD_TIMESTAMP);
  const std::string source2    = sourcename.empty() ? ("") : ('/' + sourcename);
  std::vector<std::string> tos =
  { dataType, sub1, sub2, subType, time, sourcename, source2 };
  std::string p = Strings::replaceGroup(basepattern, tokens, tos);
  // dirbase is always at front of course
  URL path = URL(dirbase + "/" + p);

  return path;
} // DataType::generateFileName

void
DataType::generateRecord(
  const URL           & pathin,
  const std::string   & factory,
  std::vector<Record> & records
)
{
  std::string dirpath   = pathin.getDirName();
  std::string filepath  = pathin.toString();
  std::string sfilepath = pathin.getBaseName();

  const rapio::Time rsTime      = getTime();
  const std::string time_string = rsTime.getString(Record::RECORD_TIMESTAMP);
  const std::string dataType    = getTypeName();
  const std::string spec        = getSubType();

  // Create record params
  std::vector<std::string> params;

  // Always add the builder/factory
  params.push_back(factory);

  // MRMS breaks it up with '/' but I don't think we actually need to do that
  // I think I'll modify MRMS to handle our records as one line if needed,
  // this simplifies the design and records for future non-file destinations.
  // FIXME: We need absolute path here, or the indexlocation thing.
  params.push_back(filepath);

  // Create record selections
  std::vector<std::string> selections;

  selections.push_back(time_string);
  selections.push_back(dataType);
  if (!spec.empty()) {
    selections.push_back(spec);
  }
  Record rec(params, selections, rsTime);

  std::string source;

  if (getString("Sourcename", source)) {
    rec.setSourceName(source);
  } else if (getString("radarName-value", source)) {
    rec.setSourceName(source);
  }

  records.push_back(rec);
} // DataType::generateRecord

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
  if ((s.empty()) || (s == "file")) { // From file index
    setDataAttributeValue("SubType", getGeneratedSubtype());
  } else {
    setDataAttributeValue("SubType", s);
  }
}

bool
DataType::initFromGlobalAttributes()
{
  // These are optional at the DataType level
  // since DataGrid might be general, etc. But we can
  // use them generically if we see them.
  getString(Constants::TypeName, myTypeName);

  // This is set by object creation, so override now to handle
  // variations.  For example, making a DataGrid object set the DataType
  // to DataGrid, but attributes might have Stage2Ingest or a custom one
  getString(Constants::sDataType, myDataType);

  // -------------------------------------------------------
  // Location
  float latDegs  = 0;
  float lonDegs  = 0;
  float htMeters = 0;

  bool success = true;

  success &= getFloat(Constants::Latitude, latDegs);
  success &= getFloat(Constants::Longitude, lonDegs);
  success &= getFloat(Constants::Height, htMeters);
  if (success) {
    myLocation = LLH(latDegs, lonDegs, htMeters / 1000.0);
  }

  // -------------------------------------------------------
  // Time
  long timesecs = 0;

  if (getLong(Constants::Time, timesecs)) {
    // optional
    double f = 0.0;
    getDouble(Constants::FractionalTime, f);
    // Cast long to time_t..hummm
    myTime = Time(timesecs, f);
  }

  return true;
} // DataType::initFromGlobalAttributes

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
