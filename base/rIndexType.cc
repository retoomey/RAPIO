#include "rIndexType.h"

#include "rRecord.h"

using namespace rapio;

// const std::string     IndexType::selectionSeparator(" ");

IndexType::IndexType()
{ }

IndexType::~IndexType()
{ }

IndexType::IndexType(
  const TimeDuration & cutoffInterval)
  : myAgeOffInterval(cutoffInterval)
{ }

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
