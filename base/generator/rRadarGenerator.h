#pragma once

#include <rIOFakeDataGenerator.h>
#include <rConfigRadarInfo.h>
#include <rRadialSet.h>

namespace rapio {
class RadarGenerator : public IOFakeDataGenerator {
public:
  /** VCP 212 list */
  const std::vector<std::string> VCP212_Elevs = {
    "0.5", "0.9", "1.3", "1.8",  "2.4",  "3.1",  "4.0",
    "5.1", "6.4", "8.0", "10.0", "12.5", "15.6", "19.5"
  };

  static void
  introduceSelf();

  /** For FakeIndex: Generate a single record for given time */
  virtual Record
  generateRecord(const std::string& params, const Time& time) override;

  /** For IOFakeDataType: Creates the actual data payload */
  virtual std::shared_ptr<DataType>
  createDataType(const Record& rec) override;

private:
  /** Function for getting value for an angle */
  float
  getFillValueForAngle(float angle);

  /** Location in the volume */
  size_t myAtElevation = 0;
};
}
