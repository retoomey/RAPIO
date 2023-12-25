#pragma once

#include <rTime.h>
#include <rGribDataType.h>

#include <vector>

namespace rapio {
class GribMessageImp : public GribMessage {
public:

  /** Create uninitialized message */
  GribMessageImp(){ };

  /** Create a GribMessage with no buffer.  Grib2 data will be stored outside us.
   * Note if the myBuf in GribDataTypeImp is resized or destroyed our pointer
   * is this case will become invalid. Used when we preread entire grib2 file. */
  GribMessageImp(unsigned char * bufferptr)
  {
    myBufferPtr = bufferptr; // Store external reference
  }

  /** Create a GribMessage with a buffer sized to accept a Grib2 data field,
   * used when we read per field off disk. */
  GribMessageImp(size_t bufferSize) : myLocalBuffer(bufferSize)
  {
    myBufferPtr = &myLocalBuffer[0];
  }

  /** Return our storage pointer */
  unsigned char * getBufferPtr(){ return myBufferPtr; }

  /** Get a new field from us.  This doesn't load anything */
  virtual std::shared_ptr<GribField>
  getField(size_t fieldNumber) override;

  // ------------------------------------------------------
  // Low level access

  /** Attempt to read g2 information from the current buffer, return false on failure */
  virtual bool
  readG2Info(size_t messageNum, size_t at) override;

protected:

  /** My buffer which _MAY_ hold a local single message. */
  std::vector<unsigned char> myLocalBuffer;

  /** My buffer pointer which may point to myLocalBuffer, or basically a weak reference to a global buffer.
   * This requires what is handling the true buffer to stay in scope for us to remain valid. */
  unsigned char * myBufferPtr;
};
}
