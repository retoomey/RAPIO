#include "rIORaw.h"

#include "rIOURL.h"
#include "rBinaryTable.h"

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
  // I've gotten in the habit of fully buffering data
  // since there are libraries like netcdf that require it,
  // but we 'could' get some speed ups in certain areas by
  // doing more standard c++ stream reading.
  // Double reading here for now, I want the size for debugging
  // or when I'm less lazy I'll at least file seek
  std::vector<char> buf;

  IOURL::read(url, buf);
  if (!buf.empty()) {
    LogSevere("RAW READER: " << url << " SIZE: " << buf.size() << "\n");
  } else {
    LogSevere("Couldn't read raw datatype at " << url << "\n");
    return nullptr;
  }

  FILE * fp = fopen(url.toString().c_str(), "r");

  if (fp == nullptr) {
    LogSevere("Couldn't open file at " << url << "\n");
    return nullptr;
  }

  // See chicken egg.  What we really want is to 'read' the entire tree..then use the data
  // to determine the 'class' right?  How to do this coolly?
  // Need a registered 'tree' factory that knows the headers and creates the object...
  std::shared_ptr<RObsBinaryTable> r = std::make_shared<RObsBinaryTable>();
  bool success = r->readBlock(fp);

  fclose(fp);

  if (!success) {
    LogSevere("Couldn't read binary table from raw datatype at " << url << "\n");
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
      LogSevere("Failed to write raw output file: " << keys["filename"] << "\n");
    }
    fclose(fp);

    // ----------------------------------------------------------
    // Post processing such as extra compression, ldm, etc.
    if (successful) {
      successful = postWriteProcess(filename, keys);
    }
  }
  return successful;
} // IORaw::encodeDataType
