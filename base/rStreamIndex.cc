#include "rStreamIndex.h"

#include "rIOIndex.h"
#include "rError.h"
#include "rIOURL.h"
#include "rOS.h"
#include "rRecordQueue.h"
#include "rStrings.h"
#include "rEXEWatcher.h"
#include "rIODataType.h"

#include <algorithm>

using namespace rapio;

/** Default constant for an exe index */
const std::string StreamIndex::STREAMINDEX = "exe";

StreamIndex::~StreamIndex()
{ }

StreamIndex::StreamIndex(
  const std::string  & protocol,
  const std::string  & indexparams,
  const TimeDuration & maximumHistory) :
  IndexType(maximumHistory), myItemStart("<item>"), myItemEnd("</item>")
{
  // Using % to space separate exe, quotes and other stuff mess with the shell
  myParams   = indexparams;
  myDFAState = 0;
}

std::string
StreamIndex::getHelpString(const std::string& fkey)
{
  return
    "Stream index watches a program, arguments separated by %.\n  Example: exe='feedme%-f%TXT' for ldm ingest of fml.";
}

bool
StreamIndex::initialRead(bool realtime, bool archive)
{
  // ---------------------------------------------------------
  // Archive
  //
  // Grab the list of files currently in the directory

  if (archive) {
    // If we trust the external process to be a 'one-shot', we could
    // implement something here.  What if it doesn't die?
    LogSevere("Stream index has no archive ability, use with -r realtime.\n");
    return false;
  }

  // ---------------------------------------------------------
  // Realtime
  //
  if (realtime) {
    std::shared_ptr<WatcherType> watcher = IOWatcher::getIOWatcher("exe");
    bool ok = watcher->attach(myParams, this);
    if (!ok) { return false; }
  }

  return true;
} // StreamIndex::initialRead

void
StreamIndex::handleNewEvent(WatchEvent * w)
{
  // Skip checking type, any buffer we'll use
  // FIXME: Later might need type if we connect to multiple
  // event types
  const size_t bytes_read = w->myBuffer.size();

  if (bytes_read > 0) {
    const std::vector<char>& b = w->myBuffer;

    /* Test dumping lines
     *
     *  const char DELIMITER = '\n';
     *  const size_t MAXLINESIZE = 8;
     *
     *  for(size_t i=0; i< bytes_read; i++){
     *    myLineCount.push_back(b[i]);
     *
     *      bool dumpline = false;
     *      if (b[i] == DELIMITER){
     *        dumpline = true;
     *      }else{
     *        if (myLineCout.size() >= MAXLINESIZE){
     *          dumpline = true;
     *          myLineCout.push_back(DELIMITER);
     *        }
     *      }
     *
     *      if (dumpline){
     *         LogInfo("WRITE: " << myLineCout));
     *         ...
     *      }
     *    }
     */

    // only work for one now

    // DFA style parsing the stream for <item> </item> groups...
    for (size_t i = 0; i < bytes_read; i++) {
      const char& c = b[i]; // lower case it?  only for tags though

      // This seems dense..lol...easier way?
      switch (myDFAState) {
          case 0: // Continually looking for start tag
            if (myItemStart.parse(c)) {
              myDFAState = 1;
              // myLineCout << "<item>";
              std::string t = "<item>";
              for (size_t z = 0; z < t.size(); z++) {
                myLineCout.push_back(t[z]);
              }
            }
            break;
          case 1:                    // Looking for end tag
            myLineCout.push_back(c); // gather until tag matched
            // FIXME: a max string size in case of bad data stream
            // FIXME: currently a bad stream could fill up memory if we
            // get a start tag and never get an end tag
            if (myItemEnd.parse(c)) {
              myDFAState = 0;
              // Here we parse the XML record, create a new record for record queue
              // for(size_t z=0; z<myLineCout.size(); z++){
              //   std::cout << myLineCout[z];
              // }
              // std::cout << "\n";

              std::shared_ptr<PTreeData> xml = std::make_shared<PTreeData>();
              try{
                if (IODataType::readBuffer<PTreeData>(myLineCout, "xml")) {
                  Record rec;
                  auto tree = xml->getTree();
                  auto item = tree->getChild("item");
                  if (rec.readXML(item, "", getIndexLabel())) {
                    Record::theRecordQueue->addRecord(rec);
                  }
                } else {
                  LogSevere("Failed record XML from stream, can't parse.\n");
                }
              }catch (const std::exception& e) {
                LogSevere("Failed record XML from stream, record invalid.\n");
              }
              myLineCout.clear();
            }
            break;
          default:
            break;
      }
    }
  }
} // StreamIndex::handleNewEvent

void
StreamIndex::introduceSelf()
{
  // Handle a stream index
  std::shared_ptr<IndexType> newOne = std::make_shared<StreamIndex>();
  IOIndex::introduce(STREAMINDEX, newOne);
}

std::shared_ptr<IndexType>
StreamIndex::createIndexType(
  const std::string  & protocol,
  const std::string  & indexparams,
  const TimeDuration & maximumHistory)
{
  std::shared_ptr<StreamIndex> result = std::make_shared<StreamIndex>(
    protocol,
    indexparams,
    maximumHistory);
  return (result); // Factory handles isValid now...
}
