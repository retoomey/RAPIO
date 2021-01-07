#pragma once

#include <rIO.h>
#include <rRecord.h>

#include <string>
#include <vector>
#include <memory>

namespace rapio {
class DataType;
class IndexType;

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

  /** Last attempt to guess the encoder factory for a given path */
  static std::shared_ptr<IODataType>
  getFactory(
    std::string& factory, const std::string& path, std::shared_ptr<DataType> dt);

  /** Factory: Read a data file object */
  template <class T> static std::shared_ptr<T>
  read(const URL& path, const std::string& factory = "")
  {
    // Note: We make it a string for the actual factories that are generic
    std::shared_ptr<DataType> dt = readDataType(path.toString(), factory);
    if (dt != nullptr) {
      std::shared_ptr<T> dr = std::dynamic_pointer_cast<T>(dt);
      return dr;
    }
    return nullptr;
  }

  /** Factory: Figure out how to read DataType from given path */
  static std::shared_ptr<DataType>
  readDataType(const std::string& factoryparams, const std::string& factory = "");

protected:

  /**
   * Subclasses create various data objects in various ways.
   * @param params The string from params, meaning depending on the builder,
   * typically but not always a file.
   */
  virtual std::shared_ptr<DataType>
  createDataType(const std::string& params) = 0;

  // ------------------------------------------------------------------------------------
  // Writer stuff

public:

  /** Generate a file name URL from datatype attributes and given suffix */
  static URL
  generateFileName(std::shared_ptr<DataType> dt,
    const std::string                        & outputinfo,
    const std::string                        & suffix,
    bool                                     directFile,
    bool                                     useSubDirs);

  /** Generate a record based on URL */
  static void
  generateRecord(std::shared_ptr<DataType> dt,
    const URL                              & pathin,
    const std::string                      & factory,
    std::vector<Record>                    & records,
    std::vector<std::string>               & files);

  /**
   *  Chooses the correct data encoder for the particular data type,
   *  writes out the product, and appends the records to the given
   *  vector.  Can write to a set file path or a directory with
   *  filename generation
   */
  static bool
  write(std::shared_ptr<DataType> dt, const std::string& outputinfo,
    bool generateFileName,
    std::vector<Record>              & records,
    std::vector<std::string>         & files,
    const std::string& factory = "");

  /**
   *  Write out a datatype using outputinfo and factory.
   */
  static bool
  write(std::shared_ptr<DataType> dt,
    const std::string             & outputinfo,
    const std::string             & factory = "");

protected:

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    const std::string                      & params,
    std::shared_ptr<XMLNode>               dfs,
    bool                                   directFile,
    // Output for notifiers
    std::vector<Record>                    & records,
    std::vector<std::string>               & files
  ){ return false; }

public:

  /** Destroy a IO DataType */
  virtual ~IODataType(){ }

protected:

  /** Create a IO DataType */
  IODataType(){ }
};
}
