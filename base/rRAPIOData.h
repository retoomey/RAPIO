#pragma once

#include <rAlgorithm.h>
#include <rRecord.h>

#include <string>
#include <memory>

namespace rapio {
/** RAPIOData is an information record passed to subclasses of a stock
 * algorithm. By grouping,
 * it allows us to add more information later without changing the API.
 * @author Robert Toomey */
class RAPIOData : public Algorithm {
public:

  RAPIOData(const Record& aRec);

  /** Get the old style record if any associated with this data */
  Record
  record();

  /** Get datatype as shared_ptr of particular type */
  template <class T> std::shared_ptr<T>
  datatype()
  {
    if (dt == 0) {
      dt = rec.createObject(); // cache it
    }
    std::shared_ptr<T> dr = std::dynamic_pointer_cast<T>(dt);

    return (dr);
  }

  /** Return matched index number in order of command line */
  int
  matchedIndexNumber();

  /** Return a brief description of the data that can be used in application for feedback */
  std::string
  getDescription();

protected:

  /** The record of the data. */
  Record rec;

  /** The created data type from the record. */
  std::shared_ptr<DataType> dt;
};
}
