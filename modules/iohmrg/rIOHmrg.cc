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
IOHmrg::createDataType(const std::string& params)
{
  URL url(params);

  // fLogInfo("HMRG reader: {}", url.toString());
  std::shared_ptr<DataType> datatype = nullptr;

  gzFile fp;

  // Clear any errno from other stuff that might have set it already
  // we could clear it in the macro..maybe best
  errno = 0;
  try{
    // FIXME: add a url to local temp file function to RAPIO.  We use URL's which can be remote.
    fp = gzopen(url.toString().c_str(), "rb");
    if (fp == nullptr) {
      fLogSevere("HRMG reader Couldn't open local file at {}, errno is {}", url.toString(), errno);
      gzclose(fp);
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
    std::map<std::string, std::string> keys;

    GzipFileStreamBuffer g(fp);
    g.setDataLittleEndian();
    StreamBufferToKey(keys, &g);

    if (validASCII) {
      fLogInfo("HMRG reader: {} (Guess: MRMS Polar Binary)", url.toString());
      std::shared_ptr<IOSpecializer> fmt = IOHmrg::getIOSpecializer("RadialSet");

      std::string radarName(v.begin(), v.end());
      keys["RadarName"] = radarName;
      datatype = fmt->read(keys, nullptr);
    } else if (validYear) {
      fLogInfo("HMRG reader: {} (Guess: MRMS Gridded Binary)", url.toString());
      std::shared_ptr<IOSpecializer> fmt = IOHmrg::getIOSpecializer("LatLonGrid");

      keys["DataYear"] = to_string(firstYear);
      datatype         = fmt->read(keys, nullptr);
    } else {
      fLogSevere("HRMG Reader: Unrecognizable radar name or valid year, can't process {}", url.toString());
    }
  } catch (const ErrnoException& ex) {
    fLogSevere("Errno: {} {}", ex.getErrnoVal(), ex.getErrnoStr());
    datatype = nullptr;
  }
  gzclose(fp);
  return datatype;
} // IOHmrg::createDataType

bool
IOHmrg::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>             & keys
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

  if (!resolveFileName(keys, "hmrg.gz", "hmrg-", filename)) {
    return false;
  }

  // ----------------------------------------------------------
  // Write Hmrg

  // Clear any errno from other stuff that might have set it already
  // we could clear it in the macro..maybe best
  bool successful = false;
  gzFile fp;

  errno = 0;
  try{
    // FIXME: Do we want to gzopen/write everything?  We have rapio compression methods
    // so in theory should wrap with those for generic compression.  This file format is
    // a special use case however
    fp = gzopen(filename.c_str(), "wb");
    if (fp == nullptr) {
      fLogSevere("HRMG writer Couldn't open local file at {}, errno is {}", filename, errno);
      gzclose(fp);
      return false;
    }

    // Write hmrg binary to a disk file here
    try {
      GzipFileStreamBuffer g(fp);
      g.setDataLittleEndian();
      StreamBufferToKey(keys, &g);
      successful = fmt->write(dt, keys);
      StreamBufferToKey(keys, nullptr);
    } catch (...) {
      successful = false;
      fLogSevere("Failed to write hmrg file for DataType");
    }
  } catch (const ErrnoException& ex) {
    fLogSevere("Errno: {} {}", ex.getErrnoVal(), ex.getErrnoStr());
  }
  gzclose(fp);

  // ----------------------------------------------------------
  // Post processing such as extra compression, ldm, etc.
  const std::string compress = keys["compression"];

  if (!compress.empty()) {
    fLogDebug("Turning off compression option '{}', since hmrg uses gzip automatically", compress);
    keys["compression"] = ""; // global for this run unless alg setting it
  }

  if (successful) {
    successful = postWriteProcess(filename, keys);
  }

  // Standard output
  if (successful) {
    showFileInfo("HMRG writer: ", keys);
  }

  return successful;
} // IOHmrg::encodeDataType

StreamBuffer *
IOHmrg::keyToStreamBuffer(std::map<std::string, std::string>& keys)
{
  StreamBuffer * sb;

  // Make sure enough address numbers for 128 bit machines and forever hopefully
  try{
    unsigned long long rawPointer = std::stol(keys["BUFFER_ID"]);
    sb = (StreamBuffer *) (rawPointer); // Clip down to os pointer size
  }catch (...) {                        // allow fail to nullptr
    sb = nullptr;
  }
  return sb;
}

void
IOHmrg::StreamBufferToKey(std::map<std::string, std::string>& keys, StreamBuffer * sb)
{
  keys["BUFFER_ID"] = to_string((unsigned long long) (sb));
}
