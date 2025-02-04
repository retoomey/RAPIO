#include "rIOFile.h"

#include "rFactory.h"
#include "rStrings.h"
#include "rError.h"
#include "rIOURL.h"
#include "rOS.h"
#include "rConfigIODataType.h"

using namespace rapio;

std::string
IOFile::getHelpString(const std::string & key)
{
  std::string help;

  help = "hands off to another writer using a lookup table of suffixes.\n";
  return help;
}

std::shared_ptr<DataType>
IOFile::createDataTypeFromBuffer(std::vector<char>& buffer)
{
  return nullptr;
}

/** Read call */
std::shared_ptr<DataType>
IOFile::createDataType(const std::string& params)
{
  return nullptr;
} // IOFile::createDataType

bool
IOFile::writeout(std::shared_ptr<DataType> dt,
  const std::string                        & outputinfo,
  std::vector<Record>                      & records,
  const std::string                        & knownfactory,
  std::map<std::string, std::string>       & outputParams)
{
  // Get the proxy factory off the file name
  // knownfactory is 'file' of course, but we want to send onto our
  // proxy builder to do the actual work, so force to extension.
  const std::string suffix = OS::getRootFileExtension(outputinfo);

  std::string factory = ConfigIODataType::getIODataTypeFromSuffix(suffix);

  if (factory == "file") {
    // We don't want to proxy to ourselves and make an infinite loop.
    LogSevere("File builder cannot proxy to file builder (itself), so cannot write.\n");
    return false;
  }

  // Get the new proxy factory for this output
  std::string f = factory; // either passed in, or blank or suffix guess stuff.

  // We are going to proxy to another IODataType
  outputParams["filepathmode"] = "direct";
  const bool success = write1(dt, outputinfo, records, f, outputParams);
  if (!success){ return false; }

  // Notification currently has default paths and things, if we do want notification for single
  // files we'll have to tweek that code some.  FIXME: I can see doing this
  LogInfo("Ignoring " << records.size() << " record notifications since you're using single file builder.\n");
  records.clear();
  return success;
} // IOFile::writeout

bool
IOFile::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>             & keys
)
{
  LogSevere("File builder can't encode anything itself, nothing writes.  You should be calling IODataType::write().\n");
  return false;
}

size_t
IOFile::encodeDataTypeBuffer(std::shared_ptr<DataType> dt, std::vector<char>& buffer)
{
  // FIXME: Do we do buffer stuff for file?  I don't think we need it at least for now.
  return 0;
}
