#include "rIndexType.h"

#include "rRecord.h"

using namespace rapio;

// const std::string     IndexType::selectionSeparator(" ");

IndexType::IndexType()
{ }

IndexType::~IndexType()
{ }

IndexType::IndexType(std::vector<std::shared_ptr<IndexListener> > listeners,
  const TimeDuration                                              & cutoffInterval)
  : myAgeOffInterval(cutoffInterval)
{
  // Add initial listeners.  They will be notified on an addRecord
  for (auto& i:listeners) {
    this->addIndexListener(i);
  }
}

void
IndexType::addIndexListener(std::shared_ptr<IndexListener> l)
{
  myListeners.push_back(l);
}

size_t
IndexType::getNumListeners()
{
  return (myListeners.size());
}

void
IndexType::processRecord(const Record& item)
{
  // Notification dispatch to virtual functions
  if (item.getSelections().back() == "EndDataset") {
    notifyEndDatasetEvent(item);
  } else {
    notifyNewRecordEvent(item);
  }
}

void
IndexType::notifyNewRecordEvent(const Record& item)
{
  for (auto& l:myListeners) {
    l->notifyNewRecordEvent(item);
  }
}

void
IndexType::notifyEndDatasetEvent(const Record& item)
{
  for (auto& l:myListeners) {
    l->notifyEndDatasetEvent(item);
  }
}

std::string
IndexType::formKey(const std::vector<std::string>& sel,
  size_t                                         begin,
  size_t                                         end) const
{
  // This is actually horrible due to creation of strings
  // We need a smarter database
  end   = std::min(end, sel.size());
  begin = std::min(begin, end);
  std::string s;
  s.reserve(512);

  for (size_t i = begin; i < end; ++i) {
    s += Constants::RecordXMLSeparator;
    s += sel[i];
  }
  return (s);
}
