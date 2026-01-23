#include "rConfigRecord.h"

#include "rStrings.h"
#include "rRecord.h"
#include "rRecordFilter.h"
#include "rOS.h"
#include "rError.h"

using namespace rapio;

void
ConfigRecord::readParams(const std::string & params,
  const std::string                        & changeAttr,
  std::vector<std::string>                 & v,
  const std::string                        & indexPath)
{
  // does it have a change attribute?
  if (!changeAttr.empty()) {
    // FIXME: Ancient indexes have this.  Might need to implement it
    static bool once = true;
    if (once) {
      fLogSevere("Can't handle a param change index at moment.  We can implement if you need this");
      once = false;
    }
    return;
  }

  // Split text into vector
  Strings::split(params, &v);

  if (v.empty()) {
    fLogSevere("Record has empty parameter tag: {}", params);
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
} // ConfigRecord::readParams

bool
ConfigRecord::readXML(Record& rec, const PTreeNode& item,
  const std::string            & indexPath,
  size_t indexLabel) // Not sure these should be passed
{
  // FIXME: We could break this down into helper methods
  // and be cleaner.
  try{
    long timelong;
    float fractional = 0;
    std::vector<std::string> theParams;
    std::vector<std::string> theSelections;
    bool haveParams = false;

    // Existance of 't' in <item> means new format
    const auto fulltime  = item.getAttr("t", std::string(""));
    const bool newFormat = !fulltime.empty();

    // Messages and records now support key/value tags
    // <p n="Key">Value</p>
    auto values = item.getChildren("v");
    for (auto v:values) {
      auto a = v.getAttr("n", std::string(""));
      auto t = v.get(std::string(""));
      if (!a.empty()) {
        rec.setValue(a, t);
      }
    }

    if (newFormat) {
      // New format Files we 'inline' the p and s to save space.  The short
      // letters reduce some with the billions of records we write.
      // 'item' is legacy so reducing that may be more trouble.
      //
      // <item t="925776886.46" p="netcdf /DATA/KTLX/Reflectivity/05.25/19990504-001446.460.netcdf.gz"
      //       s="19990504-001446.460 Reflectivity 05.25" />
      // <item t="925767266.500000">Message
      //   <p n="Color">Red</p>
      //   <p n="Other">Stuff</p>
      // </item>

      // Time as t= in item
      std::vector<std::string> s;
      Strings::splitWithoutEnds(fulltime, '.', &s);
      if (s.size() > 0) {
        timelong = std::stol(s[0]);
      }
      if (s.size() > 1) {
        fractional = std::stof("." + s[1]);
      }

      // Get the message main text 'Message' from the item
      auto t = item.get(std::string(""));
      rec.setValue("MessageText", t);

      // Params as p= in item.  A missing p means it's a
      // message
      const auto pattr = item.getAttr("p", std::string(""));
      haveParams = !pattr.empty();

      if (haveParams) {
        ConfigRecord::readParams(pattr, "", theParams, indexPath);

        // Selections as s= in item
        const auto sattr = item.getAttr("s", std::string(""));
        Strings::split(sattr, &theSelections);
      }
    } else {
      // Legacy format:
      // <item>
      // <time fractional="0.057"> 925767275 </time>
      // <params>netcdf /RADIALTEST Velocity 00.50 19990503-213435.netcdf </params>
      // <selections>19990503-213435.057 Velocity 00.50 </selections>
      // </item>

      // We also have this fun stuff.  It's an older attempt at messaging.
      // We'll check for 'Event' and convert to our parameter list.
      // <item>
      // <time fractional="0.358000"> 1747425326 </time>
      // <params>Event 3530 </params>
      // <selections>20250516-195526.358 NewVolume </selections>
      // </item>

      // Time as <time>
      const auto time = item.getChild("time");
      timelong   = time.get(0l);
      fractional = time.getAttr("fractional", 1.0f);

      // Params as <params> and selections as <selections>
      haveParams = true;
      const auto paramTag   = item.getChild("params");
      const auto p          = item.get("params", std::string("")); // direct
      const auto changeAttr = paramTag.getAttr("changes", std::string(""));
      ConfigRecord::readParams(p, changeAttr, theParams, indexPath);
      const auto selections = item.get("selections", std::string(""));
      Strings::split(selections, &theSelections);

      // Special check Lak's message format, convert the event number and
      // text to our message.  Note, messages don't have selections
      // and have empty params.
      if ((theParams.size() > 0) && (theParams[0] == "Event")) {
        // Event 12345.  Store as Count
        if (theParams.size() > 1) {
          rec.setValue("Count", theParams[1]);
        }

        // Selections: timestamp 'message', where message is say NewVolume
        if (theSelections.size() > 1) {
          rec.setValue("MessageText", theSelections[1]);
        }

        theSelections.clear(); // Don't need these and might break
        theParams.clear();
        haveParams = false;
      }
    }

    // Finalize the record
    rec.setIndexNumber(indexLabel);
    rec.setTime(Time::SecondsSinceEpoch(timelong, fractional));
    if (haveParams) {
      rec.setParams(theParams);
      rec.setSelections(theSelections);

      // FIXME: We could check later, but this allows us to keep
      // the queue smaller.
      // FIXME: Do we need to filter messages?  For now we won't.
      // Records representing data are filtered. Messages aren't
      if (Record::theRecordFilter != nullptr) {
        if (!Record::theRecordFilter->wanted(rec)) {
          return false;
        }
      }
    }
    return true;
  }catch (const std::exception& e) {
    fLogSevere("Parse error on xml of record: ", rec);
  }
  return false;
} // Record::readXML

void
ConfigRecord::constructXMLString(const Record& rec, std::ostream& ss, const std::string& indexPath)
{
  // FIXME: Write records as new format later.  Issue is MRMS will
  // have to be able to read them.  So for now, keeping old way, but
  // adding the message params.


  // Time tag ---------------------------------------------------------
  const auto t = rec.getTime();

  ss << " <time fractional=\"" << t.getFractional()
     << "\"> " << t.getSecondsSinceEpoch() << " </time>\n";

  // Write params/selections only if data record
  if (rec.isData()) {
    // Params tag -------------------------------------------------------
    ss << " <params>";
    for (auto& p2:rec.getParams()) {
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
    for (auto& iter: rec.getSelections()) {
      ss << iter << Constants::RecordXMLSeparator;
    }
    ss << "</selections>\n";
  }

  // Write value pairs if message
  if (rec.isMessage()) {
    auto k = rec.getKeys();
    auto v = rec.getValues();
    // FIXME: character filter output
    for (size_t i = 0; i < k.size(); ++i) {
      ss << " <v n=\"" << k[i] << "\">" << v[i] << "</v>\n";
    }
  }
} // ConfigRecord::constructXMLString

void
ConfigRecord::constructXMLMeta(const Record& rec, std::ostream& ss)
{
  // Note: <meta> is generated by caller
  // Write metadata.  This should be stuff written once per created file
  // since all of our code is running in its own process
  if (rec.getProcessName().empty()) {
    // host + process location for tracking purposed
    const std::string& hostname = OS::getHostName();
    rec.setProcessName("file://" + hostname + OS::getProcessName());
  }
  ss << "<process>" << rec.getProcessName() << "</process>\n";
} // ConfigRecord::constructXMLString
