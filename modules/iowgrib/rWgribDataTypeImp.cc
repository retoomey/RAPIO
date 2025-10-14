#include "rWgribDataTypeImp.h"
#include "rIOWgrib.h"

#include "rWgribMessageImp.h"

// Callbacks
#include "rCatalogCallback.h"
#include "rGridCallback.h"
#include "rArrayCallback.h"

#include <fstream>

// Cache includes
#include "rStrings.h"

using namespace rapio;

bool
GribCatalogCache::processLine(const std::string& line, GribCatalogCache::Field& f, size_t& offset, int& atMessage)
{
  atMessage = -1;
  try {
    // Split the line into parts on :
    std::vector<std::string> out;
    Strings::split(line, ':', &out);

    // Size should be at least 5...
    if (out.size() < 5) {
      LogSevere("Parse Line is '" << line << "'\n");
      LogSevere("Found a line of size " << out.size() << "\n");
      for (size_t i = 0; i < out.size(); ++i) {
        LogSevere("--> " << i << "'" << out[i] << "'\n");
      }
      return false;
    }

    // Handle the message field.. (105.1) [0]
    std::vector<std::string> message;
    Strings::split(out[0], '.', &message);
    if (message.size() > 0) {
      // We assume message ordering (might bite us later)
      auto messageNumber = std::stoull(message[0]);
      f.number  = (message.size() > 1) ? std::stoull(message[1]) : 1;
      atMessage = messageNumber;
    } else {
      LogSevere("Parse Line is '" << line << "'\n");
      LogSevere("Tried to split '" << out[0] << " and failed.\n");
      return false;
    }

    // Handle the offset field.. (3102971) [1]
    offset = std::stol(out[1]); // can throw

    // FIXME: Issue here is that wgrib2 seems to have many versions of this
    // in the field (see wgrib2/Sec1.c) So for now, we'll just store the
    // string.
    #if 0
    // One format I've see is d=YYYYMMDDHH
    std::vector<std::string> outdate;
    Strings::split(out[2], '=', &outdate);
    if ((outdate.size() == 2) && (outdate[0] == "d")) {
      // LogDebug("DATE: " << outdate[1] << "\n");
      const std::string convert = "%Y%m%d%H"; // One of the Grib formats it seems
      idx.myTime = Time(outdate[1], convert);
      // LogDebug("Converted Time is " << idx.myTime.getString(convert) << "\n");
    } else {
      break;
      // bad date?
    }
    #endif // if 0

    // Handle the date field [2]
    f.datestring = out[2];

    // Handle the product and field names [3, 4]
    f.product = out[3];
    f.level   = out[4];
    f.type    = out[5];
  }catch (const std::exception& e)
  {
    LogSevere("Parse Line is '" << line << "'\n");
    LogSevere("Exception parsing: " << e.what() << "\n");
    atMessage = -1;
    return false;
  }
  return true;
} // GribCatalogCache::processLine

void
GribCatalogCache::readCatalog()
{
  // Already loaded, skip out
  if (myLoaded) { return; }

  std::shared_ptr<CatalogCallback> action = std::make_shared<CatalogCallback>(myURL, "", "");
  auto v = action->execute(false);
  auto c = action->getMatchCount();

  bool success = true;
  Field buffer;
  size_t count = 0;
  size_t offset;

  int atMessage = 1; // Parser found message

  for (auto& l:v) {
    // Read next field.  Any error we assume catalog or wgrib2 bad
    auto f = processLine(l, buffer, offset, atMessage);
    if (!f) {
      success = false;
      continue; // try to recover from any bad lines in stream
    }

    // On first field, add a new message
    if (buffer.number == 1) {
      Message m;
      myMessages.push_back(m);
    }

    if (myMessages.size() == atMessage) {
      // Add field to the latest message
      myMessages[myMessages.size() - 1].offset = offset;
      myMessages[myMessages.size() - 1].addField(buffer);
    } else {
      LogSevere("Mismatched message number " << atMessage << ", "
                                             << "expected " << myMessages.size() << "\n");
      break; // what to do?
    }
    count++;
  }

  // Our parser should match the wgrib2 count
  if (c != count) {
    LogSevere("Mismatch wgrib2 count vs c" << count << ", " << c << "\n");
    success = false;
  }

  if (!success) {
    LogSevere("Error trying to read/parse catalog information\n");
  }

  //  LogInfo("Catalog matched: " << c << " items\n");

  myLoaded = true;
} // GribCatalogCache::readCatalog

size_t
GribCatalogCache::match(const std::string& product,
  const std::string& level, const std::string& type,
  std::string& key, size_t& message, size_t& field)
{
  // Make sure catalog cached.
  readCatalog();

  size_t count = 0;
  std::stringstream ss;
  bool useLevel = !level.empty();
  bool useType  = !type.empty();

  for (size_t i = 0; i < myMessages.size(); i++) {
    const auto& m = myMessages[i];
    for (size_t j = 0; j < m.myFields.size(); j++) {
      const auto& f = m.myFields[j];
      if (f.product != product) {
        continue;
      }
      if (useLevel && (f.level != level)) {
        continue;
      }
      if (useType && (f.type != type)) {
        continue;
      }
      // Only one match allowed
      if (count > 1) { break; }

      ss.str(""); // Clear
      ss.clear(); // Clear state flags
      // Note, wgrib2 can handle a .1 even for single fields
      ss << i + 1 << "." << j + 1;
      message = i + 1;
      field   = j + 1;
      key     = ss.str();
      count++;
    }
  }
  return count;
} // GribCatalogCache::match

std::vector<std::string>
GribCatalogCache::match3D(const std::string& product,
  const std::vector<std::string>           & zLevels)
{
  // Make sure catalog cached.
  readCatalog();

  // We need to keep the order of the zLevels, even if the fields
  // are out of order in the file
  std::vector<std::string> keys(zLevels.size());

  // Scan for the product and levels
  size_t count = 0;
  std::stringstream ss;

  for (size_t i = 0; i < myMessages.size(); i++) {
    const auto& m = myMessages[i];
    for (size_t j = 0; j < m.myFields.size(); j++) {
      const auto& f = m.myFields[j];

      if (f.product == product) {
        for (size_t k = 0; k < zLevels.size(); k++) {
          if (f.level == zLevels[k]) {
            ss.str(""); // Clear
            ss.clear(); // Clear state flags
            // Note, wgrib2 can handle a .1 even for single fields
            ss << i + 1 << "." << j + 1;
            keys[k] = ss.str();
            count++;
          }
        }
      }
    }
  }

  // If we didn't find all the fields, clear it so caller can know
  if (count != keys.size()) {
    keys.clear();
  }
  return keys;
} // GribCatalogCache::match3D

WgribDataTypeImp::WgribDataTypeImp(const URL& url) : myURL(url), myCatalog(url)
{
  myDataType = "GribData";
}

void
WgribDataTypeImp::printCatalog()
{
  myCatalog.readCatalog();
  myCatalog.print();
}

std::shared_ptr<CatalogCallback>
WgribDataTypeImp::haveSingleMatch(const std::string& match)
{
  // Run a catalog and make sure we have a single match.  Any more and we warn and fail.
  // For example, we don't want a bad match field to decode the entire file for example,
  // which could decode too much stuff and hang/freeze our realtime alg.
  std::shared_ptr<CatalogCallback> catalog = std::make_shared<CatalogCallback>(myURL, match, "");

  catalog->execute();
  auto c = catalog->getMatchCount();

  if (c != 1) {
    LogSevere("\"" << match << "\" matches " << c << " field(s). You need an exact match\n");
    return nullptr;
  }
  return catalog;
}

std::shared_ptr<GribMessage>
WgribDataTypeImp::getMessage(const std::string& key, const std::string& levelstr, const std::string& subtypestr)
{
  // ---------------------------------------------------
  // Find the keys "message.field" for the requested field
  std::string keystr;
  size_t message, field;
  auto c = myCatalog.match(key, levelstr, subtypestr, keystr, message, field);

  if (c != 1) {
    LogSevere("getFloat2D keys match " << c << " items.  Needs to be 1 unique match.\n");
    return nullptr;
  }

  // Running the catalog and matching one gives us the info section in catalog
  // Note, so far no decoding/etc. has taken place. We do that later
  auto fieldCount = myCatalog.getFieldCount(message);
  std::shared_ptr<CatalogCallback> cc = std::make_shared<CatalogCallback>(myURL, "", keystr);

  cc->execute();

  auto m = std::make_shared<WgribMessageImp>(myURL, message, fieldCount,
      cc->getFilePosition(), cc->getSection0(), cc->getSection1());

  return m;
} // WgribDataTypeImp::getMessage

std::shared_ptr<Array<float, 2> >
WgribDataTypeImp::getFloat2D(const std::string& key, const std::string& levelstr, const std::string& subtypestr)
{
  // ---------------------------------------------------
  // Find the keys "message.field" for the requested field
  std::string keystr;
  size_t message, field;
  auto c = myCatalog.match(key, levelstr, subtypestr, keystr, message, field);

  if (c != 1) {
    LogSevere("getFloat2D keys match " << c << " items.  Needs to be 1 unique match.\n");
    return nullptr;
  }

  // Run array now on single match
  std::shared_ptr<Array2DCallback> action = std::make_shared<Array2DCallback>(myURL, "", keystr);

  action->execute();

  return Array2DCallback::pull2DArray();
} // WgribDataTypeImp::getFloat2D

std::shared_ptr<Array<float, 3> >
WgribDataTypeImp::getFloat3D(const std::string& key, std::vector<std::string> zLevels)
{
  // ---------------------------------------------------
  // Find the keys "message.field" numbers for each level
  //
  auto keys = myCatalog.match3D(key, zLevels);

  if (keys.size() != zLevels.size()) {
    LogSevere("Didn't find all requested 3D levels.\n");
    return nullptr;
  }

  // ---------------------------------------------------
  // Create a 3D callback and fill the layers
  //
  std::shared_ptr<Array3DCallback> action =
    std::make_shared<Array3DCallback>(myURL, key, keys);

  for (size_t i = 0; i < zLevels.size(); ++i) {
    action->executeLayer(i);
  }

  return Array3DCallback::pull3DArray();
} // WgribDataTypeImp::getFloat3D

std::shared_ptr<LatLonGrid>
WgribDataTypeImp::getLatLonGrid(const std::string& key, const std::string& levelstr, const std::string& subtypestr)
{
  // ---------------------------------------------------
  // Find the keys "message.field" for the requested field
  std::string keystr;
  size_t message, field;
  auto c = myCatalog.match(key, levelstr, subtypestr, keystr, message, field);

  if (c != 1) {
    LogSevere("getLatLonGrid keys match " << c << " items.  Needs to be 1 unique match.\n");
    return nullptr;
  }

  std::shared_ptr<GridCallback> action = std::make_shared<GridCallback>(myURL, "", keystr);

  action->execute();

  return GridCallback::pullLatLonGrid();
}
