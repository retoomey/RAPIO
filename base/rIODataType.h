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
 * IOSpecializer
 *
 * Specializers handle a particular class of a data format.
 * For example, netcdf can read in generic netcdf, but this can
 * specialize into a RadialSet, which is no longer netcdf data.
 *
 * @author Robert Toomey
 */
class IOSpecializer : public IO {
public:
  /** Write a given DataType */
  virtual bool
  write(std::shared_ptr<DataType> dt,
    std::map<std::string, std::string>& keys) = 0;

  /** Read a DataType from given information */
  virtual std::shared_ptr<DataType>
  read(
    std::map<std::string, std::string>& keys,
    std::shared_ptr<DataType> optionalOriginal) = 0;
};

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

  /** Introduce dynamic help */
  static std::string
  introduceHelp();

  /** Help for this particular instance of IODataType */
  virtual std::string
  getHelpString(const std::string& key){ return "Help for " + key; }

  /** Initialize a loaded IODataType */
  virtual void
  initialize(){ };

  /** Specializers are readers that specialize the reading into a type.
   * For example, reading netcdf might turn into a RadialSet which is
   * no longer a netcdf object.  XML might create a DataTable. */
  virtual void
  introduce(const std::string      & dataType,
    std::shared_ptr<IOSpecializer> new_subclass);

  /** Returns a named specializer
   *  @return 0 if we don't know about this type.
   */
  std::shared_ptr<IOSpecializer>
  getIOSpecializer(const std::string& type);

  // ------------------------------------------------------------------------------------
  // Reader stuff
  //

  /** Last attempt to guess the encoder factory for a given path */
  static std::shared_ptr<IODataType>
  getFactory(
    std::string& factory, const std::string& path, std::shared_ptr<DataType> dt);

  /** Read from a given factory and parameters for that factory */
  template <class T> static std::shared_ptr<T>
  read(const std::string& params, const std::string& factory = "")
  {
    // Note: We make it a string for the actual factories that are generic
    std::shared_ptr<DataType> dt = readDataType(params, factory);
    if (dt != nullptr) {
      std::shared_ptr<T> dr = std::dynamic_pointer_cast<T>(dt);
      return dr;
    }
    return nullptr;
  }

  /** Attempt to read from given factory from a buffer in memory.
   * Note here you have to provide the factory always, we can't guess from chars */
  template <class T> static std::shared_ptr<T>
  readBuffer(std::vector<char>& buffer, const std::string& factory)
  {
    // Note: We make it a string for the actual factories that are generic
    std::shared_ptr<DataType> dt = readBufferImp(buffer, factory);
    if (dt != nullptr) {
      std::shared_ptr<T> dr = std::dynamic_pointer_cast<T>(dt);
      return dr;
    }
    return nullptr;
  }

  /** Create a new DataType object based on parameters and factory */
  static std::shared_ptr<DataType>
  readDataType(const std::string& factoryparams, const std::string& factory = "");

protected:

  /** Create a new DataType object based on buffer and given factory */
  static std::shared_ptr<DataType>
  readBufferImp(std::vector<char>& buffer, const std::string& factory);

  /** Subclasses that can read from a character buffer can implement this.
   * Since not everything can read from a buffer, we default to nullptr */
  virtual std::shared_ptr<DataType>
  createDataTypeFromBuffer(std::vector<char>& buffer)
  {
    return nullptr;
  }

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

  /** Generate a file name URL from datatype attributes */
  static URL
  generateFileName(std::shared_ptr<DataType> dt,
    const std::string                        & outputinfo,
    const std::string                        & basepattern);

  /** Generate a record based on URL */
  static void
  generateRecord(std::shared_ptr<DataType> dt,
    const URL                              & pathin,
    const std::string                      & factory,
    std::vector<Record>                    & records);

  /**
   *  Chooses the correct data encoder for the particular data type,
   *  writes out the product, and appends the records to the given
   *  vector.  Can write to a set file path or a directory with
   *  filename generation
   */
  static bool
  write(std::shared_ptr<DataType> dt, const std::string& outputinfo,
    std::vector<Record>              & records,
    const std::string& factory,
    std::map<std::string, std::string>& outputParams);

  /**
   *  Write out a datatype using outputinfo and factory.
   */
  static bool
  write(std::shared_ptr<DataType> dt,
    const std::string             & outputinfo,
    const std::string             & factory = "");

  /** Attempt to write to a buffer, not all writers can
   * do this */
  static size_t
  writeBuffer(std::shared_ptr<DataType> dt,
    std::vector<char>                   & buf,
    const std::string                   & factory = "");

  /** Handle parsing the command line param.  For example
   * factory=outputfolder, or factory= script, outputfolder.
   * This turns the command line into the param map values */
  virtual void
  handleCommandParam(const std::string& command,
    std::map<std::string, std::string> &outputParams);

  /** Default write out handling for files */
  virtual bool
  writeout(std::shared_ptr<DataType> dt, const std::string& outputinfo,
    std::vector<Record>              & records,
    const std::string& factory,
    std::map<std::string, std::string>& outputParams);

protected:

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    std::map<std::string, std::string>     & lookup
  ){ return false; }

  /** Subclasses that can write to a character buffer can implement this.
   * Since not everything can write to a buffer, we default to nullptr */
  virtual size_t
  encodeDataTypeBuffer(std::shared_ptr<DataType> dt, std::vector<char>& buffer)
  {
    return 0;
  }

public:

  /** Destroy a IO DataType */
  virtual ~IODataType(){ }

protected:

  /** Create a IO DataType */
  IODataType(){ }

  /** Specializers */
  std::map<std::string, std::shared_ptr<IOSpecializer> > mySpecializers;
};
}
