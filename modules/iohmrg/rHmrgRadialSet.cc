#include "rHmrgRadialSet.h"

#include "rOS.h"
#include "rRadialSet.h"
#include "rBinaryIO.h"

using namespace rapio;

void
HmrgRadialSet::introduceSelf(IOHmrg * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<HmrgRadialSet>();

  // DataTypes we handle
  owner->introduce("RadialSet", io);
  owner->introduce("PolarGrid", io);
  owner->introduce("SparseRadialSet", io);
  owner->introduce("SparsePolarGrid", io);
}

std::shared_ptr<DataType>
HmrgRadialSet::read(
  std::map<std::string, std::string>& keys,
  std::shared_ptr<DataType>         dt)
{
  gzFile fp = IOHmrg::keyToGZFile(keys);

  if (fp != nullptr) {
    std::string radarName = keys["RadarName"];
    return readRadialSet(fp, radarName, true);
  } else {
    LogSevere("Invalid gzfile pointer, cannot read\n");
  }
  return nullptr;
}

bool
HmrgRadialSet::write(
  std::shared_ptr<DataType>         dt,
  std::map<std::string, std::string>& keys)
{
  bool success = false;
  gzFile fp    = IOHmrg::keyToGZFile(keys);

  if (fp != nullptr) {
    auto radialSet = std::dynamic_pointer_cast<RadialSet>(dt);
    if (radialSet != nullptr) {
      success = writeRadialSet(fp, radialSet);
    }
  } else {
    LogSevere("Invalid gzfile pointer, cannot write\n");
  }

  return success;
}

std::shared_ptr<DataType>
HmrgRadialSet::readRadialSet(gzFile fp, const std::string& radarName, bool debug)
{
  GzipFileStreamBuffer g(fp);
  g.setDataLittleEndian();

  // name passed in was used to check type
  const int headerScale      = g.readInt();                    // 5-8
  const float radarMSLmeters = g.readScaledInt(headerScale);   // 9-12
  const float radarLatDegs   = g.readFloat();                  // 13-16
  const float radarLonDegs   = g.readFloat();                  // 17-20

  #if 0
  // Time
  const int year  = IOHmrg::readInt(fp); // 21-24
  const int month = IOHmrg::readInt(fp); // 25-28
  const int day   = IOHmrg::readInt(fp); // 29-32
  const int hour  = IOHmrg::readInt(fp); // 33-36
  const int min   = IOHmrg::readInt(fp); // 37-40
  const int sec   = IOHmrg::readInt(fp); // 41-44
  #endif
  Time dataTime = g.readTime();

  const float nyquest = g.readScaledInt(headerScale); // 45-48  // FIXME: Volume number?
  const int vcp       = g.readInt();                    // 49-52

  const int tiltNumber      = g.readInt();                    // 53-56
  const float elevAngleDegs = g.readScaledInt(headerScale); // 57-60

  const int num_radials = g.readInt(); // 61-64
  const int num_gates   = g.readInt(); // 65-68

  const float firstAzimuthDegs = g.readScaledInt(headerScale);          // 69-72
  const float azimuthResDegs   = g.readScaledInt(headerScale);          // 73-76
  const float distanceToFirstGateMeters = g.readScaledInt(headerScale); // 77-80
  const float gateSpacingMeters         = g.readScaledInt(headerScale); // 81-84

  std::string name = g.readString(20); // 85-104

  // Convert HMRG names etc to w2 expected
  std::string orgName = name;
  IOHmrg::HmrgToW2Name(name, name);
  LogInfo("Convert: " << orgName << " to " << name << "\n");

  const std::string units = g.readString(6); // 105-110

  int dataScale = g.readInt();

  if (dataScale == 0) {
    LogSevere("Data scale in hmrg is zero, forcing to 1.  Is data corrupt?\n");
    dataScale = 1;
  }
  const int dataMissingValue = g.readInt();

  // The placeholder.. 8 ints
  const std::string placeholder = g.readString(8 * sizeof(int)); // 119-150

  // RAPIO types
  LLH center(radarLatDegs, radarLonDegs, radarMSLmeters); // FIXME: check height MSL/ASL same as expected, easy to goof this I think

  if (debug) {
    LogDebug("   Radar Center: " << radarName << " centered at (" << radarLatDegs << ", " << radarLonDegs << ")\n");
    LogDebug("   Scale is " << headerScale << "\n");
    LogDebug("   Radar Center: " << radarName << " centered at (" << radarLatDegs << ", " << radarLonDegs << ")\n");
    LogDebug("   Date: " << dataTime.getString("%Y %m %d %H %M %S") << "\n");
    LogDebug("   Time: " << dataTime << "\n");
    LogDebug(
      "   Nyquist, VCP, tiltNumber, elevAngle:" << nyquest << ", " << vcp << ", " << tiltNumber << ", " << elevAngleDegs <<
        "\n");
    LogDebug("   Dimensions: " << num_radials << " radials, " << num_gates << " gates\n");
    LogDebug("   FirstAzDegs, AzRes, distFirstGate, gateSpacing: " << firstAzimuthDegs << ", " << azimuthResDegs << ", " << distanceToFirstGateMeters << ", "
                                                                   << gateSpacingMeters << "\n");
    LogDebug("   Name and units: '" << name << "' and '" << units << "'\n");
    LogDebug("   Data scale and missing value: " << dataScale << " and " << dataMissingValue << "\n");
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
      gateSpacingMeters, num_radials, num_gates);
  RadialSet& radialSet = *radialSetSP;

  radialSet.setUnits(units);
  radialSet.setDataAttributeValue("radarName", radarName, "dimensionless"); // FIXME: API setRadarName
  std::stringstream vcpss;

  vcpss << vcp;
  radialSet.setDataAttributeValue("vcp", vcpss.str(), "dimensionless"); // FIXME: API setVCP

  radialSet.setReadFactory("netcdf"); // Default would call us to write

  auto azimuthsA   = radialSet.getFloat1D(RadialSet::Azimuth);
  auto& azimuths   = azimuthsA->ref();
  auto beamwidthsA = radialSet.getFloat1D(RadialSet::BeamWidth);
  auto& beamwidths = beamwidthsA->ref();
  //auto gatewidthsA = radialSet.getFloat1D(RadialSet::GateWidth);
  //auto& gatewidths = gatewidthsA->ref();

  auto array = radialSet.getFloat2D(Constants::PrimaryDataName);
  auto& data = array->ref();

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
    //gatewidths[i] = gateSpacingMeters;
    for (size_t j = 0; j < num_gates; ++j) {
      auto old = rawBuffer[rawBufferIndex];
      data[i][j] = IOHmrg::fromHmrgValue(rawBuffer[rawBufferIndex++], dataUnavailable, dataMissing,
          dataScale);
      #if 0
      if (i == 0) {
        if (j < 20) {
          std::cout << old << " --> " << data[i][j] << "\n";
        }
      }
      #endif
    }
  }
  // LogInfo("    Found " << countm << " missing values\n");

  return radialSetSP;
} // IOHmrg::readRadialSet

bool
HmrgRadialSet::writeRadialSet(gzFile fp, std::shared_ptr<RadialSet> radialsetp)
{
  bool success = false;

  // HMRG radial sets have to have a starting azimuth and a delta that works for
  // all azimuths
  auto& radialset        = *radialsetp;
  const auto num_radials = radialset.getNumRadials();
  const auto num_gates   = radialset.getNumGates();
  auto& azimuth = radialset.getFloat1DRef(RadialSet::Azimuth);

  // for (size_t i = 0; i < num_radials; ++i) {
  //   std::cout << azimuth[i] << ",";
  // }

  // ----------------------------------------------------------------------------
  // Check all angles between radials are the same (FIXME: function)
  //
  // HMRG requires starting azimuth angle and a single delta addition for each,
  // so if our RadialSet violates this, we can't write it. Scan and check
  int oldDeltaDeg        = 0;
  float firstAzimuthDegs = 0;

  for (size_t i = 0; i < num_radials; ++i) {
    auto curAzimuth = std::fmod(azimuth[i], 360);
    if (i == 0) { firstAzimuthDegs = curAzimuth; continue; }
    auto prevAzimuth = std::fmod(azimuth[i - 1], 360);
    int newDeltaDeg  = curAzimuth - prevAzimuth;
    if (newDeltaDeg < 0) { newDeltaDeg += 360; }
    if (i == 1) { oldDeltaDeg = newDeltaDeg; continue; }

    // Check azimuth spacing is uniform for entire RadialSet
    if (newDeltaDeg != oldDeltaDeg) {
      LogSevere("RadialSet azimuth  " << curAzimuth << " and " << prevAzimuth << "\n");
      LogSevere(
        "RadialSet azimuth degree gap is not uniform: " << newDeltaDeg << " != " << oldDeltaDeg <<
          ", we can't write this.\n");
      return false;
    }
  }
  const float azimuthResDegs = oldDeltaDeg;

  // ----------------------------------------------------------------------------
  // Check all gatewidths are the same (FIXME: function)
  auto& gatewidth = radialset.getFloat1DRef(RadialSet::GateWidth);
  float gateW     = 0;

  for (size_t i = 0; i < num_radials; ++i) {
    if (i == 0) { gateW = gatewidth[i]; continue; }
    if (gatewidth[i] != gateW) {
      LogSevere(
        "RadialSet gatewidth gap is not uniform: " << gatewidth[i] << " != " << gateW << ", we can't write this.\n");
      return false;
    }
  }
  // ----------------------------------------------------------------------------
  const float gateSpacingMeters = gateW;

  // FIXME: From the data, etc.
  const int headerScale = 100;

  std::string radarName = "NONE";

  radialset.getString("radarName-value", radarName); // FIXME: API getRadarName I think for clarity
  std::string typeName = radialset.getTypeName();

  // With the writer we want the radar name.
  ERRNO(gzwrite(fp, &radarName[0], 4 * sizeof(unsigned char)));

  // name passed in was used to check type
  BinaryIO::writeInt(fp, headerScale);

  auto loc = radialset.getLocation();
  auto radarMSLmeters = loc.getHeightKM() * 1000;
  auto radarLatDegs   = loc.getLatitudeDeg();
  auto radarLonDegs   = loc.getLongitudeDeg();

  BinaryIO::writeScaledInt(fp, radarMSLmeters, headerScale); // 9-12
  BinaryIO::writeFloat(fp, radarLatDegs);                    // 13-16
  BinaryIO::writeFloat(fp, radarLonDegs);                    // 17-20

  // Time
  BinaryIO::writeTime(fp, radialset.getTime());

  const float nyquest = 10;

  BinaryIO::writeScaledInt(fp, nyquest, headerScale); // 45-48  // FIXME: Volume number?

  int vcp = -99;
  std::string vcpstr = "";

  radialset.getString("vcp-value", vcpstr); // FIXME: API getVCP I think for clarity
  try{
    vcp = std::stoi(vcpstr);
  }catch (const std::exception&) { }
  BinaryIO::writeInt(fp, vcp); // 49-52

  const int tiltNumber = 1; // FIXME: we could have an optional tiltNumber stored

  BinaryIO::writeInt(fp, tiltNumber); // 53-56

  const float elevAngleDegs = radialset.getElevationDegs();

  BinaryIO::writeScaledInt(fp, elevAngleDegs, headerScale); // 57-60

  // Get data
  auto& data = radialset.getFloat2DRef();

  BinaryIO::writeInt(fp, num_radials); // 61-64
  BinaryIO::writeInt(fp, num_gates);   // 65-68

  BinaryIO::writeScaledInt(fp, firstAzimuthDegs, headerScale); // 69-72
  BinaryIO::writeScaledInt(fp, azimuthResDegs, headerScale);   // 73-76
  const float distanceToFirstGateMeters = radialset.getDistanceToFirstGateM();

  BinaryIO::writeScaledInt(fp, distanceToFirstGateMeters, headerScale); // 77-80

  // const float gateSpacingMeters =  10.0;
  BinaryIO::writeScaledInt(fp, gateSpacingMeters, headerScale); // 81-84

  std::string name = typeName;

  IOHmrg::W2ToHmrgName(name, name);
  LogInfo("Convert: " << typeName << " to " << name << "\n");

  BinaryIO::writeChar(fp, name, 20); // 85-104

  name = radialset.getUnits();
  BinaryIO::writeChar(fp, name, 6); // 105-110

  int dataScale = 10;

  BinaryIO::writeInt(fp, dataScale);

  const int dataMissing = -99;

  BinaryIO::writeInt(fp, dataMissing);

  // The placeholder.. 8 ints
  int fill = 0;

  for (size_t i = 0; i < 8; i++) {
    ERRNO(gzwrite(fp, &fill, sizeof(int)));
  }

  const int count = num_radials * num_gates;
  std::vector<short int> rawBuffer;

  rawBuffer.resize(count);

  // const int dataMissing     = dataMissingValue * dataScale;
  const int dataUnavailable = -9990; // FIXME: table lookup * dataScale;
  size_t at = 0;

  for (size_t i = 0; i < num_radials; ++i) {
    for (size_t j = 0; j < num_gates; ++j) {
      rawBuffer[at++] = IOHmrg::toHmrgValue(data[i][j], dataUnavailable, dataMissing, dataScale);
    }
  }
  ERRNO(gzwrite(fp, &rawBuffer[0], count * sizeof(short int))); // should be 2 bytes, little endian order

  return true;
} // HmrgRadialSet::writeRadialSet
