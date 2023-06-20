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
  g2int numfields, localUse, section0[3], section1[13];

  int ierr = g2_info(getBufferPtr(), &section0[0], &section1[0], &numfields, &localUse);

  if (ierr > 0) {
    LogSevere(IOGrib::getGrib2Error(ierr));
    return false;
  }

  // Doing this copy in case g2int type changes from long..unlikely but older versions of
  // library are different so compiler would complain
  for (size_t i = 0; i < 3; ++i) {
    mySection0[i] = (long) (section0[i]);
  }
  for (size_t i = 0; i < 13; ++i) {
    mySection1[i] = (long) (section1[i]);
  }

  myNumberFields   = (numfields < 0) ? 0 : (size_t) (numfields);
  myNumberLocalUse = (localUse < 0) ? 0 : (size_t) (localUse);
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
