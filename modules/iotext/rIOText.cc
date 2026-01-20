#include "rIOText.h"
#include "rIOURL.h"

// Default built in DataType support
#include "rTextDataGrid.h"
#include "rTextBinaryTable.h"

#include <fstream>

using namespace rapio;

std::ostream * IOText::theFile = nullptr;

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
  fLogSevere("Reading text with IOTEXT unsupported, at least at the moment.");
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
    fLogSevere("Can't create a text IO writer for datatype {}", type);
    return false;
  }

  // Check if console because it's a lot simplier...
  bool console = (!keys["console"].empty());

  if (console) {
    theFile = &std::cout; // allowed
    return (fmt->write(dt, keys));
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
    std::ofstream currentFile(filename, std::ofstream::out);
    if (currentFile.is_open()) {
      theFile    = &currentFile;
      successful = fmt->write(dt, keys);
      currentFile.close();
    } else {
      fLogSevere("Couldn't open {} for writing.", filename);
      successful = false;
    }
  }catch (const std::exception& e) {
    fLogSevere("Failed to write {}, reason: {}", filename, e.what());
  }

  // ----------------------------------------------------------
  // Post processing such as extra compression, ldm, etc.
  if (successful) {
    successful = postWriteProcess(filename, keys);
  }

  // Standard output
  if (successful) {
    showFileInfo("Text writer: ", keys);
  }

  return successful;
} // IOText::encodeDataType
