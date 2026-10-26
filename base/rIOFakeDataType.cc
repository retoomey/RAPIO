#include "rIOFakeDataType.h"

#include "rFactory.h"
#include "rStrings.h"
#include "rError.h"
#include "rIOURL.h"
#include "rOS.h"
#include "rIOFakeDataGenerator.h"

#include "rRadarGenerator.h"

using namespace rapio;

std::string
IOFakeDataType::getHelpString(const std::string & /*key*/)
{
  std::string help = "Fake DataType builder (Used by fake index.  Read only)";

  return help;
}

void
IOFakeDataType::initialize()
{
  static bool initialized = false;

  if (!initialized) {
    // Bootstrap all available fake data plugins.
    // As you add new generators (e.g., GridGenerator), call their introduceSelf() here.
    RadarGenerator::introduceSelf();

    initialized = true;
  }
}

Record
IOFakeDataType::generateRecord(const std::string& fullParams, const Time& time)
{
  std::cout << ")))))))))))))GENERATE " << fullParams << "\n";
  std::string genName;
  size_t pos = fullParams.find(':');

  if (pos != std::string::npos) {
    genName = fullParams.substr(0, pos);
  } else {
    genName = fullParams;
  }

  auto gen = Factory<IOFakeDataGenerator>::get(genName);

  if (gen) {
    fLogDebug("Record {}", gen->generateRecord(fullParams, time));
    return gen->generateRecord(fullParams, time);
  }

  fLogSevere("----IOFakeDataType could not find generator plugin: '{}'", genName);
  return Record();
}

std::shared_ptr<DataType>
IOFakeDataType::createDataType(IOConfig&)
{
  auto * r = Record::getCreatingRecord();

  if (r == nullptr) {
    fLogSevere("No creating record available, can't create fake data.");
    return nullptr;
  }
  auto params = r->getParams();

  if (params.size() < 2) {
    fLogSevere("Record missing parameters to determine fake generator plugin.");
    return nullptr;
  }
  std::string fullParams = params[1];

  // Same code as ArrayPipeline
  std::string type, genparams;
  char delimiter = ':';
  size_t pos     = fullParams.find(delimiter);

  if (pos != std::string::npos) {
    type      = fullParams.substr(0, pos);
    genparams = fullParams.substr(pos + 1);
  } else {
    type      = fullParams;
    genparams = "";
  }

  // FIXME: fullParams is radar:KTLX
  auto gen = Factory<IOFakeDataGenerator>::get(type);

  if (gen) {
    // Pass params to the subclass.
    return gen->createDataType(*r);
  }

  fLogSevere("IOFakeDataType could not find generator plugin: '{}'", type);
  return nullptr;
} // IOFakeDataType::createDataType

std::shared_ptr<DataType>
createDataType(IOConfig&)
{
  auto * r = Record::getCreatingRecord();

  if (r == nullptr) {
    fLogSevere("No creating record available, can't create fake data.");
    return nullptr;
  }

  // The Record params look like: {"fake", "pluginName:pluginParams"}
  auto params = r->getParams();

  if (params.size() < 2) {
    fLogSevere("Record missing parameters to determine fake generator plugin.");
    return nullptr;
  }

  // Extract the plugin name before the colon
  std::string genName = params[1];
  size_t pos = genName.find(':');

  if (pos != std::string::npos) {
    genName = genName.substr(0, pos);
  }

  auto gen = Factory<IOFakeDataGenerator>::get(genName);

  if (gen) {
    return gen->createDataType(*r);
  }

  fLogSevere("IOFakeDataType could not find generator plugin: '{}'", genName);
  return nullptr;
} // createDataType

bool
IOFakeDataType::encodeDataType(std::shared_ptr<DataType>,
  IOConfig                                               &
)
{
  fLogSevere("Unable to write fake data directly.");
  return false;
}
