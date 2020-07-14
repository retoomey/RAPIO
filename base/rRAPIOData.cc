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
