#include "rIOHmrg.h"

#include "rOS.h"

#include "rHmrgProductInfo.h"

// Hand off classes to try to organize code a bit more
#include "rHmrgRadialSet.h"
#include "rHmrgLatLonGrids.h"

#include "rRadialSet.h"
#include "rLatLonGrid.h"
#include "rLatLonHeightGrid.h"

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

bool
IOHmrg::HmrgToW2Name(const std::string& varName,
  std::string                         & outW2Name)
{
  return theProductInfos.HmrgToW2Name(varName, outW2Name);
}

bool
IOHmrg::W2ToHmrgName(const std::string& varName,
  std::string                         & outW2Name)
{
  return theProductInfos.W2ToHmrgName(varName, outW2Name);
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

  // LogInfo("HMRG reader: " << url << "\n");
  std::shared_ptr<DataType> datatype = nullptr;

  gzFile fp;

  // Clear any errno from other stuff that might have set it already
  // we could clear it in the macro..maybe best
  errno = 0;
  try{
    // FIXME: add a url to local temp file function to RAPIO.  We use URL's which can be remote.
    fp = gzopen(url.toString().c_str(), "rb");
    if (fp == nullptr) {
      LogSevere("HRMG reader Couldn't open local file at " << url << ", errno is " << errno << "\n");
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
    GZFileToKey(keys, fp);
    if (validASCII) {
      LogInfo("HMRG reader: " << url << " (Guess: MRMS Polar Binary)\n");
      std::shared_ptr<IOSpecializer> fmt = IOHmrg::getIOSpecializer("RadialSet");

      std::string radarName(v.begin(), v.end());
      keys["RadarName"] = radarName;
      datatype = fmt->read(keys, nullptr);
    } else if (validYear) {
      LogInfo("HMRG reader: " << url << " (Guess: MRMS Gridded Binary)\n");
      std::shared_ptr<IOSpecializer> fmt = IOHmrg::getIOSpecializer("LatLonGrid");

      keys["DataYear"] = to_string(firstYear);
      datatype         = fmt->read(keys, nullptr);
    } else {
      LogSevere("HRMG Reader: Unrecognizable radar name or valid year, can't process " << url << "\n");
    }
  } catch (const ErrnoException& ex) {
    LogSevere("Errno: " << ex.getErrnoVal() << " " << ex.getErrnoStr() << "\n");
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
    LogSevere("Can't create a writer for datatype " << type << "\n");
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
      LogSevere("HRMG writer Couldn't open local file at " << filename << ", errno is " << errno << "\n");
      gzclose(fp);
      return false;
    }

    // Write hmrg binary to a disk file here
    try {
      GZFileToKey(keys, fp);
      successful = fmt->write(dt, keys);
      GZFileToKey(keys, nullptr);
    } catch (...) {
      successful = false;
      LogSevere("Failed to write hmrg file for DataType\n");
    }
  } catch (const ErrnoException& ex) {
    LogSevere("Errno: " << ex.getErrnoVal() << " " << ex.getErrnoStr() << "\n");
  }
  gzclose(fp);

  // ----------------------------------------------------------
  // Post processing such as extra compression, ldm, etc.
  const std::string compress = keys["compression"];

  if (!compress.empty()) {
    LogDebug("Turning off compression option '" << compress << "', since hmrg uses gzip automatically\n");
    keys["compression"] = ""; // global for this run unless alg setting it
  }

  if (successful) {
    successful = postWriteProcess(filename, keys);
  }

  // Standard output
  if (successful){
    showFileInfo("HMRG writer: ", keys);
  }

  return successful;
} // IOHmrg::encodeDataType

gzFile
IOHmrg::keyToGZFile(std::map<std::string, std::string>& keys)
{
  gzFile fp;

  // Make sure enough address numbers for 128 bit machines and forever hopefully
  try{
    unsigned long long rawPointer = std::stol(keys["GZFILE_ID"]);
    fp = (gzFile) (rawPointer); // Clip down to os pointer size
  }catch (...) {                // allow fail to nullptr
    fp = nullptr;
  }
  return fp;
}

void
IOHmrg::GZFileToKey(std::map<std::string, std::string>& keys, gzFile fp)
{
  keys["GZFILE_ID"] = to_string((unsigned long long) (fp));
}
