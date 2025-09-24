#pragma once

#include <rGribDataType.h>
#include <rCatalogCallback.h>

#include <rUtility.h>

namespace rapio {
/** Store information from the entire grib2 file */
class GribCatalogCache : public Utility {
public:

  /** Create a catalog cache */
  GribCatalogCache(URL url) : myURL(url), myLoaded(false){ }

  /** Field in the cache */
  class Field {
public:
    /** The field number (Maybe we kill this to save memory) */
    size_t number;

    /** The date string */
    std::string datestring;

    /** The product string */
    std::string product;

    /** The level string */
    std::string level;

    /** The type string */
    std::string type;

    /** Print the fields in wgrib2 format style */
    void
    print(size_t m, bool grouped, size_t offset)
    {
      // Non-virtual to avoid virtual vtable overhead
      // FIXME: stream operator?
      if (grouped) {
        LogInfo(
          m << "." << number << ":" << offset << ":" << datestring << ":" << product << ":" << level << ":" << type <<
            "\n");
      } else {
        LogInfo(m << ":" << offset << ":" << datestring << ":" << product << ":" << level << ":" << type << "\n");
      }
    }
  };

  /** Message in the cache */
  class Message {
public:

    /** Offset of message */
    size_t offset;

    // We're going to assume sorted order for now on
    // a successful parse
    // size_t messageNum;
    std::vector<Field> myFields;

    /** Add a field to the message */
    void
    addField(const Field& f)
    {
      myFields.push_back(f);
    }

    /** Print message */
    void
    print(size_t messageNumber)
    {
      for (auto& f:myFields) {
        f.print(messageNumber, (myFields.size() > 1), offset);
      }
    }
  };

  /** Process a catalog line */
  bool
  processLine(const std::string& s, Field& f, size_t& offset);

  /** Add a message, in order */
  void
  addMessage(const Message& m)
  {
    myMessages.push_back(m);
  }

  /** Read the catalog if needed */
  void
  readCatalog();

  /** Match a string, return a number */
  size_t
  match(const std::string&, const std::string&, const std::string&,
    std::string& key, size_t& message, size_t& field);

  /** Match a string, return a number */
  std::vector<std::string>
  match3D(const std::string&, const std::vector<std::string>& levels);

  /** Get the field count for a message number */
  size_t
  getFieldCount(size_t messageNumber)
  {
    // No checks.  Could add
    return (myMessages[messageNumber - 1].myFields.size());
  }

  /** Release catalog information */
  void
  releaseCatalog()
  {
    myMessages.clear();
    myLoaded = false;
  }

  /** Print the catalog */
  void
  print()
  {
    // Print messages.  To save memory we use the message number
    // from the vector
    for (size_t i = 0; i < myMessages.size(); i++) {
      myMessages[i].print(i + 1);
    }
  }

protected:

  /** Is catalog loaded? */
  bool myLoaded;

  /** The file/URL we have catalog for */
  URL myURL;

  /** Messages stored */
  std::vector<Message> myMessages;
};

/** DataType for holding Grib data
 * Implement the GribDataType interface from RAPIO
 * to provide Grib2 support.
 * @author Robert Toomey */
class WgribDataTypeImp : public GribDataType {
public:
  /** Construct a grib2 implementation */
  WgribDataTypeImp(const URL& url);

  /** Print the catalog for this GribDataType. */
  void
  printCatalog();

  /** Get matched message itself by key and level */
  std::shared_ptr<GribMessage>
  getMessage(const std::string& key, const std::string& levelstr, const std::string& subtypestr = "");

  /** One way to get 2D data, using key and level string like our HMET library */
  std::shared_ptr<Array<float, 2> >
  getFloat2D(const std::string& key, const std::string& levelstr, const std::string& subtypestr = "");

  /** Read the GRIB2 data and put it in a 3-D pointer.
   *    @param key - GRIB2 parameter "TMP"
   *    @param zLevelsVec - a vector of strings containing the levels desired, e.g. ["100 mb", "250 mb"].
   */
  std::shared_ptr<Array<float, 3> >
  getFloat3D(const std::string& key, std::vector<std::string> zLevelsVec);

  /** Get a projected LatLonGrid from the grib data */
  std::shared_ptr<LatLonGrid>
  getLatLonGrid(const std::string& key, const std::string& levelstr, const std::string& subtypestr = "");

protected:

  /** Match a single field in a grib2 file */
  std::shared_ptr<CatalogCallback>
  haveSingleMatch(const std::string& match);

private:

  /** Store the URL passed to use for filename */
  URL myURL;

  /** Store catalog */
  GribCatalogCache myCatalog;
};
}
