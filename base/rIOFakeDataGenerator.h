#pragma once

#include <rIODataType.h>

#include <rConfigRadarInfo.h>
#include <rRadialSet.h>

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

  /** For FakeIndex: Generate a single record for given time */
  virtual Record
  generateRecord(const std::string& params, const Time& time) = 0;

  /** For IOFakeDataType: Creates the actual data payload */
  virtual std::shared_ptr<DataType>
  createDataType(const Record& rec) = 0;

  /** Get the help for this generator (todo) */
  //  virtual std::string
  //  getHelp() const = 0;
};

class RadarGenerator : public IOFakeDataGenerator {
public:
  /** VCP 212 list */
  const std::vector<std::string> VCP212_Elevs = {
    "0.5", "0.9", "1.3", "1.8",  "2.4",  "3.1",  "4.0",
    "5.1", "6.4", "8.0", "10.0", "12.5", "15.6", "19.5"
  };

  /** For FakeIndex: Generate a single record for given time */
  virtual Record
  generateRecord(const std::string& params, const Time& time) override;

  /** For IOFakeDataType: Creates the actual data payload */
  std::shared_ptr<DataType>
  createDataType(const Record& rec) override;

private:
  /** Function for getting value for an angle */
  float
  getFillValueForAngle(float angle);

  /** Location in the volume */
  int myAtElevation = 0;
};
}
