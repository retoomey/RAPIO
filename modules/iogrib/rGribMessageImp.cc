#include "rGribMessageImp.h"
#include "rGribFieldImp.h"
#include "rIOGrib.h"

#include <rError.h>

extern "C" {
#include <grib2.h>
}

#include <iostream>

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
    fLogSevere("{}", IOGrib::getGrib2Error(ierr));
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

std::shared_ptr<GribField>
GribMessageImp::getField(size_t fieldNumber)
{
  if (fieldNumber <= myNumberFields) { // fields at 1 based
    auto newField = std::make_shared<GribFieldImp>(myBufferPtr, myMessageNumber, fieldNumber, myDataTypeValid);
    return newField;
  } else {
    fLogSevere("Requesting Grib field {}, but we only have {} fields.", fieldNumber, myNumberFields);
  }
  return nullptr;
}

std::ostream&
GribMessageImp::print(std::ostream& os)
{
  for (size_t fieldNumber = 1; fieldNumber <= myNumberFields; ++fieldNumber) {
    GribFieldImp theField(myBufferPtr, myMessageNumber, fieldNumber, myDataTypeValid);
    // Message 'header'
    os << myMessageNumber;
    if (myNumberFields > 1) {
      os << "." << fieldNumber;
    }
    os << ":" << getFileOffset() << ":";
    // Now the field
    theField.print(os);
  }
  return os;
}
