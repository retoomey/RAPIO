#pragma once

#include <rIO.h>
#include <rRecord.h>
#include <rRecordNotifier.h>

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
 */
class IODataType : public IO {
public:

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

  // ------------------------------------------------------------------------------------
  // Writer stuff

public:

  /**
   *  Chooses the correct data encoder for the particular data type,
   *  writes out the product, and appends the records to the given
   *  vector.
   *  @return filename (empty if error)
   */
  static std::string
  writeData(const DataType & dt,
    const std::string      & outputDir,
    std::vector<Record>    & records);

  /**
   *  Chooses the correct data encoder for the particular data type,
   *  writes out the product, and appends the records to the given
   *  vector. The records all have the specified sub-type.
   *  @return filename (empty if error)
   */
  static std::string
  writeData(const DataType & dt,
    const std::string      & outputDir,
    const std::string      & subtype,
    std::vector<Record>    & records);

  /**
   *  Chooses the correct data encoder for the particular data type,
   *  writes out the product, and notifies about the resulting record(s)
   *  @return filename (empty if error)
   */
  static std::string
  writeData(const DataType & dt,
    const std::string      & outputDir,
    RecordNotifier *       notifier);

  /**
   *  Chooses the correct data encoder for the particular data type,
   *  writes out the product, and notifies about the resulting record(s)
   *  @return filename (empty if error)
   */
  static std::string
  writeData(const DataType & dt,
    const std::string      & outputDir,
    const std::string      & subtype,
    RecordNotifier *       notifier);
protected:

  /** Write out this product and append the Record(s) for it
   *  into the resulting vector. No record will be appended if
   *  there was a problem. @return filename (empty if error) */
  virtual std::string
  encode(const rapio::DataType& dt,
    const std::string         & directory,
    bool                      useSubDirs,
    std::vector<Record>       & records) = 0;

  /** Write out this product and append the Record(s) for it
   *   (but use the specified subtype string)
   *   into the resulting vector. No record will be appended if
   *   there was a problem. @return filename (empty if error) */
  virtual std::string
  encode(const rapio::DataType& dt,
    const std::string         & subtype,
    const std::string         & directory,
    bool                      useSubDirs,
    std::vector<Record>       & records) = 0;

  /** Forward any extra settings specific to encoder from XML file. */
  virtual void
  applySettings(const std::map<std::string,
    std::string>& settings){ }

private:

  static std::shared_ptr<IODataType>
  getDataWriter(const DataType & dt,
    const std::string          & outputDir);

  static bool
  getSubType(const DataType& dt,
    std::string            & subtype);

public:

  virtual ~IODataType(){ }

protected:

  IODataType(){ }
};
}
