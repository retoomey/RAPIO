#include "rRecord.h"

#include "rIODataType.h"
#include "rOS.h"
#include "rXMLIndex.h"
#include "rStrings.h"
#include "rRecordQueue.h"
#include "rRecordFilter.h"
#include "rConfigRecord.h"

#include <cassert>
#include <algorithm>

using namespace rapio;

static_assert(!std::is_polymorphic<Record>::value, "Record must not be polymorphic!");

std::string rapio::Message::theProcessName;

std::shared_ptr<RecordFilter> rapio::Record::theRecordFilter;

std::shared_ptr<RecordQueue> rapio::Record::theRecordQueue;

std::string Message::RECORD_TIMESTAMP = "%Y%m%d-%H%M%S.%/ms";

std::string
Record::getIDString() const
{
  // Start with time.
  std::stringstream id;

  id << getTimeString();

  // If data record, use the selection fields
  if (isData()) {
    const std::vector<std::string>& s = getSelections();
    for (size_t i = 1; i < s.size(); ++i) {
      id << '_' << s[i];
    }
  }

  // Add sourcename to avoid data stomping with multi-radar output
  const std::string sourceName = getSourceName();

  if (!sourceName.empty()) {
    id << '_' << sourceName;
  }

  if (isData()) {
    // Data source type
    const std::string dataSourceType = getDataSourceType();

    id << '_' << dataSourceType;
  }

  return id.str();
}

Record::Record()
{
  // Typically called before readXML
  // FIXME: readXML could return a record and have a valid function
  setIndexNumber(0);
  myDataType = "Unknown";
  mySubType  = "";
}

Record::Record(const std::vector<std::string> & params,
  const std::string                           & builder,
  const rapio::Time                           & productTime,
  const std::string                           & dataType,
  const std::string                           & subType)
  : myParams(params)
{
  setTime(productTime);
  setIndexNumber(0);
  myBuilder  = builder;
  myDataType = dataType;
  mySubType  = subType;

  // For moment make sure selections matches.
  // mySelections.push_back(myTime.getString(RECORD_TIMESTAMP));
  // mySelections.push_back(dataType);
  // mySelections.push_back(subType);
}

std::ostream&
rapio::operator << (std::ostream& os, const Record& rec)
{
  ConfigRecord::constructXMLString(rec, os, "");
  return (os);
}

std::string
Record::getDataSourceType() const
{
  if (myParams.size() > 0) {
    return myParams[0];
  }
  return "NONE";
}

namespace rapio {
bool
operator < (const Record& a, const Record& b)
{
  // Use the product time to decide if they belong to different times
  if (a.myTime != b.myTime) {
    return (a.myTime < b.myTime);
  }

  // Use index count iff sorting larger collections of records...
  if (a.myIndexNumber != b.myIndexNumber) {
    return (a.myIndexNumber < b.myIndexNumber);
  }

  // two data records ... If you have Reflectivity:01.50 and Velocity:00.50
  // the Velocity should come first if they have the same time
  auto a_st = a.getSubType();
  auto b_st = b.getSubType();

  if (a_st != b_st) {
    return (a_st < b_st);
  }

  // Compare data types
  const std::string& a_dt = a.getDataType();
  const std::string& b_dt = b.getDataType();

  return (a_dt < b_dt);
} // <
}

std::string
Record::getFileName() const
{
  // If only builder return nothing...
  auto size = myParams.size();

  if (size < 1) {
    return "";
  }

  // ...otherwise append the strings
  std::stringstream ss;

  bool first = true;

  for (size_t i = 1; i < size; ++i) {
    auto& s = myParams[i];
    // Some WDSSII xml indexes have a GzippedFile/xmldata randomly stuffed into params,
    // we don't want this to be part of the path.
    // Netcdf format looks like:
    // netcdf {indexlocation} Velocity 00.50 19990504-213435.netcdf.gz
    // but here we have:
    // W2ALGS GzippedFile {indexlocation} xmldata restofpath.xml.gz
    if (s == "GzippedFile") { continue; }
    if (s == "xmldata") { continue; }
    if (first) {
      ss << s; // http for example shouldn't have a first slash
      first = false;
      continue;
    }
    ss << "/" << s;
  }

  return ss.str();
} // Record::getFileName

std::shared_ptr<DataType>
Record::createObject() const
{
  auto& p(getParams());

  if (p.size() < 1) {
    LogSevere("Empty record parameters, can't create anything!\n");
    return nullptr;
  }

  // All our builders mostly use a compressed string representing a URL or file name
  // FIXME: Pass record maybe or vector, let builder determine param use.
  const std::string factoryparams = getFileName();
  std::shared_ptr<DataType> dt    = IODataType::readDataType(factoryparams, p[0]);

  return (dt);
} // Record::createObject
