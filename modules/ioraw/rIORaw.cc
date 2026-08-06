#include "rIORaw.h"

#include "rIOURL.h"
#include "rBinaryTable.h"
#include "rBinaryIO.h"
#include "rRawSpecializer.h"

// Introduced raw binary table types
// FIXME: anyway to introduce dynamically from core,
// might be nice if an alg could define one on the fly.
// Right now it's part of core always
#include "rFusionBinaryTable.h"
#include "rMergerBinaryTable.h"
#include "rWindBinaryTable.h"

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

// ----------------------------------------------------------------------------
// Local Specializer Implementations
// These could go into their own files, we'll leave here for now unless it
// starts to grow
// ----------------------------------------------------------------------------
class FusionBinaryTableRawSpecializer : public RawSpecializer {
public:
  virtual std::shared_ptr<DataType>
  readRAW(FILE * fp, const std::string& path, IOConfig& keys) override
  {
    auto t = std::make_shared<FusionBinaryTable>();

    if (t->readBlock(path, fp)) { return t; }
    return nullptr;
  }
};

// The w2merger tables are RObs and WObs.  So we can dump them pretty much.
// Fusion has its own internal storage.
class RObsBinaryTableRawSpecializer : public RawSpecializer {
public:
  virtual std::shared_ptr<DataType>
  readRAW(FILE * fp, const std::string& path, IOConfig& keys) override
  {
    auto t = std::make_shared<RObsBinaryTable>();

    if (t->readBlock(path, fp)) { return t; }
    return nullptr;
  }
};

class WObsBinaryTableRawSpecializer : public RawSpecializer {
public:
  virtual std::shared_ptr<DataType>
  readRAW(FILE * fp, const std::string& path, IOConfig& keys) override
  {
    auto t = std::make_shared<WObsBinaryTable>();

    if (t->readBlock(path, fp)) { return t; }
    return nullptr;
  }
};

// The wind requires several fields for the least squares
class WindBinaryTableRawSpecializer : public RawSpecializer {
public:
  virtual std::shared_ptr<DataType>
  readRAW(FILE * fp, const std::string& path, IOConfig& keys) override
  {
    auto t = std::make_shared<WindBinaryTable>();

    if (t->readBlock(path, fp)) { return t; }
    return nullptr;
  }
};

// ----------------------------------------------------------------------------

std::string
IORaw::getHelpString(const std::string& key)
{
  std::string help;

  help += "builder that reads Merger Stage 1 RAW files.";
  return help;
}

void
IORaw::initialize()
{
  // Register all known RAW table formats by their MAGIC STRING signature
  introduce("W2-F", std::make_shared<FusionBinaryTableRawSpecializer>());
  introduce("W2-W", std::make_shared<WObsBinaryTableRawSpecializer>());
  introduce("W2-W-R", std::make_shared<RObsBinaryTableRawSpecializer>()); // RObs inherits from WObs
  introduce("W2-WIND", std::make_shared<WindBinaryTableRawSpecializer>());
}

IORaw::~IORaw()
{ }

std::shared_ptr<DataType>
IORaw::createDataType(IOConfig& config)
{
  URL url(config.getParamURL());

  fLogInfo("Raw reader: {}", url.toString());

  FILE * fp = nullptr;
  std::shared_ptr<DataType> result = nullptr;

  try {
    fp = fopen(url.toString().c_str(), "rb");
    if (fp == nullptr) {
      fLogSevere("Couldn't open file at {}", url.toString());
      return nullptr;
    }

    // 1. Read ONLY the Magic String to identify the file format
    std::string disk_magic;
    BinaryIO::read_string8(disk_magic, fp);

    // Check if we hit EOF or read failed immediately
    if (feof(fp) || ferror(fp)) {
      throw std::runtime_error("Failed to read magic string from header. File may be empty or corrupt.");
    }

    // Rewind the file pointer so the specific reader can parse the entire block
    fseek(fp, 0, SEEK_SET);

    // 2. Look up the specific reader from the IOSpecializer registry
    std::shared_ptr<IOSpecializer> fmt = getIOSpecializer(disk_magic);
    if (fmt == nullptr) {
      throw std::runtime_error("No RAW specializer registered for Magic String '" + disk_magic + "'.");
    }

    std::shared_ptr<RawSpecializer> rawFmt = std::dynamic_pointer_cast<RawSpecializer>(fmt);
    if (rawFmt == nullptr) {
      throw std::runtime_error("Registered specializer for '" + disk_magic + "' is not a RawSpecializer.");
    }

    // 3. Hand off the file pointer to the specialized reader
    result = rawFmt->readRAW(fp, url.toString(), config);

    if (!result) {
      throw std::runtime_error("RawSpecializer failed to read or validate the block data.");
    }
  } catch (const std::exception& e) {
    fLogSevere("Error reading RAW datatype from {}: {}", url.toString(), e.what());
    result = nullptr; // Ensure we don't pass back a partially constructed object
  }

  // Ensure file is closed regardless of exceptions
  if (fp != nullptr) {
    fclose(fp);
  }

  return result;
} // IORaw::createDataType

bool
IORaw::encodeDataType(std::shared_ptr<DataType> dt, IOConfig & keys)
{
  bool successful = false;
  FILE * fp       = nullptr;

  try {
    auto output = std::dynamic_pointer_cast<BinaryTable>(dt);
    if (output == nullptr) {
      throw std::runtime_error("IORaw writer received a DataType that is not a BinaryTable.");
    }

    std::string filename;
    if (!resolveFileName(keys, "raw", "raw-", filename)) {
      throw std::runtime_error("Failed to resolve output filename.");
    }

    fp = fopen(filename.c_str(), "w");
    if (fp == nullptr) {
      throw std::runtime_error("Failed to open file for writing: " + filename);
    }

    successful = output->writeBlock(fp);
    if (!successful) {
      throw std::runtime_error("Failed to write raw output block data to file.");
    }

    // Close before post-processing so LDM or other tools can access it
    fclose(fp);
    fp = nullptr;

    if (successful) {
      successful = postWriteProcess(filename, keys);
    }

    if (successful) {
      showFileInfo("Raw writer: ", keys);
    }
  } catch (const std::exception& e) {
    fLogSevere("Error writing RAW datatype: {}", e.what());
    successful = false;
  }

  // Ensure file is closed if an exception occurred while it was open
  if (fp != nullptr) {
    fclose(fp);
  }

  return successful;
} // IORaw::encodeDataType
