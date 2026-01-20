#include "rIORaw.h"

#include "rIOURL.h"
#include "rBinaryTable.h"
#include "rFusionBinaryTable.h"
#include "rMergerBinaryTable.h"

using namespace rapio;

// Library dynamic link to create this factory
extern "C"
{
void *
createRAPIOIO(void)
{
  auto * z = new IORaw();

  z->initialize();
  return reinterpret_cast<void *>(z);
}
};

std::string
IORaw::getHelpString(const std::string& key)
{
  std::string help;

  help += "builder that reads Merger Stage 1 RAW files.";
  return help;
}

void
IORaw::initialize()
{ }

IORaw::~IORaw()
{ }

std::shared_ptr<DataType>
IORaw::readRawDataType(const URL& url)
{
  fLogInfo("Raw reader: {}", url.toString());
  #if 0
  // From a buffer (web read, etc)
  std::vector<char> buf;
  IOURL::read(url, buf);
  if (buf.empty()) {
    fLogSevere("Couldn't read raw datatype at {}", url.toString());
    return nullptr;
  }
  #endif

  FILE * fp = fopen(url.toString().c_str(), "rb");

  if (fp == nullptr) {
    fLogSevere("Couldn't open file at {}", url.toString());
    return nullptr;
  }

  // Read all generic BinaryTable readBlock header, which contains the
  // datatype. This will allow us to specialize
  std::shared_ptr<BinaryTable> r = std::make_shared<BinaryTable>();
  bool success = r->readBlock(url.toString(), fp);

  if (success) {
    // Recreate the subclass.  Header is small we're ok to reread it
    fseek(fp, 0, SEEK_SET); // != 0 error check.  FIXME: use c++ right?

    // FIXME: We have IOSpecializers for this, hacking for now
    // 'ioraw' and 'iotext' are both special pets vs the other library
    // based modules.
    std::string datatype = r->getDataType();
    if (datatype == "Toomey was here") { // Make MRMS files work
      datatype = "RObsBinaryTable";
    }

    if (datatype == "FusionBinaryTable") {
      r = std::make_shared<FusionBinaryTable>();
    } else if (datatype == "RObsBinaryTable") {
      r = std::make_shared<RObsBinaryTable>();
    } else if (datatype == "WObsBinaryTable") {
      r = std::make_shared<WObsBinaryTable>();
    } else {
      success = false;
    }
    if (success) {
      success = r->readBlock(url.toString(), fp);
    }
  }

  fclose(fp);

  if (!success) {
    fLogSevere("Couldn't read binary table from raw datatype at {}", url.toString());
    return nullptr;
  }

  return r;
} // IORaw::readRawDataType

std::shared_ptr<DataType>
IORaw::createDataType(const std::string& params)
{
  // virtual to static
  return (IORaw::readRawDataType(URL(params)));
}

bool
IORaw::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>            & keys
)
{
  bool successful = false;
  auto output     = std::dynamic_pointer_cast<BinaryTable>(dt);

  if (output != nullptr) {
    // ----------------------------------------------------------
    // Get the filename we should write to
    std::string filename;
    if (!resolveFileName(keys, "raw", "raw-", filename)) {
      return false;
    }

    // ----------------------------------------------------------
    // Write Raw

    // FIXME: skipping writing temp file here, think we're ok.
    // Though I think we should merge some of this code into common functions at
    // some point with the other modules.
    FILE * fp = fopen(filename.c_str(), "w");
    successful = output->writeBlock(fp); // write block is virtual
    if (!successful) {
      fLogSevere("Failed to write raw output file: {}", keys["filename"]);
    }
    fclose(fp);

    // ----------------------------------------------------------
    // Post processing such as extra compression, ldm, etc.
    if (successful) {
      successful = postWriteProcess(filename, keys);
    }

    // Standard output
    if (successful) {
      showFileInfo("Raw writer: ", keys);
    }
  }
  return successful;
} // IORaw::encodeDataType
