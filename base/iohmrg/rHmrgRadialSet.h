#pragma once

#include "rIODataType.h"
#include "rIOHmrg.h"

#include <zlib.h>

namespace rapio {
/**
 * Read/Write HMRG Polar binary file
 *
 */
class IOHmrgRadialSet : public IOHmrg {
public:

  /** Do the heavy work of creating a RadialSet */
  static std::shared_ptr<DataType>
  readRadialSet(gzFile fp, const std::string& radarName, bool debug = false);
};
}
