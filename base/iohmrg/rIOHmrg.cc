#include "rIOHmrg.h"

#include "rOS.h"

#include "rHmrgProductInfo.h"

// Hand off classes to try to organize code a bit more
#include "rHmrgRadialSet.h"
#include "rHmrgLatLonGrids.h"

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
}

IOHmrg::~IOHmrg()
{ }

bool
IOHmrg::HmrgToW2(const std::string& varName,
  std::string                     & outW2Name)
{
  return theProductInfos.HmrgToW2(varName, outW2Name);
}

bool
IOHmrg::isMRMSValidYear(int year)
{
  return (!((year < 1900) || (year > 2500)));
}

float
IOHmrg::readScaledInt(gzFile fp, float scale)
{
  int temp;

  ERRNO(gzread(fp, &temp, sizeof(int)));
  if (OS::isBigEndian()) {
    OS::byteswap(temp); // Data always written little endian
  }
  return float(temp) / scale;
}

int
IOHmrg::readInt(gzFile fp)
{
  int temp;

  ERRNO(gzread(fp, &temp, sizeof(int)));
  if (OS::isBigEndian()) {
    OS::byteswap(temp); // Data always written little endian
  }
  return temp;
}

int
IOHmrg::readFloat(gzFile fp)
{
  float temp;

  ERRNO(gzread(fp, &temp, sizeof(float)));
  if (OS::isBigEndian()) {
    OS::byteswap(temp); // Data always written little endian
  }
  return temp;
}

std::string
IOHmrg::readChar(gzFile fp, size_t length)
{
  std::vector<unsigned char> v;

  v.resize(length);
  const size_t bytes = length * sizeof(unsigned char);

  ERRNO(gzread(fp, &v[0], bytes));
  std::string name;

  // Use zero to finish std::string
  for (size_t i = 0; i < bytes; i++) {
    if (v[i] == 0) { break; }
    name += v[i];
  }

  return name;
}

std::shared_ptr<DataType>
IOHmrg::readRawDataType(const URL& url)
{
  gzFile fp;
  std::shared_ptr<DataType> dataout = nullptr;

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
    if (OS::isBigEndian()) { OS::byteswap(firstYear); }
    const bool validYear = isMRMSValidYear(firstYear);

    // --------------------------------------------------------------------------
    // Factory
    if (validASCII) {
      LogInfo("HMRG reader: " << url << " (Guess: MRMS Polar Binary)\n");
      std::string radarName(v.begin(), v.end());
      dataout = IOHmrgRadialSet::readRadialSet(fp, radarName, true);
    } else if (validYear) {
      LogInfo("HMRG reader: " << url << " (Guess: MRMS Gridded Binary)\n");
      dataout = IOHmrgLatLonGrids::readLatLonGrids(fp, firstYear, true);
      // sleep(2000);
    } else {
      LogSevere("HRMG Reader: Unrecognizable radar name or valid year, can't process " << url << "\n");
    }
  } catch (const ErrnoException& ex) {
    LogSevere("Errno: " << ex.getErrnoVal() << " " << ex.getErrnoStr() << "\n");
    dataout = nullptr;
  }
  gzclose(fp);
  return dataout;
} // IOHmrg::readRawDataType

std::shared_ptr<DataType>
IOHmrg::createDataType(const std::string& params)
{
  // virtual to static
  return (IOHmrg::readRawDataType(URL(params)));
}

bool
IOHmrg::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>             & keys
)
{
  LogSevere("HMRG encoder is not implemented\n");
  return false;
} // IOHmrg::encodeDataType
