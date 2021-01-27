#include "rXMLIndex.h"

#include "rIOIndex.h"
#include "rStrings.h"
#include "rError.h"
#include "rOS.h"
#include "rIODataType.h"
#include "rRecordQueue.h"
#include "rCompression.h"

#include <iostream>

using namespace rapio;

/** Default constant for a static XML index */
const std::string XMLIndex::XMLINDEX = "ixml";

XMLIndex::XMLIndex(const URL & xmlURL,
  const TimeDuration         & maximumHistory)
  : IndexType(maximumHistory)
{
  myURL = xmlURL;
}

XMLIndex::~XMLIndex()
{ }

std::string
XMLIndex::getHelpString(const std::string& fkey)
{
  return
    "Reads a collection of fml records from a static file such as code_index.xml.\n  Example: /myarchive/code_index.xml";
}

bool
XMLIndex::canHandle(const URL& url, std::string& protocol, std::string& indexparams)
{
  // We'll claim any missing protocal with a .fam ending for legacy support
  if (protocol.empty()) {
    std::string suffix = url.getSuffixLC();
    // Anything with .xml we'll try to take...
    if (suffix == "xml") {
      protocol = XMLIndex::XMLINDEX;
      return true;
      // ...or .gz etc with .xml
    } else if (Compression::suffixRecognized(suffix)) {
      URL tmp(url);
      tmp.removeSuffix(); // removes the suffix
      std::string p = tmp.getPath();
      p.resize(p.size() - 1); /// removes the dot
      tmp.setPath(p);
      suffix = tmp.getSuffixLC();

      if (suffix == "xml") {
        protocol = XMLIndex::XMLINDEX;
        return true;
      }
    }
  }
  return false;
}

void
XMLIndex::introduceSelf()
{
  std::shared_ptr<IndexType> newOne = std::make_shared<XMLIndex>();
  IOIndex::introduce(XMLINDEX, newOne);
}

std::shared_ptr<IndexType>
XMLIndex::createIndexType(
  const std::string  & protocol,
  const std::string  & indexparams,
  const TimeDuration & maximumHistory)
{
  std::shared_ptr<XMLIndex> result = std::make_shared<XMLIndex>(URL(indexparams),
      maximumHistory);
  return (result);
}

/** Called by RAPIOAlgorithm to gather initial records from source */
bool
XMLIndex::initialRead(bool realtime, bool archive)
{
  auto doc2 = IODataType::read<PTreeData>(myURL.toString());

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
        if (rec.readXML(r, indexPath, indexLabel)) {
          Record::theRecordQueue->addRecord(rec);
        }
      }
    }catch (const std::exception& e) {
      LogSevere("Error parsing codeindex XML\n");
      return (false);
    }
    return (true);
  }

  return (false);
} // XMLIndex::initialRead
