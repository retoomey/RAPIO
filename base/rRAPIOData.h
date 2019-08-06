#pragma once

#include <rAlgorithm.h>
#include <rRecord.h>

#include <string>
#include <memory>

namespace rapio {
/** RAPIOData is an information record passed to subclasses of a stock
 * algorithm. By grouping,
 * it allows us to add more information later without changing the API. */
class RAPIOData : public Algorithm {
public:

  RAPIOData(const Record& aRec, int aIndexNumber);

  /** Get the old style record if any associated with this data */
  Record
  record();

  /** Get datatype as smart ptr of particular type */
  template <class T> std::shared_ptr<T>
  datatype()
  {
    if (dt == 0) {
      dt = rec.createObject(); // cache it
    }
    return (dt);
  }

  int
  matchedIndexNumber();

protected:

  /** The record of the data. */
  Record rec;

  /** The created data type from the record. */
  std::shared_ptr<DataType> dt;

  /** The index number.  Typically from the -i entry. */
  int indexNumber;
};
}
