#include "rHmrgRadialSet.h"

#include "rOS.h"
#include "rRadialSet.h"

using namespace rapio;

std::shared_ptr<DataType>
IOHmrgRadialSet::readRadialSet(gzFile fp, const std::string& radarName, bool debug)
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

  // Convert HMRG names etc to w2 expected
  HmrgToW2(name, name);
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
  const int count = num_radials * num_gates;
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

  auto array = radialSet.getFloat2D(Constants::PrimaryDataName);
  auto& data = array->ref();

  const bool needSwap = OS::isBigEndian(); // data is little
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
      data[i][j] = convertDataValue(rawBuffer[rawBufferIndex++], needSwap, dataUnavailable, dataMissing, dataScale);
    }
  }
  // LogInfo("    Found " << countm << " missing values\n");

  return radialSetSP;
} // IOHmrg::readRadialSet
