#include "rIOFakeDataType.h"

#include "rFactory.h"
#include "rStrings.h"
#include "rError.h"
#include "rIOURL.h"
#include "rOS.h"
#include "rIOFakeDataGenerator.h"

using namespace rapio;

std::string
IOFakeDataType::getHelpString(const std::string & key)
{
  std::string help = "Fake DataType builder (Used by fake index.  Read only)";

  return help;
}

void
IOFakeDataType::initialize()
{
  std::shared_ptr<RadarGenerator> radarSim = std::make_shared<RadarGenerator>();
  Factory<IOFakeDataGenerator>::introduce("radar1", radarSim);
}

/** Read call */
std::shared_ptr<DataType>
IOFakeDataType::createDataType(const std::string& params)
{
  auto * r = Record::getCreatingRecord();

  if (r == nullptr) {
    fLogSevere("No creating record available, can't create fake data.");
    return nullptr;
  }

  auto gen = Factory<IOFakeDataGenerator>::get("radar1");

  return gen ? gen->createDataType(*r) : nullptr;
} // IOFakeDataType::createDataType

bool
IOFakeDataType::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>                     & keys
)
{
  fLogSevere("Unable to write fake data directly.");
  return false;
}
