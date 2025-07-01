#pragma once

#include <rGribDataType.h>

namespace rapio {
class WgribMessageImp : public GribMessage {
public:

  /** Create message with grib2 information */
  WgribMessageImp(const URL& url, int messageNumber, int numFields, long int filepos, std::array<long, 3>& sec0,
    std::array<long, 13>& sec1);

  /** Get a new field from us. */
  virtual std::shared_ptr<GribField>
  getField(size_t fieldNumber) override;

protected:

  /** Store the URL passed to use for filename */
  URL myURL;
};
}
