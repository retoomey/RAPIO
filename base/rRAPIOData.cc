#include "rRAPIOData.h"

using namespace rapio;
using namespace std;

RAPIOData::RAPIOData(const Record& aRec, int aIndexNumber)
  : rec(aRec),
  indexNumber(aIndexNumber)
{ }

Record
RAPIOData::record()
{
  return (rec);
}

int
RAPIOData::matchedIndexNumber()
{
  return (indexNumber);
}
