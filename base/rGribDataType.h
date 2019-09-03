#pragma once

#include <rDataType.h>
#include <rBuffer.h>
#include <rTime.h>
#include <rLLH.h>

#include <memory>

namespace rapio {
/** DataType for the data of a Netcdf file before any possible
 * delegation.  Note, on delegation the delegated data type
 * might not be a direct subclass.
 *
 * @author Robert Toomey */
class GribDataType : public DataType {
public:
  GribDataType(Buffer buf) : myBuf(buf)
  {
    myDataType = "GribData";
  }

  // FIXME: not liking this forced at this level...
  //
  /** Return Location that corresponds to this DataType */
  virtual LLH
  getLocation() const override;

  /** Return Time that corresponds to this DataType */
  virtual Time
  getTime() const override;

  // get2DData(values )
private:
  /** Store the buffer of data (copy wraps around shared_ptr) */
  Buffer myBuf;
};
}
