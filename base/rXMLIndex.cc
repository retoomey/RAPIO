#include "rXMLIndex.h"

#include "rIOIndex.h"
#include "rStrings.h"
#include "rError.h"
#include "rOS.h"
#include "rRecordQueue.h"

#include <iostream>

using namespace rapio;

XMLIndex::XMLIndex(const URL                        & xmlURL,
  const std::vector<std::shared_ptr<IndexListener> >& listeners,
  const TimeDuration                                & maximumHistory)
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
  LogSevere("Radial set single read/write test\n");
  auto radialset1 = IODataType::read<DataType>("/tmp/test.netcdf");
  IODataType::write(radialset1, "/tmp/test2.netcdf");
  exit(1);

  auto doc2 = IODataType::read<XMLData>(myURL);

  if (doc2 != nullptr) {
    const auto indexPath  = IOIndex::getIndexPath(myURL);
    const auto indexLabel = getIndexLabel();

    try{
      /* <codeindex>
       * <item>
       *   <time fractional="0.00000000"> 925767255 </time>
       *   <params>
       *   <selections>
       * </item>
       * </codeindex>
       */
      auto itemTree = doc2->getTree()->getChild("codeindex");
      auto items    = itemTree.getChildren("item");
      for (auto r: items) {
        // Note priority queue time sorts all initial indexes
        Record rec;
        // FIXME: still using boost
        if (rec.readXML(r.node, indexPath, indexLabel)) {
          Record::theRecordQueue->addRecord(rec);
        }
      }
    }catch (std::exception& e) {
      LogSevere("Error parsing codeindex XML\n");
      return (false);
    }
    return (true);
  }

  return (false);
} // XMLIndex::initialRead
