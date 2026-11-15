/**
 * @file rTDWRAccumulator.cc
 * @brief TDWR sweep accumulator implementation.
 */

#include "rTDWRAccumulator.h"
#include <rConstants.h>
#include <rError.h>
#include <rStrings.h>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace rapio {
TDWRAccumulator::TDWRAccumulator()
{
  reset();
}

TDWRAccumulator::~TDWRAccumulator()
{ }

void
TDWRAccumulator::reset()
{
  myCurrentScanNumber    = -1;
  myCurrentScanStrategy  = -1;
  myCurrentElevation     = -999.0f;
  myCurrentStartOfVolume = false;
  myCurrentEndOfVolume   = false;
  myCurrentCompressedRadials.clear();
  myCurrentLowPRFRadials.clear();
  myCompletedRadialSets.clear();
  myHaveVolumeStartTime = false;
  myStartOfVolumeIsSet  = false;
}

void
TDWRAccumulator::setMagneticOffset(float offsetDegs)
{
  myMagneticOffset       = offsetDegs;
  myHaveExplicitMagnetic = true;
}

void
TDWRAccumulator::setMagneticOffset(const std::string& offsetStr)
{
  if (offsetStr.empty()) {
    myHaveExplicitMagnetic = false;
    myMagneticOffset       = 0.0f;
    return;
  }
  myMagneticOffset       = parseMagneticOffsetString(offsetStr);
  myHaveExplicitMagnetic = true;
}

float
TDWRAccumulator::getMagneticOffset() const
{
  if (myHaveExplicitMagnetic) {
    return myMagneticOffset;
  }
  if (!myRadarName.empty() && ConfigRadarInfo::haveRadar(myRadarName)) {
    return parseMagneticOffsetString(ConfigRadarInfo::getMagneticOffset(myRadarName));
  }
  return myMagneticOffset;
}

void
TDWRAccumulator::setFrequency(int freqMHz)
{
  myFrequency = freqMHz;
  myHaveExplicitFrequency = true;
}

int
TDWRAccumulator::getFrequency() const
{
  if (myHaveExplicitFrequency) {
    return myFrequency;
  }
  if (!myRadarName.empty() && ConfigRadarInfo::haveRadar(myRadarName)) {
    int freq = ConfigRadarInfo::getFrequency(myRadarName);
    if (freq > 0) {
      return freq;
    }
  }
  return myFrequency;
}

float
TDWRAccumulator::parseMagneticOffsetString(const std::string& str)
{
  if (str.empty()) {
    return 0.0f;
  }

  std::string s = str;

  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
    s.erase(s.begin());
  }
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
    s.pop_back();
  }
  if (s.empty()) {
    return 0.0f;
  }

  char lastChar = static_cast<char>(std::toupper(static_cast<unsigned char>(s.back())));

  if (lastChar == 'W') {
    s.pop_back();
    try {
      float val = std::stof(s);
      return -std::abs(val); // West is negative declination
    } catch (...) {
      return 0.0f;
    }
  } else if (lastChar == 'E') {
    s.pop_back();
    try {
      float val = std::stof(s);
      return std::abs(val); // East is positive declination
    } catch (...) {
      return 0.0f;
    }
  } else {
    try {
      return std::stof(s);
    } catch (...) {
      return 0.0f;
    }
  }
} // TDWRAccumulator::parseMagneticOffsetString

LLH
TDWRAccumulator::resolveRadarLocation()
{
  if (!myRadarName.empty() && ConfigRadarInfo::haveRadar(myRadarName)) {
    return ConfigRadarInfo::getLocation(myRadarName);
  }
  return LLH(0.0, 0.0, 0.0);
}

float
TDWRAccumulator::resolveMagneticOffset()
{
  return getMagneticOffset();
}

int
TDWRAccumulator::resolveFrequency()
{
  return getFrequency();
}

void
TDWRAccumulator::processRadial(const TDWRCompressedRadial& radial)
{
  // Flush any lingering Low PRF radials if stream switched message types
  if (!myCurrentLowPRFRadials.empty()) {
    finishCurrentSweep();
  }

  // Split sweep if scan number changes, scan strategy changes, or elevation changes drastically without EOE/SOE
  if (((myCurrentScanNumber != -1) && (radial.hdr.scan_number != myCurrentScanNumber)) ||
    ((myCurrentScanStrategy != -1) && (radial.hdr.scan_strategy != myCurrentScanStrategy) ) ||
    ((myCurrentElevation > -900.0f) && (std::abs(radial.hdr.current_elevation - myCurrentElevation) > 0.2f) ))
  {
    finishCurrentSweep();
  }

  // Start of Elevation (SOE) trigger
  if (radial.hdr.SOE && !myCurrentCompressedRadials.empty()) {
    finishCurrentSweep();
  }

  // Volume start tracking
  if (radial.hdr.SOV) {
    myVolumeStartTime      = radial.hdr.radial_time;
    myHaveVolumeStartTime  = true;
    myCurrentStartOfVolume = true;
    myStartOfVolumeIsSet   = true;
  } else {
    myStartOfVolumeIsSet = false;
    if (!myHaveVolumeStartTime) {
      myVolumeStartTime      = radial.hdr.radial_time;
      myHaveVolumeStartTime  = true;
      myCurrentStartOfVolume = true;
    }
  }

  if (radial.hdr.EOV) {
    myCurrentEndOfVolume = true;
  }

  myCurrentScanNumber   = radial.hdr.scan_number;
  myCurrentScanStrategy = radial.hdr.scan_strategy;
  myCurrentElevation    = radial.hdr.current_elevation;

  myCurrentCompressedRadials.push_back(radial);

  // End of Elevation (EOE) or End of Volume (EOV) trigger
  if (radial.hdr.EOE || radial.hdr.EOV) {
    finishCurrentSweep();
  }
} // TDWRAccumulator::processRadial

void
TDWRAccumulator::processRadial(const TDWRLowPRFRadial& radial)
{
  // Flush any lingering Compressed radials if stream switched message types
  if (!myCurrentCompressedRadials.empty()) {
    finishCurrentSweep();
  }

  // Split sweep if scan number changes, scan strategy changes, or elevation changes drastically without EOE/SOE
  if (((myCurrentScanNumber != -1) && (radial.hdr.scan_number != myCurrentScanNumber)) ||
    ((myCurrentScanStrategy != -1) && (radial.hdr.scan_strategy != myCurrentScanStrategy) ) ||
    ((myCurrentElevation > -900.0f) && (std::abs(radial.hdr.current_elevation - myCurrentElevation) > 0.2f) ))
  {
    finishCurrentSweep();
  }

  // Start of Elevation (SOE) trigger
  if (radial.hdr.SOE && !myCurrentLowPRFRadials.empty()) {
    finishCurrentSweep();
  }

  // Volume start tracking
  if (radial.hdr.SOV || !myStartOfVolumeIsSet || !myHaveVolumeStartTime) {
    myStartOfVolumeIsSet   = true;
    myCurrentStartOfVolume = true;
    myVolumeStartTime      = radial.hdr.radial_time;
    myHaveVolumeStartTime  = true;
  }

  if (radial.hdr.EOV) {
    myCurrentEndOfVolume = true;
  }

  myCurrentScanNumber   = radial.hdr.scan_number;
  myCurrentScanStrategy = radial.hdr.scan_strategy;
  myCurrentElevation    = radial.hdr.current_elevation;

  myCurrentLowPRFRadials.push_back(radial);

  // End of Elevation (EOE) or End of Volume (EOV) trigger
  if (radial.hdr.EOE || radial.hdr.EOV) {
    finishCurrentSweep();
  }
} // TDWRAccumulator::processRadial

void
TDWRAccumulator::flush()
{
  if (!myCurrentCompressedRadials.empty() || !myCurrentLowPRFRadials.empty()) {
    finishCurrentSweep();
  }
}

void
TDWRAccumulator::finishCurrentSweep()
{
  if (myCurrentCompressedRadials.empty() && myCurrentLowPRFRadials.empty()) {
    return;
  }

  std::vector<std::shared_ptr<DataType> > sweepOutputs;

  if (!myCurrentCompressedRadials.empty()) {
    auto sets = buildCompressedRadialSets();
    for (auto& s : sets) {
      if (s) {
        sweepOutputs.push_back(s);
        if (myKeepCompletedRadialSets) {
          myCompletedRadialSets.push_back(s);
        }
      }
    }
  } else if (!myCurrentLowPRFRadials.empty()) {
    auto sets = buildLowPRFRadialSets();
    for (auto& s : sets) {
      if (s) {
        sweepOutputs.push_back(s);
        if (myKeepCompletedRadialSets) {
          myCompletedRadialSets.push_back(s);
        }
      }
    }
  }

  if (mySweepCallback && !sweepOutputs.empty()) {
    mySweepCallback(sweepOutputs);
  }

  myCurrentCompressedRadials.clear();
  myCurrentLowPRFRadials.clear();
  myCurrentScanNumber    = -1;
  myCurrentScanStrategy  = -1;
  myCurrentElevation     = -999.0f;
  myCurrentStartOfVolume = false;
  myCurrentEndOfVolume   = false;
} // TDWRAccumulator::finishCurrentSweep

std::vector<std::shared_ptr<RadialSet> >
TDWRAccumulator::buildCompressedRadialSets()
{
  std::vector<std::shared_ptr<RadialSet> > result;

  if (myCurrentCompressedRadials.empty()) {
    return result;
  }

  size_t num_radials = myCurrentCompressedRadials.size();
  size_t max_gates   = 0;

  for (const auto& r : myCurrentCompressedRadials) {
    if (r.gates.size() > max_gates) {
      max_gates = r.gates.size();
    }
  }
  if (max_gates == 0) {
    max_gates = TDWRConstants::MAX_COMPRESSED_GATES;
  }

  const auto& firstRad = myCurrentCompressedRadials.front();
  const auto& firstHdr = firstRad.hdr;

  float targetElevDegs = firstHdr.current_elevation;
  float firstGateDist  = 0.0f; // TDWR begins at radar location
  float gateWidth      = TDWRConstants::GATE_SPACING_150M;

  LLH location   = resolveRadarLocation();
  Time sweepTime = firstHdr.radial_time;

  float magOffset = resolveMagneticOffset();
  int freqMHz     = resolveFrequency();

  int vcp = myUserVCP;

  if (vcp <= 0) {
    if ((firstHdr.scan_strategy == 1) || (firstHdr.scan_strategy == 3)) {
      vcp = 90; // MONITOR
    } else if (firstHdr.scan_strategy == 2) {
      vcp = 80; // HAZARDOUS WEATHER
    } else {
      vcp = 80;
    }
  }

  // Moment definitions for Message 0x2b00
  struct MomentSpec {
    const char * typeName;
    const char * units;
  };
  const MomentSpec moments[] = {
    { "Reflectivity",        "dBZ"             },
    { "ConditionedVelocity", "MetersPerSecond" }, // Dealiased velocity
    { "AliasedVelocity",     "MetersPerSecond" }, // Raw Doppler velocity
    { "SpectrumWidth",       "MetersPerSecond" },
    { "SignalToNoise",       "dB"              }
  };

  bool uniformNyquist = true;
  float firstNyq      = (num_radials > 0) ? myCurrentCompressedRadials[0].hdr.getNyquistVelocity(freqMHz) : 0.0f;

  for (size_t r = 1; r < num_radials; ++r) {
    if (std::abs(myCurrentCompressedRadials[r].hdr.getNyquistVelocity(freqMHz) - firstNyq) > 1e-3f) {
      uniformNyquist = false;
      break;
    }
  }

  // Pre-calculate rotated azimuths and beamwidths
  std::vector<float> rotatedAzimuths(num_radials);
  std::vector<float> beamWidths(num_radials, 1.0f);
  std::vector<int> radialTimesMs(num_radials, 0);

  for (size_t r = 0; r < num_radials; ++r) {
    float az = myCurrentCompressedRadials[r].hdr.current_azimuth + magOffset;
    while (az < 0.0f) {
      az += 360.0f;
    }
    while (az >= 360.0f) {
      az -= 360.0f;
    }
    rotatedAzimuths[r] = az;

    radialTimesMs[r] = static_cast<int>(
      (myCurrentCompressedRadials[r].hdr.radial_time - sweepTime).milliseconds());
  }

  for (size_t r = 1; r < num_radials; ++r) {
    float spacing = rotatedAzimuths[r] - rotatedAzimuths[r - 1];
    if (spacing < 0.0f) {
      spacing += 360.0f;
    }
    if ((spacing > 1.5f) || (spacing < 0.01f)) {
      spacing = 1.0f;
    }
    beamWidths[r] = spacing;
  }
  if (num_radials > 1) {
    beamWidths[0] = beamWidths[1];
  } else if (num_radials == 1) {
    beamWidths[0] = 1.0f;
  }

  // Build each moment RadialSet
  for (const auto& m : moments) {
    auto rs = RadialSet::Create(
      m.typeName, m.units, location, sweepTime,
      targetElevDegs, firstGateDist, gateWidth,
      num_radials, max_gates);

    if (!rs) {
      continue;
    }

    ArrayFloat2DPtr outData = rs->getFloat2DPtr();
    ArrayFloat1DPtr azDegs  = rs->getFloat1DPtr(RadialSet::Azimuth);
    ArrayFloat1DPtr bwDegs  = rs->getFloat1DPtr(RadialSet::BeamWidth);
    ArrayFloat1DPtr gwArr   = rs->getFloat1DPtr(RadialSet::GateWidth);

    rs->addFloat1D(RadialSet::AzimuthSpacing, "Degrees", { 0 });
    ArrayFloat1DPtr azSpacing = rs->getFloat1DPtr(RadialSet::AzimuthSpacing);

    ArrayFloat1DPtr nyqArr = nullptr;
    if (!uniformNyquist) {
      rs->addNyquistArray(0.0f);
      nyqArr = rs->getFloat1DPtr(RadialSet::NyquistVelocity);
    }
    rs->addRadialTimeArray(0);
    ArrayInt1DPtr timeArr = rs->getInt1DPtr(RadialSet::RadialTime);

    for (size_t r = 0; r < num_radials; ++r) {
      const auto& rad = myCurrentCompressedRadials[r];

      if (azDegs)    { (*azDegs)[r] = rotatedAzimuths[r]; }
      if (bwDegs)    { (*bwDegs)[r] = beamWidths[r]; }
      if (azSpacing) { (*azSpacing)[r] = beamWidths[r]; }
      if (gwArr)     { (*gwArr)[r] = gateWidth; }
      if (nyqArr)    { (*nyqArr)[r] = rad.hdr.getNyquistVelocity(freqMHz); }
      if (timeArr)   { (*timeArr)[r] = radialTimesMs[r]; }

      size_t numG = rad.gates.size();
      for (size_t g = 0; g < max_gates; ++g) {
        if (g < numG) {
          const auto& gate = rad.gates[g];
          float val        = static_cast<float>(Constants::MissingData);

          if (std::strcmp(m.typeName, "Reflectivity") == 0) {
            // Reflectivity (dBZ): value * 0.5f - 30.0f (applied when CCV == true and CTF == false)
            if (gate.CCV && !gate.CTF) {
              val = static_cast<float>(gate.Z) * 0.5f - 30.0f;
            }
          } else if (std::strcmp(m.typeName, "ConditionedVelocity") == 0) {
            // Dealiased Velocity (m/s): value * 0.25f - 80.0f (applied when CAF == 0 and CVF == true)
            if ((gate.CAF == 0) && gate.CVF) {
              val = static_cast<float>(gate.dealVEL) * 0.25f - 80.0f;
            }
          } else if (std::strcmp(m.typeName, "AliasedVelocity") == 0) {
            // Raw Doppler Velocity (m/s): value * 0.25f - 80.0f (applied when CVF == true)
            if (gate.CVF) {
              val = static_cast<float>(gate.VEL) * 0.25f - 80.0f;
            }
          } else if (std::strcmp(m.typeName, "SpectrumWidth") == 0) {
            // Spectrum Width (m/s): value * 0.25f (applied when CVF == true)
            if (gate.CVF) {
              val = static_cast<float>(gate.W) * 0.25f;
            }
          } else if (std::strcmp(m.typeName, "SignalToNoise") == 0) {
            // SNR (dB): value * 0.5f (applied when CCV == true and CTF == false)
            if (gate.CCV && !gate.CTF) {
              val = static_cast<float>(gate.SNR) * 0.5f;
            }
          }

          (*outData)[r][g] = val;
        } else {
          (*outData)[r][g] = static_cast<float>(Constants::MissingData);
        }
      }
    }

    // Set RadialSet attributes matching legacy
    rs->setRadarName(myRadarName);
    rs->setVCP(vcp);
    rs->setElevationDegs(targetElevDegs);

    rs->setString("radarName", myRadarName);
    if (uniformNyquist) {
      rs->setGlobalNyquistVelocity(firstNyq);
    }
    rs->setString("Unit", m.units);
    std::string scanNoStr = std::to_string(firstHdr.scan_number);
    rs->setString("scanNo", scanNoStr);
    rs->setDataAttributeValue("scanNo", scanNoStr, "dimensionless");
    std::string scanStratStr = std::to_string(firstHdr.scan_strategy);
    rs->setString("scan_strategy", scanStratStr);
    rs->setDataAttributeValue("scan_strategy", scanStratStr, "dimensionless");
    std::string sovStr = myCurrentStartOfVolume ? "1" : "0";
    rs->setString("SOV", sovStr);
    rs->setDataAttributeValue("SOV", sovStr, "dimensionless");
    std::string eovStr = myCurrentEndOfVolume ? "true" : "false";
    rs->setString("EndOfVolume", eovStr);
    rs->setDataAttributeValue("EndOfVolume", eovStr, "text");

    if (myUseVolumeTime && myHaveVolumeStartTime) {
      std::string volTimeStr = myVolumeStartTime.getString("%Y%m%d-%H%M%S");
      rs->setString(Constants::FilenameDateTime, volTimeStr);
      rs->setDataAttributeValue(Constants::FilenameDateTime, volTimeStr, "dimensionless");
    }

    if (myAlignmentResolution > 0.0f) {
      size_t targetRadials = static_cast<size_t>(std::round(360.0f / myAlignmentResolution));
      if (targetRadials > 0) {
        rs = rs->Remap(rs->getGateWidthKMs() * 1000.0f, targetRadials, rs->getNumGates(), false);
        std::string elevStr = DataType::formatString(targetElevDegs, 5, 2);
        rs->setSubType(elevStr);
        rs->setString("SubType", elevStr);
      }
    }

    result.push_back(rs);
  }

  return result;
} // TDWRAccumulator::buildCompressedRadialSets

std::vector<std::shared_ptr<RadialSet> >
TDWRAccumulator::buildLowPRFRadialSets()
{
  std::vector<std::shared_ptr<RadialSet> > result;

  if (myCurrentLowPRFRadials.empty()) {
    return result;
  }

  size_t num_radials   = myCurrentLowPRFRadials.size();
  const auto& firstRad = myCurrentLowPRFRadials.front();
  const auto& firstHdr = firstRad.hdr;

  float targetElevDegs = firstHdr.current_elevation;
  float firstGateDist  = 0.0f;
  float gateWidth      = TDWRConstants::GATE_SPACING_150M; // Expanded to uniform 150m spacing

  // Low PRF expanded gates: 900 inner (150m) + 1084 outer (300m duplicated) = 3068 gates
  size_t totalExpandedGates = TDWRConstants::LOWPRF_EXPANDED_GATES;

  LLH location   = resolveRadarLocation();
  Time sweepTime = firstHdr.radial_time;

  float magOffset = resolveMagneticOffset();
  int freqMHz     = resolveFrequency();

  int vcp = myUserVCP;

  if (vcp <= 0) {
    if ((firstHdr.scan_strategy == 1) || (firstHdr.scan_strategy == 3)) {
      vcp = 90; // MONITOR
    } else if (firstHdr.scan_strategy == 2) {
      vcp = 80; // HAZARDOUS WEATHER
    } else {
      vcp = 80;
    }
  }

  struct LowPRFMomentSpec {
    const char * typeName;
    const char * units;
  };
  const LowPRFMomentSpec moments[] = {
    { "ReflectivityLowPRF",  "dBZ" },
    { "SignalToNoiseLowPRF", "dB"  }
  };

  bool uniformNyquist = true;
  float firstNyq      = (num_radials > 0) ? myCurrentLowPRFRadials[0].hdr.getNyquistVelocity(freqMHz) : 0.0f;

  for (size_t r = 1; r < num_radials; ++r) {
    if (std::abs(myCurrentLowPRFRadials[r].hdr.getNyquistVelocity(freqMHz) - firstNyq) > 1e-3f) {
      uniformNyquist = false;
      break;
    }
  }

  // Pre-calculate rotated azimuths and beamwidths
  std::vector<float> rotatedAzimuths(num_radials);
  std::vector<float> beamWidths(num_radials, 1.0f);
  std::vector<int> radialTimesMs(num_radials, 0);

  for (size_t r = 0; r < num_radials; ++r) {
    float az = myCurrentLowPRFRadials[r].hdr.current_azimuth + magOffset;
    while (az < 0.0f) {
      az += 360.0f;
    }
    while (az >= 360.0f) {
      az -= 360.0f;
    }
    rotatedAzimuths[r] = az;

    radialTimesMs[r] = static_cast<int>(
      (myCurrentLowPRFRadials[r].hdr.radial_time - sweepTime).milliseconds());
  }

  for (size_t r = 1; r < num_radials; ++r) {
    float spacing = rotatedAzimuths[r] - rotatedAzimuths[r - 1];
    if (spacing < 0.0f) {
      spacing += 360.0f;
    }
    if ((spacing > 1.5f) || (spacing < 0.01f)) {
      spacing = 1.0f;
    }
    beamWidths[r] = spacing;
  }
  if (num_radials > 1) {
    beamWidths[0] = beamWidths[1];
  } else if (num_radials == 1) {
    beamWidths[0] = 1.0f;
  }

  for (const auto& m : moments) {
    auto rs = RadialSet::Create(
      m.typeName, m.units, location, sweepTime,
      targetElevDegs, firstGateDist, gateWidth,
      num_radials, totalExpandedGates);

    if (!rs) {
      continue;
    }

    ArrayFloat2DPtr outData = rs->getFloat2DPtr();
    ArrayFloat1DPtr azDegs  = rs->getFloat1DPtr(RadialSet::Azimuth);
    ArrayFloat1DPtr bwDegs  = rs->getFloat1DPtr(RadialSet::BeamWidth);
    ArrayFloat1DPtr gwArr   = rs->getFloat1DPtr(RadialSet::GateWidth);

    rs->addFloat1D(RadialSet::AzimuthSpacing, "Degrees", { 0 });
    ArrayFloat1DPtr azSpacing = rs->getFloat1DPtr(RadialSet::AzimuthSpacing);

    ArrayFloat1DPtr nyqArr = nullptr;
    if (!uniformNyquist) {
      rs->addNyquistArray(0.0f);
      nyqArr = rs->getFloat1DPtr(RadialSet::NyquistVelocity);
    }
    rs->addRadialTimeArray(0);
    ArrayInt1DPtr timeArr = rs->getInt1DPtr(RadialSet::RadialTime);

    for (size_t r = 0; r < num_radials; ++r) {
      const auto& rad = myCurrentLowPRFRadials[r];

      if (azDegs)    { (*azDegs)[r] = rotatedAzimuths[r]; }
      if (bwDegs)    { (*bwDegs)[r] = beamWidths[r]; }
      if (azSpacing) { (*azSpacing)[r] = beamWidths[r]; }
      if (gwArr)     { (*gwArr)[r] = gateWidth; }
      if (nyqArr)    { (*nyqArr)[r] = rad.hdr.getNyquistVelocity(freqMHz); }
      if (timeArr)   { (*timeArr)[r] = radialTimesMs[r]; }

      size_t numZ   = rad.Z.size();
      size_t numSNR = rad.SNR.size();

      // Compute unexpanded scaled arrays
      std::vector<float> unexpanded(numZ, static_cast<float>(Constants::MissingData));

      if (std::strcmp(m.typeName, "ReflectivityLowPRF") == 0) {
        for (size_t g = 0; g < numZ; ++g) {
          float snr_db = (g < numSNR) ? (static_cast<float>(rad.SNR[g]) * 0.5f) : 0.0f;
          if (snr_db > myNoiseThreshold) {
            unexpanded[g] = static_cast<float>(rad.Z[g]) * 0.5f - 30.0f;
          } else {
            unexpanded[g] = static_cast<float>(Constants::MissingData);
          }
        }
      } else { // SignalToNoiseLowPRF
        for (size_t g = 0; g < numSNR; ++g) {
          unexpanded[g] = static_cast<float>(rad.SNR[g]) * 0.5f;
        }
      }

      // Replicate legacy Low PRF Gate Expansion:
      // First 900 gates copied directly (150m spacing)
      // Subsequent 1084 gates (300m spacing) duplicated twice (idx = 900 + 2*i, 900 + 2*i + 1)
      size_t innerGates = std::min(numZ, TDWRConstants::LOWPRF_INNER_GATES);
      for (size_t g = 0; g < innerGates; ++g) {
        (*outData)[r][g] = unexpanded[g];
      }
      for (size_t g = innerGates; g < TDWRConstants::LOWPRF_INNER_GATES; ++g) {
        (*outData)[r][g] = static_cast<float>(Constants::MissingData);
      }

      size_t availableOuter = (numZ > TDWRConstants::LOWPRF_INNER_GATES) ?
        std::min(numZ - TDWRConstants::LOWPRF_INNER_GATES, TDWRConstants::LOWPRF_OUTER_GATES) :
        0;

      for (size_t i = 0; i < availableOuter; ++i) {
        size_t srcIdx = TDWRConstants::LOWPRF_INNER_GATES + i;
        float val     = unexpanded[srcIdx];
        size_t dstIdx = TDWRConstants::LOWPRF_INNER_GATES + i * 2;
        if (dstIdx < totalExpandedGates) {
          (*outData)[r][dstIdx] = val;
        }
        if (dstIdx + 1 < totalExpandedGates) {
          (*outData)[r][dstIdx + 1] = val;
        }
      }

      // Pad remaining if outer gates were truncated
      size_t filledOuter = TDWRConstants::LOWPRF_INNER_GATES + availableOuter * 2;
      for (size_t g = filledOuter; g < totalExpandedGates; ++g) {
        (*outData)[r][g] = static_cast<float>(Constants::MissingData);
      }
    }

    // Set attributes
    rs->setRadarName(myRadarName);
    rs->setVCP(vcp);
    rs->setElevationDegs(targetElevDegs);

    rs->setString("radarName", myRadarName);
    if (uniformNyquist) {
      rs->setGlobalNyquistVelocity(firstNyq);
    }
    rs->setString("Unit", m.units);
    std::string scanNoStr = std::to_string(firstHdr.scan_number);
    rs->setString("scanNo", scanNoStr);
    rs->setDataAttributeValue("scanNo", scanNoStr, "dimensionless");
    std::string scanStratStr = std::to_string(firstHdr.scan_strategy);
    rs->setString("scan_strategy", scanStratStr);
    rs->setDataAttributeValue("scan_strategy", scanStratStr, "dimensionless");
    std::string sovStr = myCurrentStartOfVolume ? "1" : "0";
    rs->setString("SOV", sovStr);
    rs->setDataAttributeValue("SOV", sovStr, "dimensionless");
    std::string eovStr = myCurrentEndOfVolume ? "true" : "false";
    rs->setString("EndOfVolume", eovStr);
    rs->setDataAttributeValue("EndOfVolume", eovStr, "text");

    if (myUseVolumeTime && myHaveVolumeStartTime) {
      std::string volTimeStr = myVolumeStartTime.getString("%Y%m%d-%H%M%S");
      rs->setString(Constants::FilenameDateTime, volTimeStr);
      rs->setDataAttributeValue(Constants::FilenameDateTime, volTimeStr, "dimensionless");
    }

    if (myAlignmentResolution > 0.0f) {
      size_t targetRadials = static_cast<size_t>(std::round(360.0f / myAlignmentResolution));
      if (targetRadials > 0) {
        rs = rs->Remap(rs->getGateWidthKMs() * 1000.0f, targetRadials, rs->getNumGates(), false);
        std::string elevStr = DataType::formatString(targetElevDegs, 5, 2);
        rs->setSubType(elevStr);
        rs->setString("SubType", elevStr);
      }
    }

    result.push_back(rs);
  }

  return result;
} // TDWRAccumulator::buildLowPRFRadialSets
} // namespace rapio
