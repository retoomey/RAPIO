#include "rRAPIOData.h"

using namespace rapio;
using namespace std;

RAPIOData::RAPIOData(const Record& aRec)
  : rec(aRec)
{ }

Record
RAPIOData::record()
{
  return (rec);
}

int
RAPIOData::matchedIndexNumber()
{
  return (rec.getIndexNumber());
}

std::string
RAPIOData::getDescription()
{
  std::stringstream s;
  auto sel = record().getSelections();
  const size_t size = sel.size();

  for (size_t i = 0; i < size; i++) {
    s << sel[i];
    if (i != size - 1) {
      s << " ";
    }
  }
  s << " from index " << matchedIndexNumber();
  return s.str();
}
