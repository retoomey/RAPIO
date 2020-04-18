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

  /**
   * Convenience function to get the builder for a sourceType, eg. hires.
   * @param sourceType The sourceType name, such as "AWIPS" or "hires"
   */
  static std::shared_ptr<IODataType>
  getIODataType(const std::string& sourceType);

  /**
   * Create a data object.
   * @param params The parameters needed to create the object.
   *               Their number and meaning vary by sourceType.
   */
  virtual std::shared_ptr<DataType>
  createObject(const std::vector<std::string>& params) = 0;

  /** Standard get filename from param list */
  static URL
  getFileName(const std::vector<std::string>& params);

  // ------------------------------------------------------------------------------------
  // Writer stuff

public:

  /** Generate information from datatype for the output URL location and
   * the specials strings used to generate notification records.
   * FIXME: still feel this could be cleaner. */
  static URL
  generateOutputInfo(const DataType    & dt,
    const std::string                  & directory,
    std::shared_ptr<DataFormatSetting> dfs,
    const std::string                  & suffix,
    std::vector<std::string>           & params,
    std::vector<std::string>           & selections);

  /**
   *  Chooses the correct data encoder for the particular data type,
   *  writes out the product, and appends the records to the given
   *  vector.
   *  @return filename (empty if error)
   */
  static std::string
  writeData(std::shared_ptr<DataType> dt,
    const std::string                 & outputDir,
    std::vector<Record>               & records);

protected:

  /** Write out this product and append the Record(s) for it
   *   into the resulting vector. No record will be appended if
   *   there was a problem. @return filename (empty if error) */
  virtual std::string
  encode(std::shared_ptr<DataType>     dt,
    const std::string                  & directory,
    std::shared_ptr<DataFormatSetting> dfs,
    std::vector<Record>                & records) = 0;

public:

  /** Destroy a IO DataType */
  virtual ~IODataType(){ }

protected:

  /** Create a IO DataType */
  IODataType(){ }
};
}
