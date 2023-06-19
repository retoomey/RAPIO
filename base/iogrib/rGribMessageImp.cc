#include "rGribMessageImp.h"
#include "rIOGrib.h"

#include <rError.h>

using namespace rapio;

bool
GribMessageImp::readG2Info(size_t messageNum, size_t at)
{
  myMessageNumber = messageNum; // Message number in grib2 source
  // Offset in the grib2 SOURCE.  Note, this is not an offset into our pointer, we directly point
  // to the start our grib2 message, either in our local buffer or an outside one
  myFileLocationAt = at;

  // We can just call g2_info ourselves, return false if error
  g2int numfields;
  int ierr = g2_info(getBufferPtr(), mySection0, mySection1, &numfields, &myNumLocalUse);

  if (ierr > 0) {
    LogSevere(IOGrib::getGrib2Error(ierr));
    return false;
  }
  myNumberFields = (numfields < 0) ? 0 : (size_t) (numfields);
  return true;
}

gribfield *
GribMessageImp::readFieldInfo(size_t fieldNumber)
{
  if (fieldNumber <= myNumberFields) { // fields at 1 based
    // We want the field but without data expanded unpacked 'yet'
    gribfield * gfld   = 0; // output
    const g2int unpack = 0; // 0 do not unpack
    const g2int expand = 0; // 1 expand the data?
    int ierr = g2_getfld(myBufferPtr, fieldNumber, unpack, expand, &gfld);
    if (ierr > 0) {
      LogSevere(IOGrib::getGrib2Error(ierr));
    } else {
      return gfld;
    }
  }
  return nullptr;
}

gribfield *
GribMessageImp::readField(size_t fieldNumber)
{
  if (fieldNumber <= myNumberFields) { // fields at 1 based
    // We want the field but without data expanded unpacked 'yet'
    gribfield * gfld   = 0; // output
    const g2int unpack = 1; // 1 unpack
    const g2int expand = 1; // 1 expand the data?
    int ierr = g2_getfld(myBufferPtr, fieldNumber, unpack, expand, &gfld);
    if (ierr > 0) {
      LogSevere(IOGrib::getGrib2Error(ierr));
      LogSevere("Grib2 field unpack/expand unsuccessful\n");
    } else {
      return gfld;
    }
  }
  return nullptr;
}

Time
GribMessageImp::getTime()
{
  Time theTime = Time(mySection1[5], // year
      mySection1[6],                 // month
      mySection1[7],                 // day
      mySection1[8],                 // hour
      mySection1[9],                 // minute
      mySection1[10],                // second
      0.0                            // fractional
  );

  return theTime;
}

std::string
GribMessageImp::getDateString()
{
  // Match idx and wgrib2 for now.  Strange they don't report min/sec
  // FIXME: We could make the pattern a static settable
  return (getTime().getString("%Y%m%d%H"));
  // return(getTime().getString("%Y%m%d%H%M%S"));
}
