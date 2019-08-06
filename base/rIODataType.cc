#include "rIODataType.h"

#include "rFactory.h"
#include "rDataType.h"
#include "rStrings.h"
#include "rConfigDataFormat.h"
#include "rIOXML.h"

using namespace rapio;

// -----------------------------------------------------------------------------------------
// Reader stuff
//
std::shared_ptr<IODataType>
IODataType::getIODataType(const std::string& sourceType)
{
  return (Factory<IODataType>::get(sourceType, "IODataType source"));
}

// -----------------------------------------------------------------------------------------
// Writer stuff
//

// static
std::shared_ptr<IODataType>
IODataType::getDataWriter(const DataType & dt,
  const std::string                      & outputDir)
{
  std::shared_ptr<DataFormatSetting> dfs = ConfigDataFormat::getSetting(dt.getDataType());

  if (dfs != nullptr) {
    return (dfs->prototype);
  }
  return (0);
}

// static
std::string
IODataType::writeData(const DataType & dt,
  const std::string                  & outputDir,
  std::vector<Record>                & records)
{
  std::string subtype;

  if (getSubType(dt, subtype)) {
    return (writeData(dt, outputDir, subtype, records));
  }

  std::shared_ptr<IODataType> encoder = getDataWriter(dt, outputDir);

  if (encoder != nullptr) {
    const bool useSubDirs = true;
    std::string result    = encoder->encode(dt, outputDir, useSubDirs, records);
    return (result);
  }
  return (std::string());
}

std::string
IODataType::writeData(const DataType & dt,
  const std::string                  & outputDir,
  const std::string                  & subtype,
  std::vector<Record>                & records)
{
  std::shared_ptr<IODataType> encoder = getDataWriter(dt, outputDir);

  if (encoder != nullptr) {
    const bool useSubDirs = true;
    std::string result    = encoder->encode(dt, subtype,
        outputDir, useSubDirs, records);
    return (result);
  }
  return (std::string());
}

std::string
IODataType::writeData(const DataType & dt,
  const std::string                  & outputDir,
  RecordNotifier *                   notifier)
{
  std::vector<Record> records;
  std::string result = writeData(dt, outputDir, records);

  // LogDebug("\tDEBUG! call notifier.writeRecords() " << LINE_ID << "\n");
  if (notifier != nullptr) {
    notifier->writeRecords(records);
  }
  return (result);
}

std::string
IODataType::writeData(const DataType & dt,
  const std::string                  & outputDir,
  const std::string                  & subtype,
  RecordNotifier *                   notifier)
{
  std::vector<Record> records;
  std::string result = writeData(dt, outputDir, subtype, records);

  // LogDebug("\tDEBUG! call notifier.writeRecords() " << LINE_ID << "\n");
  if (notifier != nullptr) {
    notifier->writeRecords(records);
  }
  return (result);
}

bool
IODataType::getSubType(const DataType& dt, std::string& result)
{
  std::string subtype = dt.getDataAttributeString("SubType");

  if (subtype == "NoPRI") {
    return (false);
  }

  if (!subtype.empty()) {
    result = subtype;
    return (true);
  }
  return (false);
}
