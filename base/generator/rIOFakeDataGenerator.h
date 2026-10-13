#pragma once

#include <rIODataType.h>
#include <rRecord.h>

namespace rapio {
/** An individual generator for fake data.
 * @ingroup rapio_io
 * @brief Create a particular type of fake DataType based on factory/params
 */
class IOFakeDataGenerator {
public:
  /** Create a FakeDataGenerator */
  virtual
  ~IOFakeDataGenerator() = default;

  /** Parse the options for a generator */
  virtual bool parseOptions(const std::string& params){ return true; }

  /** For FakeIndex: Generate a single record for given time */
  virtual Record
  generateRecord(const std::string& params, const Time& time) = 0;

  /** Create a DataType */
  virtual std::shared_ptr<DataType>
  createDataType(const Record& rec) = 0;
};
}
