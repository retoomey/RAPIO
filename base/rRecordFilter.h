#pragma once

#include <rRecord.h>

namespace rapio {
/** A filter of records, the default return all records */
class RecordFilter
{
public:
  virtual bool
  wanted(const Record& rec){ return true; }
};

/** A filter of records using a ConfigParamGroupI */
class AlgRecordFilter : public RecordFilter
{
public:
  AlgRecordFilter(){ };

  virtual bool
  wanted(const Record& rec) override;
};
}
