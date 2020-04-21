#pragma once

#include <rIO.h>
#include <rRecord.h>
#include <rConfigDataFormat.h>

#include <string>
#include <vector>
#include <memory>

namespace rapio {
class DataType;
class IndexType;
class ConfigDataFormat;

/**
 * IODataType has the ability to read and write some group of objects,
 * such as netcdf output, xml, etc.
 * We group reader and writer ability into a single object since typically
 * the libraries, headers, initialization are common for a third party library.
 *
 * @author Robert Toomey
 */
class IODataType : public IO {
public:

  /** Initialize a loaded IODataType */
  virtual void
  initialize(){ };

  // ------------------------------------------------------------------------------------
  // Reader stuff
  //

  /** Factory: Read a data file object */
  template <class T> static std::shared_ptr<T>
  read(const URL& path, const std::string& factory = "")
  {
    std::shared_ptr<DataType> dt = readDataType(path, factory);
    if (dt != nullptr) {
      std::shared_ptr<T> dr = std::dynamic_pointer_cast<T>(dt);
      return dr;
    }
    return nullptr;
  }

  /** Factory: Figure out how to read DataType from given path */
  static std::shared_ptr<DataType>
  readDataType(const URL& path, const std::string& factory = "");

protected:

  /**
   * Subclasses create various data objects in various ways.
   * @param url The URL to the data location.
   */
  virtual std::shared_ptr<DataType>
  createDataType(const URL& url) = 0;

  // ------------------------------------------------------------------------------------
  // Writer stuff

public:

  /**
   *  Chooses the correct data encoder for the particular data type,
   *  writes out the product, and appends the records to the given
   *  vector.  Can write to a set file path or a directory with
   *  filename generation
   */
  static bool
  write(std::shared_ptr<DataType> dt, const URL& path,
    bool generateFileName,
    std::vector<Record>              & records,
    const std::string& factory = "");

  /**
   *  Write out to a set given path, ignore records.
   */
  static bool
  write(std::shared_ptr<DataType> dt,
    const URL                     & path,
    const std::string             & factory = "");

protected:

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    const URL                              & path,
    std::shared_ptr<DataFormatSetting>     dfs){ return false; }

public:

  /** Destroy a IO DataType */
  virtual ~IODataType(){ }

protected:

  /** Create a IO DataType */
  IODataType(){ }
};
}
