#include "rWgribMessageImp.h"
#include "rWgribFieldImp.h"
#include "rIOWgrib.h"

#include <rError.h>

using namespace rapio;

WgribMessageImp::WgribMessageImp(const URL& url,
  int messageNumber,
  long int filePos,
  std::array<long, 3>& sec0, std::array<long, 13>& sec1) : myURL(url)
{
  // FIXME: Make the base class use std::array. For now I'll copy
  // since I don't want to mess with the older iogrib module yet
  //  mySection0 = sec0;
  //  mySection1 = sec1;
  std::copy(sec0.begin(), sec0.end(), mySection0);
  std::copy(sec1.begin(), sec1.end(), mySection1);
  myMessageNumber  = messageNumber;
  myFileLocationAt = (size_t) (filePos);

  // FIXME: We need to do another query in order to get number of fields, right?
  // Or we do it higher up and pass in.
}

std::shared_ptr<GribField>
WgribMessageImp::getField(size_t fieldNumber)
{
  // FIXME: Create and return a proper field.

  if (fieldNumber <= myNumberFields) { // fields at 1 based
    auto newField = std::make_shared<WgribFieldImp>();
    return newField;
  } else {
    LogSevere("Requesting Grib field " << fieldNumber
                                       << ", but we only have " << myNumberFields << " fields.\n");
  }
  return nullptr;
}
