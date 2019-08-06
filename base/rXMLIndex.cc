#include "rXMLIndex.h"

#include "rIOIndex.h"
#include "rStrings.h"
#include "rError.h"
#include "rOS.h"
#include "rRecordQueue.h"

#include "rConfig.h"
#include "rConfigDirectoryMapping.h"

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
  std::shared_ptr<IndexType> newOne(new XMLIndex());
  IOIndex::introduce("xml", newOne);
}

std::shared_ptr<IndexType>
XMLIndex::createIndexType(
  const URL                                    & location,
  std::vector<std::shared_ptr<IndexListener> > listeners,
  const TimeDuration                           & maximumHistory)
{
  std::shared_ptr<XMLIndex> result(new XMLIndex(location,
    listeners,
    maximumHistory));
  return (result);
}

/*
 * XMLIndex::extractRecords(const XMLElement & item,
 *                       const std::string  & indexPath,
 *                       std::vector<Record>& records,
 *                       size_t indexLabel)
 * {
 * if (item.getTagName() == "item") {
 *  const size_t at = records.size();
 *  records.resize(at+1);
 *  if (!records[at].readXML(item, indexPath, indexLabel)){
 *    records.pop_back();
 *  }
 * }
 * }
 */


void
XMLIndex::handleElements(
  const std::string   & indexPath,
  const XMLElementList& elements,
  // std::vector<Record>   & out, // FIXME: for subclass we'd need pointers...
  size_t              indexLabel)
{
  // We're adding to the passed in out vector
  //  const size_t startSize = out.size();
  //  out.reserve(startSize + elements.size());

  // Grab all the records...
  for (auto i:elements) {
    //  extractRecords(*i, indexPath, out, indexLabel);
    //
    if (i->getTagName() == "item") {
      /*
       *  const size_t at = out.size();
       *  out.resize(at+1);
       *  if (!out[at].readXML(*i, indexPath, indexLabel)){
       *    out.pop_back();
       *  }
       */
      Record rec;
      if (rec.readXML(*i, indexPath, indexLabel)) {
        Record::theRecordQueue->addRecord(rec);
      }
    }
  }
}

/** Called by RAPIOAlgorithm to gather initial records from source */
bool
XMLIndex::initialRead(bool realtime)
{
  // FIXME: realtime has no meaning to xml archive...do we turn
  // off reading in realtime mode?  We need _more_ modes.

  // For an XML file, currently already read all records on 'start'
  // Don't sort or add..RAPIOAlgorithm will handle it
  // Preread can get pretty big.  Pulling by item could be nice
  std::shared_ptr<XMLDocument> doc(IOXML::readXMLDocument(myURL));

  if (doc != nullptr) {
    // add all the immediate children of the top-level node.
    // These correspond to the items in the XML document.
    const XMLElement& docElem      = doc->getRootElement();
    const XMLElementList& elements = docElem.getChildren();
    const std::string indexPath    = IOIndex::getIndexPath(myURL);

    // handleElements(indexPath, elements, out, getIndexLabel());
    handleElements(indexPath, elements, getIndexLabel());
    readok = true;
  }

  return (readok);
}
