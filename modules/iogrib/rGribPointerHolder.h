#pragma once

#include <rData.h>

namespace rapio {
class GribDataTypeImp;

/** Held onto for weak referencing pointers. GribMessageImp and GribFieldImp
 * need to weakly hold onto us (without a base std::shared_ptr).
 * This will work even if GribDataTypeImp is not a std::shared_ptr. */
class GribPointerHolder : public Data {
public:
  GribPointerHolder(GribDataTypeImp * h) : myDataType(h){ }

  GribDataTypeImp * myDataType;
};
}
