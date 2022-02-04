#include "rIOHmrg.h"

#include "rIOURL.h"
#include "rRadialSet.h"

using namespace rapio;

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
{ }

IOHmrg::~IOHmrg()
{ }

namespace {
// What we consider a valid year in the MRMS binary file, used for validation of datatype
bool
isMRMSValidYear(int year)
{
  return (!((year < 1900) || (year > 2500)));
}

// Check endianness (OS) function probably
bool
is_big_endian()
{
  // Humm even better, set a global variable on start up I think...
  static bool firstTime = true;
  static bool big       = false;

  if (!firstTime) {
    union {
      uint32_t i;
      char     c[4];
    } bint = { 0x01020304 };
    big       = (bint.c[0] == 1);
    firstTime = false;
  }
  return big;
}

// Swap bytes for a given type for endian change
template <class T>
inline void
byteswap(T &data)
{
  size_t N = sizeof( T );

  if (N == 1) { return; }
  size_t mid = floor(N / 2); // Even 4 --> 2 Odd 5 --> 2 (middle is already good)

  char * char_data = reinterpret_cast<char *>( &data );

  for (size_t i = 0; i < mid; i++) { // swap in place
    const char temp = char_data[i];
    char_data[i]         = char_data[N - i - 1];
    char_data[N - i - 1] = temp;
  }
}

/** Read a scaled integer with correct endian and return as a float */
float
readScaledInt(gzFile fp, float scale)
{
  int temp;

  ERRNO(gzread(fp, &temp, sizeof(int)));
  if (is_big_endian()) {
    byteswap(temp); // Data always written little endian
  }
  return float(temp) / scale;
}

/** Read an integer with correct endian and return as an int */
int
readInt(gzFile fp)
{
  int temp;

  ERRNO(gzread(fp, &temp, sizeof(int)));
  if (is_big_endian()) {
    byteswap(temp); // Data always written little endian
  }
  return temp;
}

/** Read a float with correct endian and return as a float */
int
readFloat(gzFile fp)
{
  float temp;

  ERRNO(gzread(fp, &temp, sizeof(float)));
  if (is_big_endian()) {
    byteswap(temp); // Data always written little endian
  }
  return temp;
}

/** Read up to length characters into a std::string */
std::string
readChar(gzFile fp, size_t length)
{
  std::vector<unsigned char> v;

  v.resize(length);
  const size_t bytes = length * sizeof(unsigned char);

  ERRNO(gzread(fp, &v[0], bytes));
  // std::string name(v.begin(), v.end()); includes zeros
  std::string name;

  for (size_t i = 0; i < bytes; i++) {
    if (v[i] == 0) { break; }
    name += v[i];
  }

  return name;
}
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
    const unsigned int firstYear = (v[0] | (v[1] << 8) | (v[2] << 16) | (v[3] << 24)); // little endian
    const bool validYear         = isMRMSValidYear(firstYear);

    // --------------------------------------------------------------------------
    // Factory
    if (validASCII) {
      LogInfo("HMRG reader: " << url << " (Guess: MRMS Polar Binary)\n");
      std::string radarName(v.begin(), v.end());
      dataout = readRadialSet(fp, radarName, true);
    } else if (validYear) {
      gzclose(fp);
      return nullptr;

      // Try Gridded binary at this point.  ASCII seems good
      LogInfo("HMRG reader: " << url << " (Guess: MRMS Gridded Binary)\n");
      IOHmrgGriddedHeader h;
      h.year  = firstYear;
      dataout = readGriddedType(fp, h, true);
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
IOHmrg::readRadialSet(gzFile fp, const std::string& radarName, bool debug)
{
  // name passed in was used to check type
  const int headerScale    = readInt(fp);   // 5-8
  const int radarMSLmeters = readInt(fp);   // 9-12
  const float radarLatDegs = readFloat(fp); // 13-16
  const float radarLonDegs = readFloat(fp); // 17-20

  // Time
  const int year  = readInt(fp); // 21-24
  const int month = readInt(fp); // 25-28
  const int day   = readInt(fp); // 29-32
  const int hour  = readInt(fp); // 33-36
  const int min   = readInt(fp); // 37-40
  const int sec   = readInt(fp); // 41-44

  const float nyquest = readScaledInt(fp, headerScale); // 45-48
  const int vcp       = readInt(fp);                    // 49-52

  const int tiltNumber      = readInt(fp);                    // 53-56
  const float elevAngleDegs = readScaledInt(fp, headerScale); // 57-60

  const int num_radials = readInt(fp); // 61-64
  const int num_gates   = readInt(fp); // 65-68

  const float firstAzimuthDegs = readScaledInt(fp, headerScale);          // 69-72
  const float azimuthResDegs   = readScaledInt(fp, headerScale);          // 73-76
  const float distanceToFirstGateMeters = readScaledInt(fp, headerScale); // 77-80
  const float gateSpacingMeters         = readScaledInt(fp, headerScale); // 81-84

  std::string name = readChar(fp, 20); // 85-104

  // TODO: Lookup table .dat looks like...next pass
  if (name == "REFL") {
    name = "Reflectivity";
  }
  const std::string units = readChar(fp, 6); // 105-110

  const int dataScale        = readInt(fp);
  const int dataMissingValue = readInt(fp);

  // The placeholder.. 8 ints
  const std::string placeholder = readChar(fp, 8 * sizeof(int)); // 119-150

  // RAPIO types
  Time dataTime(year, month, day, hour, min, sec, 0.0);
  LLH center(radarLatDegs, radarLonDegs, radarMSLmeters); // FIXME: check height MSL/ASL same as expected, easy to goof this I think

  if (debug) {
    LogInfo("   Scale is " << headerScale << "\n");
    LogInfo("   Radar Center: " << radarName << " centered at (" << radarLatDegs << ", " << radarLonDegs << ")\n");
    LogInfo("   Date: " << year << " " << month << " " << day << " " << hour << " " << min << " " << sec << "\n");
    LogInfo("   Time: " << dataTime << "\n");
    LogInfo(
      "   Nyquist, VCP, tiltNumber, elevAngle:" << nyquest << ", " << vcp << ", " << tiltNumber << ", " << elevAngleDegs <<
        "\n");
    LogInfo("   Dimensions: " << num_radials << " radials, " << num_gates << " gates\n");
    LogInfo("   FirstAzDegs, AzRes, distFirstGate, gateSpacing: " << firstAzimuthDegs << ", " << azimuthResDegs << ", " << distanceToFirstGateMeters << ", "
                                                                  << gateSpacingMeters << "\n");
    LogInfo("   Name and units: '" << name << "' and '" << units << "'\n");
    LogInfo("   Data scale and missing value: " << dataScale << " and " << dataMissingValue << "\n");
  }

  auto d = RadialSet::Create(name, units, center, dataTime, elevAngleDegs, distanceToFirstGateMeters,
      num_radials, num_gates);

  d->setReadFactory("netcdf"); // Default would call us to write

  // FIXME: fill in the data I think...lol

  return d;
} // IOHmrg::readRadialSet

std::shared_ptr<DataType>
IOHmrg::readGriddedType(gzFile fp, IOHmrgGriddedHeader& h, bool debug)
{
  // Timestamp
  // h.year passed in;    // 1-4
  h.month = readInt(fp); // 5-8
  h.day   = readInt(fp); // 9-12
  h.hour  = readInt(fp); // 13-16
  h.min   = readInt(fp); // 17-20
  h.sec   = readInt(fp); // 21-24

  // RAPIO time handles this
  Time t(h.year, h.month, h.day, h.hour, h.min, h.sec, 0.0);

  // Dimensions
  h.num_x = readInt(fp);
  h.num_y = readInt(fp);
  h.num_z = readInt(fp);

  if (debug) {
    LogInfo(
      "   Date: " << h.year << " " << h.month << " " << h.day << " " << h.hour << " " << h.min << " " << h.sec << "\n");
    LogInfo("   Time: " << t << "\n");
    LogInfo("   Dimensions: " << h.num_x << " " << h.num_y << " " << h.num_z << "\n");
  }

  // LatLonGrid or LatLonHeightGrid?
  // I could see 'some' situations where we'd want to force a LatLonHeightGrid of '1',
  // we can add settings in the xml later for this...
  if (h.num_z == 1) {
    LogInfo("   Z dimension of 1, creating LatLonGrid by default.\n");
    return readLatLonGrid(fp, h, debug);
  } else {
    LogInfo("   Z dimension of " << h.num_z << " creating LatLonHeightGrid..\n");
    return readLatLonHeightGrid(fp, h, debug);
  }
  return nullptr;
} // IOHmrg::readGriddedType

std::shared_ptr<DataType>
IOHmrg::readLatLonGrid(gzFile fp, IOHmrgGriddedHeader& h, bool debug)
{
  h.projection = readChar(fp, 4);
  // Perhaps LL is the only one actually used, we can warn on others though
  // "    " proj1=0;
  // "PS  " proj1=1;
  // "LAMB" proj1=2;
  // "MERC" proj1=3;
  // "LL  " proj1=4;

  h.map_scale = readInt(fp);
  h.lat1      = readScaledInt(fp, h.map_scale);
  h.lat2      = readScaledInt(fp, h.map_scale);
  h.lon       = readScaledInt(fp, h.map_scale);
  h.lonnw     = readScaledInt(fp, h.map_scale);
  h.center    = readScaledInt(fp, h.map_scale);

  // lol, of course the scale is stored 'after' the scaled values
  // So we'll manually scale
  const int depScale        = readInt(fp);
  const int gridCellLonDegs = readInt(fp);
  const int gridCellLatDegs = readInt(fp);
  const int dxy_scale       = readInt(fp);

  // This part tricky/confusing guessing here.  1 z should give you 1 right?
  std::vector<int> levels;

  levels.resize(h.num_z);
  gzread(fp, &levels[0], h.num_z * sizeof(int));

  const int z_scale = readInt(fp);
  std::vector<int> placeholder;

  placeholder.resize(10);
  gzread(fp, &placeholder[0], 10 * sizeof(int));

  std::string varName = readChar(fp, 20);
  std::string varUnit = readChar(fp, 6);

  // Scale for scaling data values
  int varScale = readInt(fp);
  int missing  = readInt(fp);

  int numRadars = readInt(fp);
  std::vector<std::string> radars;

  for (size_t i = 0; i < numRadars; i++) {
    std::string r = readChar(fp, 4);
    radars.push_back(r);
    LogInfo("Got radar '" << r << "'\n");
  }

  if (debug) {
    LogInfo("   Projection: '" << h.projection << "'\n"); // always "LL  "?
    LogInfo("   Lat, lat2, lon: " << h.lat1 << ", " << h.lat2 << ", " << h.lon << "\n");
    LogInfo("   VarNameUnit: " << varName << ", " << varUnit << "\n");
    LogInfo("   VarScale/Missing: " << varScale << " " << missing << "\n");
  }

  // Create a LatLonGrid using the data, right?
  // std::shared_ptr<LatLonGrid> LatLonGridSP = std::make_shared<LatLonGrid>();
  return nullptr;
} // IOHmrg::readLatLonGrid

std::shared_ptr<DataType>
IOHmrg::readLatLonHeightGrid(gzFile fp, IOHmrgGriddedHeader& h, bool debug)
{
  LogSevere(" TODO: Implement LatLonHeightGrid class.\n");
  return nullptr;
}

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
