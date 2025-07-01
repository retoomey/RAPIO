#include "rWgribMessageImp.h"
#include "rWgribFieldImp.h"
#include "rCatalogCallback.h"
#include "rIOWgrib.h"

#include <rError.h>

using namespace rapio;

WgribMessageImp::WgribMessageImp(const URL& url,
  int messageNumber,
  int numFields,
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
  myNumberFields   = numFields;
}

std::shared_ptr<GribField>
WgribMessageImp::getField(size_t fieldNumber)
{
  if (fieldNumber <= myNumberFields) { // fields at 1 based
    // We query now for the given field number, say "^12:" for a single field,
    // or "^12.5:" for multiple fields.
    std::string match = "^" + std::to_string(myMessageNumber);
    if (myNumberFields > 1) {
      match += "." + std::to_string(fieldNumber);
    }
    match += ":";
    CatalogCallback c(myURL, match);
    c.execute();
    auto count = c.getMatchCount();
    if (count > 1) {
      LogSevere("Match of \"" << match << "\" matches " << count << " fields and it should be 1.\n");
      return nullptr;
    }

    auto newField = std::make_shared<WgribFieldImp>(myURL, myMessageNumber, fieldNumber,
        c.getFilePosition(), c.getSection0(),
        c.getSection1());
    return newField;
  } else {
    LogSevere("Requesting Grib field " << fieldNumber
                                       << ", but we only have " << myNumberFields << " fields.\n");
  }
  return nullptr;
}
