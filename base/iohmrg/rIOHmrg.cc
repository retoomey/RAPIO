#include "rIOHmrg.h"

#include "rIOURL.h"
#include "rRadialSet.h"
#include "rLatLonGrid.h"

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
  std::string name;

  // Use zero to finish std::string
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
    unsigned int firstYear = (v[0] | (v[1] << 8) | (v[2] << 16) | (v[3] << 24)); // little endian
    if (is_big_endian()) { byteswap(firstYear); }
    const bool validYear = isMRMSValidYear(firstYear);

    // --------------------------------------------------------------------------
    // Factory
    if (validASCII) {
      LogInfo("HMRG reader: " << url << " (Guess: MRMS Polar Binary)\n");
      std::string radarName(v.begin(), v.end());
      dataout = readRadialSet(fp, radarName, true);
    } else if (validYear) {
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
  const int headerScale      = readInt(fp);                    // 5-8
  const float radarMSLmeters = readScaledInt(fp, headerScale); // 9-12
  const float radarLatDegs   = readFloat(fp);                  // 13-16
  const float radarLonDegs   = readFloat(fp);                  // 17-20

  // Time
  const int year  = readInt(fp); // 21-24
  const int month = readInt(fp); // 25-28
  const int day   = readInt(fp); // 29-32
  const int hour  = readInt(fp); // 33-36
  const int min   = readInt(fp); // 37-40
  const int sec   = readInt(fp); // 41-44

  const float nyquest = readScaledInt(fp, headerScale); // 45-48  // FIXME: Volume number?
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

  int dataScale = readInt(fp);

  if (dataScale == 0) {
    LogSevere("Data scale in hmrg is zero, forcing to 1.  Is data corrupt?\n");
    dataScale = 1;
  }
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

  // Read the data right?  Buffer the short ints we'll have to unscale them into the netcdf
  // We'll handle endian swapping in the convert loop.  So these shorts 'could' be flipped if
  // we're on a big endian machine
  // Order is radial major, all gates for a single radial/ray first.  That's great since
  // that's our RAPIO/W2 order.
  int count = num_radials * num_gates;
  std::vector<short int> rawBuffer;

  rawBuffer.resize(count);
  ERRNO(gzread(fp, &rawBuffer[0], count * sizeof(short int))); // should be 2 bytes, little endian order

  auto radialSetSP = RadialSet::Create(name, units, center, dataTime, elevAngleDegs, distanceToFirstGateMeters,
      num_radials, num_gates);
  RadialSet& radialSet = *radialSetSP;

  radialSet.setReadFactory("netcdf"); // Default would call us to write

  auto azimuthsA   = radialSet.getFloat1D("Azimuth");
  auto& azimuths   = azimuthsA->ref();
  auto beamwidthsA = radialSet.getFloat1D("BeamWidth");
  auto& beamwidths = beamwidthsA->ref();
  auto gatewidthsA = radialSet.getFloat1D("GateWidth");
  auto& gatewidths = gatewidthsA->ref();

  auto array = radialSet.getFloat2D("primary");
  auto& data = array->ref();

  const bool needSwap = is_big_endian(); // data is little
  // size_t countm = 0;
  size_t rawBufferIndex = 0;

  // Think using the missing scaled up will help prevent float drift here
  // and avoid some divisions in loop
  const int dataMissing     = dataMissingValue * dataScale;
  const int dataUnavailable = -9990; // FIXME: table lookup * dataScale;

  for (size_t i = 0; i < num_radials; ++i) {
    float start_az = i; // Each degree

    // We could add each time but that might accumulate drift error
    // Adding would be faster.  Does it matter?
    azimuths[i]   = std::fmod(firstAzimuthDegs + (i * azimuthResDegs), 360);
    beamwidths[i] = 1; // Correct?
    gatewidths[i] = gateSpacingMeters;
    for (size_t j = 0; j < num_gates; ++j) {
      short int v = rawBuffer[rawBufferIndex++];
      if (needSwap) { byteswap(v); }
      if (v == dataUnavailable) {
        data[i][j] = Constants::DataUnavailable;
      } else if (v == dataMissing) {
        data[i][j] = Constants::MissingData;
        // countm++;
      } else {
        data[i][j] = (float) v / (float) dataScale;
      }
    }
  }
  // LogInfo("    Found " << countm << " missing values\n");

  return radialSetSP;
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
  h.num_x = readInt(fp); // 25-28
  h.num_y = readInt(fp); // 29-32
  h.num_z = readInt(fp); // 33-36

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
  h.projection = readChar(fp, 4); // 37-40
  // Perhaps LL is the only one actually used, we can warn on others though
  // "    " proj1=0;
  // "PS  " proj1=1;
  // "LAMB" proj1=2;
  // "MERC" proj1=3;
  // "LL  " proj1=4;

  h.map_scale = readInt(fp);                    // 41-44
  h.lat1      = readScaledInt(fp, h.map_scale); // 45-48
  h.lat2      = readScaledInt(fp, h.map_scale); // 49-52
  h.lon       = readScaledInt(fp, h.map_scale); // 53-56
  h.lonNWDegs = readScaledInt(fp, h.map_scale); // 57-60
  h.latNWDegs = readScaledInt(fp, h.map_scale); // 61-64

  // Manually scale since scale after the values
  const int xy_scale         = readInt(fp); // 65-68 Deprecated, used anywhere?
  const int temp1            = readInt(fp); // 69-72
  const int temp2            = readInt(fp); // 73-76
  const int dxy_scale        = readInt(fp); // 77-80
  const float lonSpacingDegs = (float) temp1 / (float) dxy_scale;
  const float latSpacingDegs = (float) temp2 / (float) dxy_scale;

  // Read the height levels, scaled by Z_scale
  std::vector<int> levels;

  levels.resize(h.num_z);
  gzread(fp, &levels[0], h.num_z * sizeof(int));

  // scale is after data again, but we deal with it...
  const int z_scale = readInt(fp);
  std::vector<float> heightMeters;

  heightMeters.resize(h.num_z);
  for (size_t i = 0; i < levels.size(); i++) {
    // This 'appears' to be height in meters
    heightMeters[i] = (float) levels[i] / (float) (z_scale);
  }

  std::vector<int> placeholder;

  placeholder.resize(10);
  gzread(fp, &placeholder[0], 10 * sizeof(int));

  // Get the variable name and units
  std::string varName = readChar(fp, 20);
  std::string varUnit = readChar(fp, 6);

  if (varName == "CREF") { // example use Reflectivy colormap, etc. FIXME: make table
    varName = "MergedReflectivityComposite";
  }

  // Common code here with Radial
  // Scale for scaling data values
  int dataScale = readInt(fp);

  if (dataScale == 0) {
    LogSevere("Data scale in hmrg is zero, forcing to 1.  Is data corrupt?\n");
    dataScale = 1;
  }

  const int dataMissingValue = readInt(fp);

  int numRadars = readInt(fp);
  std::vector<std::string> radars;

  for (size_t i = 0; i < numRadars; i++) {
    std::string r = readChar(fp, 4);
    radars.push_back(r);
    LogInfo("   Got radar '" << r << "'\n");
  }

  // Common code here with Radial
  int count = h.num_x * h.num_y;
  std::vector<short int> rawBuffer;

  rawBuffer.resize(count);
  ERRNO(gzread(fp, &rawBuffer[0], count * sizeof(short int))); // should be 2 bytes, little endian order

  if (debug) {
    LogInfo("   Projection: '" << h.projection << "'\n"); // always "LL  "?
    LogInfo("   Lat, lat2, lon: " << h.lat1 << ", " << h.lat2 << ", " << h.lon << "\n");
    LogInfo("   VarNameUnit: " << varName << ", " << varUnit << "\n");
    LogInfo("   VarScale/Missing: " << dataScale << " " << dataMissingValue << "\n");
    if (heightMeters.size() > 0) {
      LogInfo("  OK height Z at 0 is " << heightMeters[0] << "\n");
      LogInfo("  Spacing is " << latSpacingDegs << ", " << lonSpacingDegs << "\n");
    }
  }

  Time time(h.year, h.month, h.day, h.hour, h.min, h.sec, 0.0);

  // HMRG uses center of cell for northwest corner, while we use the actual northwest corner
  // or actual top left of the grid cell.  We need to adjust using lat/lon spacing...so
  // that 'should' be just shifting NorthWest by half of the deg spacing, which means
  //  W2/RAPIO lon = + (.5*HMRG lon), W2/RAPIO lat = - (.5*HMRG lat)
  h.lonNWDegs + .5 * (lonSpacingDegs);
  h.latNWDegs - .5 * (latSpacingDegs);
  LLH location(h.latNWDegs, h.lonNWDegs, heightMeters[0] / 1000.0); // takes kilometers...

  // Create a LatLonGrid using the data
  auto latLonGridSP = LatLonGrid::Create(
    varName,
    varUnit,
    location,
    time,
    latSpacingDegs,
    lonSpacingDegs,
    h.num_y, // num_lats
    h.num_x  // num_lons
  );

  LatLonGrid& grid = *latLonGridSP;

  grid.setReadFactory("netcdf"); // Default would call us to write

  // Fill in the data here
  const bool needSwap = is_big_endian(); // data is little
  // size_t countm = 0;
  size_t rawBufferIndex = 0;

  // Think using the missing scaled up will help prevent float drift here
  // and avoid some divisions in loop
  const int dataMissing     = dataMissingValue * dataScale;
  const int dataUnavailable = -9990; // FIXME: table lookup * dataScale;

  // Grid 2d primary
  auto array = grid.getFloat2D("primary");
  auto& data = array->ref();

  // NOTE: flipped order from RadialSet array if you try to merge the code
  for (size_t j = 0; j < h.num_y; ++j) {
    const size_t jflip = h.num_y - j - 1;
    for (size_t i = 0; i < h.num_x; ++i) {       // row order for the data, so read in order
      short int v = rawBuffer[rawBufferIndex++]; // we could improve speed here and the swap
      if (needSwap) { byteswap(v); }
      if (v == dataUnavailable) {
        data[jflip][i] = Constants::DataUnavailable;
      } else if (v == dataMissing) {
        data[jflip][i] = Constants::MissingData;
        // countm++;
      } else {
        data[jflip][i] = (float) v / (float) dataScale;
      }
    }
  }
  // LogInfo("    Found " << countm << " missing values\n");

  return latLonGridSP;
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
