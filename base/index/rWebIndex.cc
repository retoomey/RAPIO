#include "rWebIndex.h"

#include "rError.h"
#include "rStrings.h"
#include "rXMLIndex.h"
#include "rRecordQueue.h"
#include "rIODataType.h"
#include "rWebIndexWatcher.h"
#include "rConfigRecord.h"

#include <iostream>
#include <algorithm>

using namespace rapio;

/** Default constant for a web polling index */
const std::string WebIndex::WEBINDEX = "iweb";

void
WebIndex::introduceSelf()
{
  std::shared_ptr<IndexType> newOne = std::make_shared<WebIndex>();

  IOIndex::introduce(WEBINDEX, newOne);
}

WebIndex::WebIndex(const URL & aURL,
  const TimeDuration         & maximumHistory) :
  IndexType(maximumHistory),
  myLastRead(0),
  myLastReadNS(0),
  myURL(aURL),
  indexDataPath(aURL)
{
  // The user needs to give us http://venus:8080/?source=KABR
  // We then add servlet relative path on the remote server
  // This keeps the datapath correct (what the user provided)
  // fLogInfo("The provided path was: {}", myURL.path);
  myURL.setPath(myURL.getPath() + "/webindex/getxml.do");

  // fLogInfo("I've changed it to {}", myURL.path);
  if (!myURL.hasQuery("source")) {
    fLogSevere("Missing 'source' in URL.");
    return;
  }

  // the data comes via static links from the same server, but we need to append
  // the source
  std::string source = indexDataPath.getQuery("source");

  indexDataPath.setPath(indexDataPath.getPath() + '/' + source);
  indexDataPath.clearQuery();
}

WebIndex::~WebIndex()
{ }

std::string
WebIndex::getHelpString(const std::string& fkey)
{
  return
    "Web index polls a NSSL/WG data webserver for metadata.\n  Example: iweb=//vmrms-sr20/KTLX or http://vmrms-webserv/vmrms-sr02?source=KTLX (non macro)";
}

bool
WebIndex::canHandle(const URL& url, std::string& protocol, std::string& indexparams)
{
  if (protocol.empty() || (protocol == WEBINDEX)) {
    // If there's a source tag, it's an old web index...
    // -i "iweb=http://vmrms-webserv/vmrms-sr02?source=KTLX"
    if (!url.getQuery("source").empty()) {
      protocol = WebIndex::WEBINDEX;
      return true;
    }

    // ...else we macro machine names to our nssl vmrms data sources like so...
    // "iweb=//vmrms-sr02/KTLX" --> http://vmrms-webserv/vmrms-sr02?source=KTLX
    // "//vmrms-sr02/KTLX" --> http://vmrms-webserv/vmrms-sr02?source=KTLX

    // If it's a null host, look for a macro conversion
    if (url.getHost() == "") {
      std::string p = url.getPath();

      if (p.size() > 1) {
        if ((p[0] == '/') && (p[1] == '/')) {
          p = p.substr(2);
          std::vector<std::string> pieces;
          Strings::splitWithoutEnds(p, '/', &pieces);

          if (pieces.size() > 1) {
            std::string machine  = pieces[0];
            std::string source   = pieces[1];
            std::string expanded = "http://vmrms-webserv/" + machine
              + "?source=" + source;
            fLogInfo("Web macro source sent to {}", expanded);
            //   u = URL(expanded);
            indexparams = expanded; // Change to non-macroed format
          }
          protocol = WebIndex::WEBINDEX;
          return true;
        }
      }
    }
  }
  return false;
} // WebIndex::canHandle

bool
WebIndex::initialRead(bool realtime, bool archive)
{
  // Realtime starts from NOW, so don't real the entire archive.
  if (realtime) {
    // Only read the latest record at start, then future polls will read newer
    // records
    myLastRead   = -2;
    myLastReadNS = 0;

    // The pulse poll that updates for realtime. Not needed for archive mode
    std::shared_ptr<WatcherType> watcher = IOWatcher::getIOWatcher(WebIndexWatcher::WEB_WATCH);

    bool ok = watcher->attach(myURL.getPath(), realtime, archive, this);
    if (!ok) { return false; }
  }

  // This will read the full archive or just the latest record depending
  // on last read settings
  return (readRemoteRecords());
}

bool
WebIndex::handlePoll()
{
  return (readRemoteRecords());
}

bool
WebIndex::readRemoteRecords()
{
  bool myFoundNew = false;

  if (myURL.empty()) { // Don't crash on a bad config just inform
    fLogDebug("Empty URL for web index, nothing to read or poll");
    return (false);
  }

  // ----------------------------------------------------------------------
  // Build a URL that has the right lastRead query.
  URL tmpURL(myURL);
  char str[64], str2[64];

  std::snprintf(str, sizeof(str), "%lld", myLastRead);
  tmpURL.setQuery("lastRead", str);
  std::snprintf(str2, sizeof(str2), "%ld", myLastReadNS);
  tmpURL.setQuery("lastReadNS", str2);

  // ----------------------------------------------------------------------
  // Read the URL and parse the results.
  // the result should be an XML-formatted ItemList like what's passed into
  // XMLIndex's ctor, plus a new "lastRead" attribute in the toplevel.
  // The lastRead needs to be kept for next time to give the server a
  // point of reference of what "new" means for us.
  fLogInfo("Web:{}", tmpURL.toString());
  auto doc = IODataType::read<PTreeData>(tmpURL.toString(), "xml");

  const bool myReadOK = (doc != nullptr);

  if (myReadOK) {
    auto tree    = doc->getTree();
    auto rectest = tree->getChildOptional("records");
    if (rectest == nullptr) {
      fLogSevere("Couldn't read records tag from webindex xml return");
      return false;
    }
    const long long lastRead = rectest->getAttr("lastRead", (long long) (-1));
    const long lastReadNS    = rectest->getAttr("lastReadNS", (long) (0));

    if (lastRead >= 0) {
      // W2Server returns AT the time or greater...so when no new data comes
      // in within the poll period, we'd get a duplicate.  Bleh, but there
      // unfortunately are cases where more files came in with the same
      // time and nanotime.  So it's possible to 'skip' newer products that
      // match the old time/nanotime.  Which "shouldn't" happen, but drive
      // writing is what it is.
      myFoundNew = false;

      if (lastRead > myLastRead) { // Some number
        myFoundNew = true;
      } else if (lastRead == myLastRead) {
        // > Only strictly 'new' timestamp files (possibly skip some new ones)
        // vs >= which will get a duplicate always if new data didn't come in.
        // I'm going with > here hoping files aren't skipped.
        if (lastReadNS > myLastReadNS) {
          myFoundNew = true;
        }
      } else { // files are all older than our last read..
      }

      if (myFoundNew) {
        const std::string indexPath = indexDataPath.toString();
        size_t count = 0;
        auto recs    = rectest->getChildren("item");
        for (auto r: recs) { // Can boost get first child?
          // Note priority queue time sorts all initial indexes
          Record rec;
          if (ConfigRecord::readXML(rec, r, indexPath, getIndexLabel())) {
            Record::theRecordQueue->addRecord(rec);
          }
          count++;
        }

        fLogDebug("Polled: {} New:({})", tmpURL.toString(), count);
      } else {
        fLogDebug("Polled: {}", tmpURL.toString());
      }
      myLastRead   = lastRead;
      myLastReadNS = lastReadNS;
    } else if (lastRead == -1) {
      // no new messages - do nothing
    } else if (lastRead == -2) {
      // End of messages
    } else {
      fLogSevere("Unhandled lastRead={}", lastRead);
    }
  }

  return (myFoundNew);
} // WebIndex::readRemoteRecords

std::shared_ptr<IndexType>
WebIndex::createIndexType(
  const std::string  & protocol,
  const std::string  & indexparams,
  const TimeDuration & maximumHistory)
{
  std::shared_ptr<WebIndex> result = std::make_shared<WebIndex>(URL(indexparams),
      maximumHistory);

  return (result);
}
