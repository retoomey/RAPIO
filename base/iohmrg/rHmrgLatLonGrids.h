#pragma once

#include "rIODataType.h"
#include "rIOHmrg.h"

#include <zlib.h>

namespace rapio {
/**
 * Read/Write HMRG Gridded binary file
 *
 */
class IOHmrgLatLonGrids : public IOHmrg {
public:

  static std::shared_ptr<DataType>
  readLatLonGrids(gzFile fp, const int year, bool debug = false);
};
}
