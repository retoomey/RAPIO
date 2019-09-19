#include "rXMLIndex.h"

#include "rIOIndex.h"
#include "rStrings.h"
#include "rError.h"
#include "rOS.h"
#include "rRecordQueue.h"

#include <iostream>

using namespace rapio;

XMLIndex::XMLIndex(const URL                   & xmlURL,
  std::vector<std::shared_ptr<IndexListener> > listeners,
  const TimeDuration                           & maximumHistory)
  : IndexType(listeners, maximumHistory),
  readok(false)
{
  myURL = xmlURL;
}

XMLIndex::~XMLIndex()
{ }

void
XMLIndex::introduceSelf()
{
  std::shared_ptr<IndexType> newOne = std::make_shared<XMLIndex>();
  IOIndex::introduce("xml", newOne);
}

std::shared_ptr<IndexType>
XMLIndex::createIndexType(
  const URL                                    & location,
  std::vector<std::shared_ptr<IndexListener> > listeners,
  const TimeDuration                           & maximumHistory)
{
  std::shared_ptr<XMLIndex> result = std::make_shared<XMLIndex>(location,
      listeners,
      maximumHistory);
  return (result);
}

/** Called by RAPIOAlgorithm to gather initial records from source */
bool
XMLIndex::initialRead(bool realtime)
{
  auto doc2 = IOXML::readURL(myURL);

  if (doc2 != nullptr) {
    const auto indexPath  = IOIndex::getIndexPath(myURL);
    const auto indexLabel = getIndexLabel();

    try{
      // Each child of the full ptree
      for (auto r: doc2->get_child("codeindex")) { // Can boost get first child?
        if (r.first == "item") {                   // do we need to check? Safest but slower
          // Note priority queue time sorts all initial indexes
          Record rec;
          if (rec.readXML(r.second, indexPath, indexLabel)) {
            Record::theRecordQueue->addRecord(rec);
          }
        }
      }
    }catch (std::exception e) {
      LogSevere("Error parsing codeindex XML\n");
      return (false);
    }
    return (true);
  }

  return (false);
}
