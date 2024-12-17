#include "rRecord.h"

#include "rIODataType.h"
#include "rOS.h"
#include "rXMLIndex.h"
#include "rStrings.h"
#include "rRecordQueue.h"
#include "rRecordFilter.h"

#include <cassert>
#include <algorithm>

using namespace rapio;

std::string rapio::Record::theProcessName;

std::shared_ptr<RecordFilter> rapio::Record::theRecordFilter;

std::shared_ptr<RecordQueue> rapio::Record::theRecordQueue;

std::string Record::RECORD_TIMESTAMP = "%Y%m%d-%H%M%S.%/ms";

Record::Record(const std::vector<std::string> & params,
  const std::vector<std::string>              & selects,
  const rapio::Time                           & productTime)
  : myParams(params),
  mySelections(selects),
  myTime(productTime)
{
  myIndexCount    = 0;
  mySelections[0] = myTime.getString(RECORD_TIMESTAMP);
}

namespace {
std::string
add_colon(const std::vector<std::string>& sel)
{
  std::string result = sel[1];

  result += ':';
  result += sel[2];
  return (result);
}
}

bool
Record::matches(const std::string& spec) const
{
  if ((getDataType() == spec) ||
    ((mySelections.size() > 2) && (add_colon(mySelections) == spec)))
  {
    return (true);
  }
  return (false);
}

std::ostream&
operator << (std::ostream& os, const rapio::Record& rec)
{
  rec.constructXMLString(os, "");
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

const std::string&
Record::getDataType() const
{
  if (getSelections().size() > 1) { return (this->getSelections()[1]); }

  LogDebug("Record does not have a data type set in the mySelections ... \n");
  static std::string Unknown("Unknown");

  return (Unknown);
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
  if (a.myIndexCount != b.myIndexCount) {
    return (a.myIndexCount < b.myIndexCount);
  }

  // two data records ... If you have Reflectivity:01.50 and Velocity:00.50
  // the Velocity should come first if they have the same time
  std::string a_st, b_st;

  if (a.getSelections().size() > 2) {
    a_st = a.getSelections()[2];
  }

  if (b.getSelections().size() > 2) {
    b_st = b.getSelections()[2];
  }

  if (a_st != b_st) {
    return (a_st < b_st);
  }

  const std::string& a_dt = a.getSelections()[1];
  const std::string& b_dt = b.getSelections()[1];

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

bool
Record::getSubtype(std::string& out)
{
  if (isValid()) {
    const std::vector<std::string>& temp = getSelections();

    if (temp.size() > 2) {
      out = temp.back();
    }
    return (true);
  }
  return (false);
}

void
Record::readParams(const std::string & params,
  const std::string                  & changeAttr,
  std::vector<std::string>           & v,
  const std::string                  & indexPath)
{
  // does it have a change attribute?
  if (!changeAttr.empty()) {
    // FIXME: Ancient indexes have this.  Might need to implement it
    static bool once = true;
    if (once) {
      LogSevere("Can't handle a param change index at moment.  We can implement if you need this\n");
      once = false;
    }
    return;
  }

  // Split text into vector
  Strings::split(params, &v);

  if (v.empty()) {
    LogSevere("Record has empty parameter tag: " << params << '\n');
    return;
  }

  // Replace first {IndexLocation} with index location
  auto it = std::find(v.begin(), v.end(), Constants::IndexPathReplace);

  if (it != v.end()) {
    *it = indexPath; // quicker if only one
  } else {
    // This is special logic for WebIndex to allow relative and absolute
    // paths to be sent to the w2server, but still being backward compatable.
    // Basically when reading xml for a webindex, we force even absolute paths
    // to be 'http'-ed.  This means always appending the INDEXPATH somehow to
    // the params
    if (Strings::beginsWith(indexPath, "http:")) {
      if (v[0] == "Event") { } else {
        // For each param
        for (auto& s:v) {
          // The first / becomes ABS for http
          if ((s.length() > 0) && (s[0] == '/')) {
            s = indexPath + "/{ABS}" + s;
            break;
          }
        }
      }
    }
  }
} // Record::readParams

bool
Record::readXML(const PTreeNode& item,
  const std::string            & indexPath,
  size_t                       indexLabel) // Not sure these should be passed
{
  // <item>
  // <time fractional="0.057"> 925767275 </time>
  // <params>netcdf /RADIALTEST Velocity 00.50 19990503-213435.netcdf </params>
  // <selections>19990503-213435.057 Velocity 00.50 </selections>
  // </item>

  // Time tag
  const auto time = item.getChild("time");
  // or as const auto timelong   = item.get("time", 0l);
  const auto timelong   = time.get(0l);
  const auto fractional = time.getAttr("fractional", 1.0f);

  myTime = Time::SecondsSinceEpoch(timelong, fractional);

  // Params tag
  const auto paramTag   = item.getChild("params");
  const auto params     = item.get("params", std::string("")); // direct
  const auto changeAttr = paramTag.getAttr("changes", std::string(""));

  // Might be faster just to inline here
  readParams(params, changeAttr, myParams, indexPath);

  // Selections tag
  const auto selections = item.get("selections", std::string(""));

  Strings::split(selections, &mySelections);

  if (myParams.empty() || mySelections.empty()) {
    LogSevere("Ill-formed XML record. Lacks tags for params and/or mySelections \n");
    return false;
  }

  // Make sure time matches actual time object...
  mySelections[0] = myTime.getString(RECORD_TIMESTAMP);
  myIndexCount    = indexLabel;

  // Abort record if not wanted
  if (theRecordFilter != nullptr) {
    if (!theRecordFilter->wanted(*this)) { return false; }
  }
  return true;
} // Record::readXML

void
Record::constructXMLMeta(std::ostream& ss) const
{
  // Note: <meta> is generated by caller
  // Write metadata.  This should be stuff written once per created file
  // since all of our code is running in its own process
  if (getProcessName().empty()) {
    // host + process location for tracking purposed
    const std::string& hostname = OS::getHostName();
    setProcessName("file://" + hostname + OS::getProcessName());
  }
  ss << "<process>" << getProcessName() << "</process>\n";
}

void
Record::constructXMLString(std::ostream& ss, const std::string& indexPath) const
{
  // Note: <item> is generated by caller

  // Time tag ---------------------------------------------------------
  const auto t = getTime();

  ss << " <time fractional=\"" << t.getFractional()
     << "\"> " << t.getSecondsSinceEpoch() << " </time>\n";

  // Params tag -------------------------------------------------------
  ss << " <params>";
  for (auto& p2:getParams()) {
    std::string p = p2;
    // W2 uses w2merger, not raw...HACK for moment
    //    if (p =="raw"){
    //	    p = "w2merger";
    //    }
    // Replace actual index path with a marker
    if (p == indexPath) {
      ss << Constants::IndexPathReplace << Constants::RecordXMLSeparator;
    } else {
      ss << p << Constants::RecordXMLSeparator;
    }
  }
  ss << "</params>\n";

  // Selections tag ---------------------------------------------------
  ss << " <selections>";
  for (auto& iter: getSelections()) {
    ss << iter << Constants::RecordXMLSeparator;
  }
  ss << "</selections>\n";
} // Record::constructXMLString
