#include "rIONIDS.h"
#include "rIOURL.h"
#include "rBinaryIO.h"

// Default built in DataType support
#include "rNIDSRadialSet.h"

#include "rConfigNIDSInfo.h"

#include "rBlockMessageHeader.h"
#include "rBlockProductDesc.h"
#include "rBlockProductSymbology.h"
#include "rBlockRadialSet.h"

#include <cstdio>

using namespace rapio;
using namespace std;

// Library dynamic link to create this factory
extern "C"
{
void *
createRAPIOIO(void)
{
  auto * z = new IONIDS();

  z->initialize();
  return reinterpret_cast<void *>(z);
}
};

std::string
IONIDS::getHelpString(const std::string& key)
{
  std::string help;

  help += "builder that reads/writes NIDS Nexrad Level III data.";
  return help;
}

void
IONIDS::initialize()
{
  // Add the default classes we handle...
  NIDSRadialSet::introduceSelf(this);
}

IONIDS::~IONIDS()
{ }

void
IONIDS::readHeaders(StreamBuffer& b)
{
  // FIXME: Move to the rBlockMessageHeader class, allowing
  // storing of headers to 'write' if wanted.

  // ------------------------------------------------------------------
  // Hunt and skip headers if they exist.
  // Level III products that come from NOAA, NCDC, or the NWS FTP feeds
  // are wrapped in a WMO/AWIPS text bulletin header.
  // Each header ends with a cr, cr, lf or 0x0D 0x0D 0x0A.
  // I'm using the DFA to also trigger on 0x0D 0x0A, but I haven't seen it
  // I've seen at most two headers.
  //
  // There can be a WMO header something like:
  //    SDUS82 → WMO product ID (origin + type)
  //    KGSP → Station identifier (here, Greer, SC)
  //    082001 → Date/time group (day = 08, time = 2001 UTC)
  //
  // There can be a AWIPS header like:
  //    NYX → Product category (e.g., NEXRAD “text” product)
  //    GSP → Radar site ID again
  const size_t MAX_HEADER_SKIP = 2;
  size_t at = 0;

  for (size_t h = 0; h < MAX_HEADER_SKIP; ++h) { // up to two headers
    size_t state = 0;
    for (size_t i = 0; i < 100; ++i) {
      auto c = b.readChar();
      switch (state) {
          case 0: // Checking for first 0D
            if (c == 0x0D) { state = 1; }
            break;
          case 1: // Checking for another 0D or 0A
            if (c == 0x0D) { state = 1; } else if (c == 0x0A) {
              at += i + 1;
              i   = 10000; // break out
            } else { state = 0; }
            break;
          default:
            break;
      }
    }
    b.seek(at);
  }
} // IONIDS::readHeaders

std::shared_ptr<DataType>
IONIDS::createDataType(const std::string& params)
{
  // FIXME: Do we try/catch here or higher I forget
  URL url(params);

  fLogInfo("NIDS reader: {}", url.toString());
  std::shared_ptr<DataType> datatype = nullptr;
  std::vector<char> buf;

  IOURL::read(url, buf);

  if (buf.empty()) {
    fLogSevere("Empty data buffer from url, can't create.");
    return nullptr;
  }

  try {
    // Wrap the buffer in a memory stream, allowing us to
    // read from it.  This passes ownership of buf to avoid copying
    size_t fullSize = buf.size();
    MemoryStreamBuffer b(std::move(buf));

    b.setDataBigEndian(); // NIDS is in big endian

    fLogInfo("Processing NIDS {}", url.toString());

    readHeaders(b); // Skip WMO/AWIPS headers if any
    BlockMessageHeader header;
    const size_t dataSize = fullSize - b.tell();
    header.read(b);
    header.dump();
    const auto expectedSize = header.getMsgLength();

    // Check the length given by message matches the full byte size.
    // This is the number minus the headers of course
    if (expectedSize == dataSize) {
      fLogInfo("NIDS header/size match: {} bytes.", expectedSize);
    } else {
      fLogSevere("Expected {} bytes, but we have {}", expectedSize, dataSize);
      throw(std::runtime_error("Invalid NIDS data size."));
    }

    BlockProductDesc desc;

    desc.read(b); // desc.dump();

    // So here we need product info so we can find out if
    // compressed or it's a RadialSet, etc.
    int product     = desc.getProductCode();
    const auto info = ConfigNIDSInfo::getNIDSInfo(product);

    std::string dataType = info.getMsgFormat();
    if (dataType == "Radial") { dataType = "RadialSet"; }

    std::shared_ptr<IOSpecializer> fmt = IONIDS::getIOSpecializer(dataType);
    if (fmt != nullptr) {
      std::shared_ptr<NIDSSpecializer> nidsFmt =
        std::dynamic_pointer_cast<NIDSSpecializer>(fmt);

      if (nidsFmt != nullptr) {
        // FIXME:Humm I thought reads came from above, maybe not
        std::map<std::string, std::string> keys;

        BlockProductSymbology sym;
        if (info.getChkCompression()) {
          // char test = b.readChar();
          // std::cout << "CHARACTER should be 66 for 'B' IS " << (int)(test) << "\n";
          // exit(1);

          MemoryStreamBuffer z = b.readBZIP2();
          sym.read(z);
          datatype = nidsFmt->readNIDS(keys, header, desc, sym, z);
        } else {
          sym.read(b);
          datatype = nidsFmt->readNIDS(keys, header, desc, sym, b);
        }

        if (datatype) {
          datatype->postRead(keys);
        }
        return datatype;
      }
    } else {
      fLogSevere("Couldn't find a specializer for datatype {}", dataType);
    }
  } catch (const std::exception& e) {
    fLogSevere("Failed to read NIDS {}", e.what());
  }

  return nullptr;
} // IONIDS::readNIDSDataType

bool
IONIDS::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>             & keys
)
{
  fLogSevere("Writing NIDS probably not working.");
  // ----------------------------------------------------------
  // Get specializer for the data type
  const std::string type = dt->getDataType();

  std::shared_ptr<IOSpecializer> fmt = IONIDS::getIOSpecializer(type);

  if (fmt == nullptr) {
    fLogSevere("Can't create a nids IO writer for datatype {}", type);
    return false;
  }
  std::shared_ptr<NIDSSpecializer> nidsFmt =
    std::dynamic_pointer_cast<NIDSSpecializer>(fmt);

  // ----------------------------------------------------------
  // Get the filename we should write to
  std::string filename;

  if (!resolveFileName(keys, "nids", "nids-", filename)) {
    return false;
  }

  // ----------------------------------------------------------
  // Write Nids

  // Clear any errno from other stuff that might have set it already
  // we could clear it in the macro..maybe best
  bool successful = false;
  FILE * fp;

  errno = 0;

  try{
    fp = fopen(filename.c_str(), "wb");
    if (fp == nullptr) {
      fLogSevere("NIDS writer Couldn't open local file at {}, errno is {}", filename, errno);
      fclose(fp);
      return false;
    }

    // Write NIDS binary to a disk file here
    try {
      FileStreamBuffer g(fp);
      g.setDataBigEndian(); // NIDS is BigEndian
      successful = nidsFmt->writeNIDS(keys, dt, g);
    } catch (...) {
      successful = false;
      fLogSevere("Failed to write nids file for DataType.");
    }
  } catch (const ErrnoException& ex) {
    fLogSevere("Errno: {} {}", ex.getErrnoVal(), ex.getErrnoStr());
  }
  fclose(fp);

  // ----------------------------------------------------------
  // Post processing such as extra compression, ldm, etc.
  if (successful) {
    successful = postWriteProcess(filename, keys);
  }

  // Standard output
  if (successful) {
    showFileInfo("NIDS (LEVELIII) writer: ", keys);
  }

  return successful;
} // IONIDS::encodeDataType
