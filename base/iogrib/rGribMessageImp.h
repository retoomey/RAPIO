#pragma once

#include <rTime.h>
#include <rGribDataType.h>

#include <vector>

extern "C" {
#include <grib2.h>
}

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

  /** Attempt to read g2 information from the current buffer, return false on failure */
  bool
  readG2Info(size_t messageNum, size_t at);

  // ------------------------------------------------------
  // Low level access
  // FIXME: These might get wrapped later.  Caller currently
  // should call g2_free after use.  Could have a single method
  // with expand/unpack but two seems clearer on intention

  /** Raw read a GRIB2 field number as info only, no unpacking/expanding data.
   * Return a fresh pointer on success. */
  gribfield *
  readFieldInfo(size_t fieldNumber);

  /** Raw read a GRIB2 field number fully.  This unpacks/expands data.
   * Returns a fresh pointer on success */
  gribfield *
  readField(size_t fieldNumber);

protected:

  /** My buffer which _MAY_ hold a local single message. */
  std::vector<unsigned char> myLocalBuffer;

  /** My buffer pointer which may point to myLocalBuffer, or basically a weak reference to a global buffer.
   * This requires what is handling the true buffer to stay in scope for us to remain valid. */
  unsigned char * myBufferPtr;
};
}
