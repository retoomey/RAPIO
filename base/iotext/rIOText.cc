#include "rIOText.h"
#include "rIOURL.h"

// Default built in DataType support
#include "rTextDataGrid.h"
#include "rTextBinaryTable.h"

using namespace rapio;

std::ofstream IOText::theFile;
// = nullptr;

// Library dynamic link to create this factory
extern "C"
{
void *
createRAPIOIO(void)
{
  auto * z = new IOText();

  z->initialize();
  return reinterpret_cast<void *>(z);
}
};

std::string
IOText::getHelpString(const std::string& key)
{
  std::string help;

  help += "builder for outputting text of supported DataTypes.";
  return help;
}

void
IOText::initialize()
{
  TextDataGrid::introduceSelf(this);
  TextBinaryTable::introduceSelf(this);
}

IOText::~IOText()
{ }

std::shared_ptr<DataType>
IOText::readRawDataType(const URL& url)
{
  LogSevere("Reading text with IOTEXT unsupported, at least at the moment.\n");
  return nullptr;
} // IOText::readRawDataType

std::shared_ptr<DataType>
IOText::createDataType(const std::string& params)
{
  // virtual to static
  return (IOText::readRawDataType(URL(params)));
}

bool
IOText::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>             & keys
)
{
  // ----------------------------------------------------------
  // Get specializer for the data type
  const std::string type = dt->getDataType();

  std::shared_ptr<IOSpecializer> fmt = IOText::getIOSpecializer(type);

  // Default base class for now at least is DataType, so if doesn't cast
  // we have no methods to write this
  if (fmt == nullptr) {
    auto dataGrid = std::dynamic_pointer_cast<DataGrid>(dt);
    if (dataGrid != nullptr) {
      fmt = IOText::getIOSpecializer("DataGrid");
    }
  }

  if (fmt == nullptr) {
    LogSevere("Can't create a text IO writer for datatype " << type << "\n");
    return false;
  }

  // ----------------------------------------------------------
  // Get the filename we should write to
  std::string filename;

  if (!resolveFileName(keys, "text", "text-", filename)) {
    return false;
  }

  // ----------------------------------------------------------
  // Write Text

  bool successful = false;

  try{
    theFile = std::ofstream(filename, std::ofstream::out);
    if (theFile.is_open()) {
      // We 'could' try to convert file ofstream to string and put in keys,
      // gonna use a static.
      // Note: This would break if we ever multithread write without locking.
      successful = fmt->write(dt, keys);
      theFile.close();
    } else {
      LogSevere("Couldn't open " << filename << " for writing.\n");
      successful = false;
    }
  }catch (const std::exception& e) {
    LogSevere("Failed to write " << filename << ", reason: " << e.what() << "\n");
  }

  // ----------------------------------------------------------
  // Post processing such as extra compression, ldm, etc.
  if (successful) {
    successful = postWriteProcess(filename, keys);
  }

  LogInfo("Text writer: " << keys["filename"] << "\n");

  return successful;
} // IOText::encodeDataType
