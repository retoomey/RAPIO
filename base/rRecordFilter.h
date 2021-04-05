#pragma once

#include <rData.h>
#include <rRecord.h>

namespace rapio {
class RAPIOAlgorithm;

/** A filter of records, the default return all records */
class RecordFilter : public Data
{
public:
  virtual bool
  wanted(const Record& rec){ return true; }
};

/** A filter of records using the -I option of RAPIOAlgorithm */
class AlgRecordFilter : public RecordFilter
{
public:
  AlgRecordFilter(RAPIOAlgorithm * a) : myAlg(a){ };

  virtual bool
  wanted(const Record& rec) override;

protected:
  RAPIOAlgorithm * myAlg;
};
}
