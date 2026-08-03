#include "rIOHmrg.h"

#include "rOS.h"

#include "rHmrgProductInfo.h"

// Hand off classes to try to organize code a bit more
#include "rHmrgRadialSet.h"
#include "rHmrgLatLonGrids.h"

#include "rRadialSet.h"
#include "rLatLonGrid.h"
#include "rLatLonHeightGrid.h"
#include "rBinaryIO.h"

using namespace rapio;

ProductInfoSet IOHmrg::theProductInfos;

// Library dynamic link to create this factory
extern "C"
{
void *
createRAPIOIO(void)
{
  auto * z = new IOHmrg();

  z->initialize();
  return reinterpret_cast<void *>(z);
}
};

std::string
IOHmrg::getHelpString(const std::string& key)
{
  std::string help;

  help += "builder that reads/writes HMET hmrg binary files.";
  return help;
}

void
IOHmrg::initialize()
{
  theProductInfos.readConfigFile();
  // theProductInfos.dump();

  // Add the default classes we handle...
  HmrgRadialSet::introduceSelf(this);
  HmrgLatLonGrids::introduceSelf(this);
}

IOHmrg::~IOHmrg()
{ }

ProductInfo *
IOHmrg::getProductInfo(const std::string& varName, const std::string& units)
{
  return theProductInfos.getProductInfo(varName, units);
}

bool
IOHmrg::HmrgToW2Name(const std::string& varName,
  std::string                         & outW2Name)
{
  return theProductInfos.HmrgToW2Name(varName, outW2Name);
}

bool
IOHmrg::isMRMSValidYear(int year)
{
  return (!((year < 1900) || (year > 2500)));
}

std::shared_ptr<DataType>
IOHmrg::createDataType(IOConfig& config)
{
  fLogSevere("Here we at {}", __LINE__);
  URL url(config.getParamURL());

  // fLogInfo("HMRG reader: {}", url.toString());
  std::shared_ptr<DataType> datatype = nullptr;

  gzFile fp = nullptr;

  // Clear any errno from other stuff that might have set it already
  // we could clear it in the macro..maybe best
  errno = 0;
  try{
    // FIXME: add a url to local temp file function to RAPIO.  We use URL's which can be remote.
    fp = gzopen(url.toString().c_str(), "rb");
    if (fp == nullptr) {
      fLogSevere("HRMG reader Couldn't open local file at {}, errno is {}", url.toString(), errno);
      return nullptr;
    }
    // --------------------------------------------------------------------------
    // Guessing the binary format based on the first four characters, which is either
    // a Radar name, or a Year in little endian.  Some fuzzy logic here
    // We check names match letters and numbers, and year matches reasonably around
    // data expected.
    // Note: noticed some HMET code they byte swap.  Soooo if we were on a Big Endian
    // system this reader would fail at moment.
    std::vector<unsigned char> v;
    v.resize(4);
    bool validASCII = true;
    ERRNO(gzread(fp, &v[0], 4 * sizeof(unsigned char)));
    for (size_t i = 0; i < 4; i++) {
      const int c = v[i];
      if ((c >= 65) && (c <= 90)) { continue; }  // A-Z
      if ((c >= 97) && (c <= 127)) { continue; } // a-z
      if ((c >= 48) && (c <= 57)) { continue; }  // 0-9
      validASCII = false;
      break;
    }
    unsigned int firstYear = (v[0] | (v[1] << 8) | (v[2] << 16) | (v[3] << 24)); // little endian
    #if IS_BIG_ENDIAN
    OS::byteswap(firstYear);
    #endif
    const bool validYear = isMRMSValidYear(firstYear);

    // --------------------------------------------------------------------------
    // Factory
    GzipFileStreamBuffer g(fp);
    g.setDataLittleEndian();
    StreamBufferToKey(config, &g);

    if (validASCII) {
      fLogInfo("HMRG reader: {} (Guess: MRMS Polar Binary)", url.toString());
      std::shared_ptr<IOSpecializer> fmt = IOHmrg::getIOSpecializer("RadialSet");

      std::string radarName(v.begin(), v.end());
      config.set("RadarName", radarName);
      datatype = fmt->read(config);
    } else if (validYear) {
      fLogInfo("HMRG reader: {} (Guess: MRMS Gridded Binary)", url.toString());
      std::shared_ptr<IOSpecializer> fmt = IOHmrg::getIOSpecializer("LatLonGrid");

      config.set("DataYear", to_string(firstYear));
      datatype = fmt->read(config);
    } else {
      fLogSevere("HRMG Reader: Unrecognizable radar name or valid year, can't process {}", url.toString());
    }
  } catch (const ErrnoException& ex) {
    fLogSevere("Errno: {} {}", ex.getErrnoVal(), ex.getErrnoStr());
    datatype = nullptr;
  }
  if (fp != nullptr) {
    gzclose(fp);
  }
  return datatype;
} // IOHmrg::createDataType

bool
IOHmrg::encodeDataType(std::shared_ptr<DataType> dt,
  IOConfig                                       & config
)
{
  // ----------------------------------------------------------
  // Get specializer for the data type
  const std::string type = dt->getDataType();

  std::shared_ptr<IOSpecializer> fmt = IOHmrg::getIOSpecializer(type);

  if (fmt == nullptr) {
    fLogSevere("Can't create a writer for datatype {}", type);
    return false;
  }

  // ----------------------------------------------------------
  // Get the filename we should write to
  std::string filename;

  if (!resolveFileName(config, "hmrg.gz", "hmrg-", filename)) {
    return false;
  }

  // ----------------------------------------------------------
  // Write Hmrg

  // Clear any errno from other stuff that might have set it already
  // we could clear it in the macro..maybe best
  bool successful = false;
  gzFile fp       = nullptr;

  errno = 0;
  try{
    // FIXME: Do we want to gzopen/write everything?  We have rapio compression methods
    // so in theory should wrap with those for generic compression.  This file format is
    // a special use case however
    fp = gzopen(filename.c_str(), "wb");
    if (fp == nullptr) {
      fLogSevere("HRMG writer Couldn't open local file at {}, errno is {}", filename, errno);
      return false;
    }

    // Write hmrg binary to a disk file here
    try {
      GzipFileStreamBuffer g(fp);
      g.setDataLittleEndian();
      StreamBufferToKey(config, &g);
      successful = fmt->write(dt, config);
      StreamBufferToKey(config, nullptr);
    } catch (...) {
      successful = false;
      fLogSevere("Failed to write hmrg file for DataType");
    }
  } catch (const ErrnoException& ex) {
    fLogSevere("Errno: {} {}", ex.getErrnoVal(), ex.getErrnoStr());
  }
  if (fp != nullptr) {
    gzclose(fp);
  }

  // ----------------------------------------------------------
  // Post processing such as extra compression, ldm, etc.
  const std::string compress = config.get("compression");

  if (!compress.empty()) {
    fLogDebug("Turning off compression option '{}', since hmrg uses gzip automatically", compress);
    config.set("compression", ""); // global for this run unless alg setting it
  }

  if (successful) {
    successful = postWriteProcess(filename, config);
  }

  // Standard output
  if (successful) {
    showFileInfo("HMRG writer: ", config);
  }

  return successful;
} // IOHmrg::encodeDataType

StreamBuffer *
IOHmrg::keyToStreamBuffer(IOConfig& keys)
{
  StreamBuffer * sb;

  // Make sure enough address numbers for 128 bit machines and forever hopefully
  try{
    unsigned long long rawPointer = std::stol(keys.get("BUFFER_ID"));
    sb = (StreamBuffer *) (rawPointer); // Clip down to os pointer size
  }catch (...) {                        // allow fail to nullptr
    sb = nullptr;
  }
  return sb;
}

void
IOHmrg::StreamBufferToKey(IOConfig& keys, StreamBuffer * sb)
{
  keys.set("BUFFER_ID", to_string((unsigned long long) (sb)));
}
