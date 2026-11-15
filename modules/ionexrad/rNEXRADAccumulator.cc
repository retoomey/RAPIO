#include "rNEXRADAccumulator.h"
#include <rConstants.h>
#include <rError.h>
#include <rStrings.h>
#include <cmath>

namespace rapio {
NEXRADAccumulator::NEXRADAccumulator()
{
  reset();
}

NEXRADAccumulator::~NEXRADAccumulator()
{ }

void
NEXRADAccumulator::reset()
{
  myCurrentSweepNo     = -1;
  myLastWrittenSweepNo = -1;
  myCurrentElevation   = -999.0f;
  myCurrentRadials.clear();
  myCurrentStartOfVolume = false;
  myCurrentEndOfVolume   = false;
  myCompletedRadialSets.clear();
  myPendingDataTables.clear();
  myPendingMsg2.reset();
  myPendingMsg3.reset();
  myHaveVCP = false;
  myHaveVolumeStartTime = false;
  myPrevElevation.clear();
  myPrevSectorSpare.clear();
  myCurrentPRF.clear();
}

void
NEXRADAccumulator::setVCP(const NEXRADMessage5VCP& vcp)
{
  myVCP     = vcp;
  myHaveVCP = true;
}

void
NEXRADAccumulator::processRadial(const NEXRADMessage31Radial& radial)
{
  if ((radial.radar[0] != '\0') && myRadarName.empty()) {
    myRadarName = radial.radar;
  }
  if ((myCurrentSweepNo < 0) &&
    (radial.elevation_no == myLastWrittenSweepNo) &&
    (myLastWrittenSweepNo > 1) &&
    !radial.startOfElev() &&
    !radial.startOfVolume())
  {
    fLogInfo("NEXRADAccumulator: Ignoring late-arriving radial at az={} for already written sweep {}",
      radial.azimuth, static_cast<int>(radial.elevation_no));
    return;
  }

  // Check for sweep transition
  bool isNewSweep = false;

  if (myCurrentSweepNo == -1) {
    isNewSweep = true;
  } else if (radial.elevation_no != myCurrentSweepNo) {
    isNewSweep = true;
  } else if (radial.startOfElev() && !myCurrentRadials.empty()) {
    isNewSweep = true;
  }

  if (isNewSweep && !myCurrentRadials.empty()) {
    finishCurrentSweep();
  }

  myCurrentSweepNo   = radial.elevation_no;
  myCurrentElevation = radial.elevation;
  // Lock in the first available radial time as the volume time if we joined mid-stream
  // without a volume header. This ensures -V keeps all sweeps aligned to one timestamp.
  if (!myHaveVolumeStartTime) {
    myVolumeStartTime      = radial.getTime();
    myHaveVolumeStartTime  = true;
    myCurrentStartOfVolume = true;
  } else if (radial.startOfVolume()) {
    // Standard volume restart
    myVolumeStartTime      = radial.getTime();
    myCurrentStartOfVolume = true;
    myLastWrittenSweepNo   = -1;
    myPrevElevation.clear();
    myPrevSectorSpare.clear();
    myCurrentPRF.clear();
  }
  if (radial.endOfVolume()) {
    myCurrentEndOfVolume = true;
  }

  NEXRADMessage31Radial radCopy = radial;

  if (myHaveVCP) {
    int matchedCutIdx = -1;
    int cutIdx        = static_cast<int>(radCopy.elevation_no) - 1;
    if ((cutIdx >= 0) && (cutIdx < static_cast<int>(myVCP.elevations.size()))) {
      matchedCutIdx = cutIdx;
    } else {
      matchedCutIdx =
        myVCP.findCutIndex(radCopy.elevation, 0.05f, isBitSet(myVCP.vcp_supplemental_data, HW10_SAILS_VCP));
    }
    if (matchedCutIdx >= 0) {
      if (radCopy.elev_block.waveform_type == 0) {
        radCopy.elev_block.waveform_type = myVCP.elevations[matchedCutIdx].waveform_type;
      }
      if (radCopy.elev_block.sector1_spare == 0) {
        radCopy.elev_block.sector1_spare = myVCP.elevations[matchedCutIdx].sector1_spare;
      }
    }
  }

  myCurrentRadials.push_back(radCopy);

  if (radial.endOfElev() || radial.endOfVolume()) {
    finishCurrentSweep();
  }
} // NEXRADAccumulator::processRadial

void
NEXRADAccumulator::flush()
{
  if (!myCurrentRadials.empty() || !myPendingDataTables.empty() || myPendingMsg2.has_value() ||
    myPendingMsg3.has_value())
  {
    finishCurrentSweep();
  }
}

LLH
NEXRADAccumulator::resolveRadarLocation(const NEXRADMessage31Radial& firstRadial)
{
  // Strategy:
  // 1. Prioritize internal Message 31 coordinates if available (site_lat, site_lon, site_ht).
  // 2. Fallback to ConfigRadarInfo (radarinfo.xml) using radarName.
  // Note: Future IOConfig options can select/force a specific source.
  if (firstRadial.has_vol_block) {
    float lat = firstRadial.vol_block.site_lat;
    float lon = firstRadial.vol_block.site_lon;
    float ht  = static_cast<float>(firstRadial.vol_block.site_ht) / 1000.0f;
    if ((std::abs(lat) > 0.001f) || (std::abs(lon) > 0.001f)) {
      return LLH(lat, lon, ht);
    }
  }

  std::string name = myRadarName;

  if (name.empty() && (firstRadial.radar[0] != '\0')) {
    name = firstRadial.radar;
  }

  if (!name.empty() && ConfigRadarInfo::haveRadar(name)) {
    return ConfigRadarInfo::getLocation(name);
  }

  fLogInfo("NEXRADAccumulator: Unable to resolve radar location for site '{}', defaulting to 0,0", name);
  return LLH(0.0, 0.0, 0.0);
}

void
NEXRADAccumulator::finishCurrentSweep()
{
  materializePendingTables();

  if (myCurrentRadials.empty() && myPendingDataTables.empty()) {
    return;
  }

  // Moment definitions matching standard MRMS names
  const struct MomentDef {
    const char * rawName;
    const char * rapioName;
    const char * units;
  } moments[] = {
    { "REF", "Reflectivity",    "dBZ"                         },
    { "VEL", "AliasedVelocity", "MetersPerSecond"             },
    { "SW",  "SpectrumWidth",   "MetersPerSecond"             },
    { "ZDR", "Zdr",             "dB"                          },
    { "PHI", "PhiDP",           "degrees"                     },
    { "RHO", "RhoHV",           "dimensionless"               },
    { "SNR", "SNRR",            "dB"                          },
    { "SND", "SNRD",            "dB"                          }
  };

  std::vector<std::shared_ptr<DataType> > sweepOutputs;

  if (!myCurrentRadials.empty()) {
    for (const auto& mdef : moments) {
      auto rs = buildRadialSetForMoment(mdef.rawName, mdef.rapioName, mdef.units);
      if (rs) {
        sweepOutputs.push_back(rs);
        if (myKeepCompletedRadialSets) {
          myCompletedRadialSets.push_back(rs);
        }
      }
    }

    if (myWriteHVAN) {
      auto hvanTable = buildHVAzNoiseTable();
      if (hvanTable) {
        sweepOutputs.push_back(hvanTable);
      }
    }
  }

  for (const auto& dt : myPendingDataTables) {
    sweepOutputs.push_back(dt);
  }
  myPendingDataTables.clear();

  if (mySweepCallback && !sweepOutputs.empty()) {
    mySweepCallback(sweepOutputs);
  }
  if (myCurrentSweepNo > 0) {
    myLastWrittenSweepNo = myCurrentSweepNo;
  }

  myCurrentRadials.clear();
  myCurrentSweepNo       = -1;
  myCurrentElevation     = -999.0f;
  myCurrentStartOfVolume = false;
  myCurrentEndOfVolume   = false;
} // NEXRADAccumulator::finishCurrentSweep

std::shared_ptr<RadialSet>
NEXRADAccumulator::buildRadialSetForMoment(
  const std::string& rawMomentName,
  const std::string& rapioTypeName,
  const std::string& units)
{
  // 1. Verify if any radial in this sweep contains this moment
  size_t max_gates    = 0;
  float firstGateDist = 0.0f;
  float gateWidth     = 0.0f;
  bool foundMoment    = false;

  for (const auto& rad : myCurrentRadials) {
    auto it = rad.moments.find(rawMomentName);
    if (it != rad.moments.end()) {
      const auto& mb = it->second;
      if (mb.num_gates > max_gates) {
        max_gates = mb.num_gates;
      }
      if (!foundMoment) {
        firstGateDist = mb.getRangeToFirstGate() - (mb.getGateWidth() * 0.5f);
        gateWidth     = mb.getGateWidth();
        foundMoment   = true;
      }
    }
  }

  if (!foundMoment || (max_gates == 0)) {
    return nullptr;
  }

  const auto& firstRad = myCurrentRadials.front();

  // Find first radial with elev_block, vol_block, and rad_block (split cuts often omit them on later radials)
  const NEXRADMessage31Radial * elevRad = nullptr;
  const NEXRADMessage31Radial * volRad  = nullptr;
  const NEXRADMessage31Radial * radRad  = nullptr;

  for (const auto& rad : myCurrentRadials) {
    if (!elevRad && rad.has_elev_block) {
      elevRad = &rad;
    }
    if (!volRad && rad.has_vol_block) {
      volRad = &rad;
    }
    if (!radRad && rad.has_rad_block) {
      radRad = &rad;
    }
    if (elevRad && volRad && radRad) {
      break;
    }
  }
  if (!radRad) {
    radRad = &firstRad;
  }

  LLH location         = resolveRadarLocation(volRad ? *volRad : firstRad);
  Time sweepTime       = firstRad.getTime();
  float targetElevDegs = firstRad.elevation;
  size_t num_radials   = myCurrentRadials.size();

  std::string outputTypeName = rapioTypeName;
  std::string checkRadar     = myRadarName.empty() ? std::string(firstRad.radar) : myRadarName;
  bool isCBand = (ConfigRadarInfo::getFreqBand(myRadarName) == "C" || ConfigRadarInfo::getFreqBand(checkRadar) == "C");

  if (!isCBand && !ConfigRadarInfo::haveRadar(checkRadar)) {
    if ((checkRadar.size() == 4) && ((checkRadar[0] == 'T') || (checkRadar[0] == 't'))) {
      isCBand = true;
    }
  }
  const auto& effElevRad = elevRad ? *elevRad : firstRad;

  if ((rawMomentName == "REF") && isCBand && (effElevRad.elev_block.waveform_type == 1)) {
    outputTypeName = "ReflectivityLowPRF";
  }

  bool isSAILS = false;

  if (myHaveVCP) {
    int matchedCutIdx = -1;
    int cutIdx        = static_cast<int>(firstRad.elevation_no) - 1;
    if ((cutIdx >= 0) && (cutIdx < static_cast<int>(myVCP.elevations.size()))) {
      matchedCutIdx = cutIdx;
    } else {
      matchedCutIdx = myVCP.findCutIndex(targetElevDegs, 0.05f, isBitSet(myVCP.vcp_supplemental_data, HW10_SAILS_VCP));
    }

    if (matchedCutIdx >= 0) {
      float vcpElev = myVCP.elevations[matchedCutIdx].getElevationDegs((myUserVCP == 0) ? 1 : 2);
      if (std::abs(vcpElev - firstRad.elevation) < 1.0f) {
        targetElevDegs = vcpElev;
      }
      isSAILS = myVCP.elevations[matchedCutIdx].isSAILSCut();
      if (!isSAILS && isBitSet(myVCP.vcp_supplemental_data, HW10_SAILS_VCP) &&
        (firstRad.elevation_no > myVCP.elevations.size()))
      {
        isSAILS = true;
      }
    } else {
      isSAILS = isBitSet(myVCP.vcp_supplemental_data, HW10_SAILS_VCP);
    }
  }

  // Check SAILS cut flag (1 << 0), MRLE (1 << 4) and MPDA (1 << 9) flags, treating them identical to SAILS cuts
  if ((effElevRad.elev_block.sector1_spare & (1 << 0)) ||
    (effElevRad.elev_block.sector1_spare & (1 << 4)) ||
    (effElevRad.elev_block.sector1_spare & (1 << 9)))
  {
    isSAILS = true;
  }

  // PRF tracking per raw moment
  if (outputTypeName != "ReflectivityLowPRF") {
    auto itPrev = myPrevElevation.find(rawMomentName);
    if ((itPrev == myPrevElevation.end()) || (std::abs(targetElevDegs - itPrev->second) > 0.05f)) {
      myCurrentPRF[rawMomentName]      = 1;
      myPrevElevation[rawMomentName]   = targetElevDegs;
      myPrevSectorSpare[rawMomentName] = effElevRad.elev_block.sector1_spare;
    } else {
      auto itSpare = myPrevSectorSpare.find(rawMomentName);
      if ((itSpare == myPrevSectorSpare.end()) || (effElevRad.elev_block.sector1_spare != itSpare->second)) {
        myCurrentPRF[rawMomentName] = 1;
      } else {
        myCurrentPRF[rawMomentName]++;
      }
      myPrevElevation[rawMomentName]   = targetElevDegs;
      myPrevSectorSpare[rawMomentName] = effElevRad.elev_block.sector1_spare;
    }
  }

  std::string finalTypeName = outputTypeName;

  if ((outputTypeName != "ReflectivityLowPRF") && (myCurrentPRF[rawMomentName] > 1)) {
    finalTypeName += "PRF" + std::to_string(myCurrentPRF[rawMomentName]);
  }

  if (isSAILS && mySeparateSAILS) {
    if ((finalTypeName.size() < 6) || (finalTypeName.rfind("_EXTRA") != finalTypeName.size() - 6)) {
      finalTypeName += "_EXTRA";
    }
  }

  if (!myAddLower && (targetElevDegs < 0.45f)) {
    if ((finalTypeName.size() < 6) || (finalTypeName.rfind("_EXTRA") != finalTypeName.size() - 6)) {
      finalTypeName += "_EXTRA";
    }
  }

  // 2. Instantiate RAPIO RadialSet
  auto rs = RadialSet::Create(
    finalTypeName, units, location, sweepTime,
    targetElevDegs, firstGateDist, gateWidth,
    num_radials, max_gates);

  if (!rs) {
    return nullptr;
  }

  // 3. Obtain pointers to 2D grid and 1D metadata arrays
  ArrayFloat2DPtr outputArray = rs->getFloat2DPtr();
  ArrayFloat1DPtr azDegs      = rs->getFloat1DPtr(RadialSet::Azimuth);
  ArrayFloat1DPtr bwDegs      = rs->getFloat1DPtr(RadialSet::BeamWidth);
  ArrayFloat1DPtr gwMeters    = rs->getFloat1DPtr(RadialSet::GateWidth);

  const auto& effRadRad = (radRad ? *radRad : firstRad);
  float globalNyq       = effRadRad.radial_block.getNyquistVelocity();

  bool uniformNyquist = true;

  for (size_t r = 0; r < num_radials; ++r) {
    float rNyq = myCurrentRadials[r].has_rad_block ? myCurrentRadials[r].radial_block.getNyquistVelocity() : globalNyq;
    if (std::abs(rNyq - globalNyq) > 1e-3f) {
      uniformNyquist = false;
      break;
    }
  }

  ArrayFloat1DPtr nyqArr = nullptr;

  if (!uniformNyquist) {
    rs->addNyquistArray(0.0f);
    nyqArr = rs->getFloat1DPtr(RadialSet::NyquistVelocity);
  }
  rs->addRadialTimeArray(0);
  ArrayInt1DPtr timeArr = rs->getInt1DPtr(RadialSet::RadialTime);

  // 4. Populate radials and gates
  for (size_t r = 0; r < num_radials; ++r) {
    const auto& rad = myCurrentRadials[r];

    // Steve Smith beam center correction (legacy ldm2netcdf convention)
    float currAz = rad.azimuth - 0.5f * rad.getAzimuthSpacing();
    if (currAz < 0.0f) { currAz += 360.0f; }
    if (currAz >= 360.0f) { currAz -= 360.0f; }

    (*azDegs)[r]   = currAz;
    (*bwDegs)[r]   = rad.getAzimuthSpacing();
    (*gwMeters)[r] = gateWidth;
    if (nyqArr) {
      float rNyq = rad.has_rad_block ? rad.radial_block.getNyquistVelocity() : globalNyq;
      (*nyqArr)[r] = rNyq;
    }
    if (timeArr) {
      (*timeArr)[r] = static_cast<int>((rad.getTime() - sweepTime).milliseconds());
    }

    auto it = rad.moments.find(rawMomentName);
    if (it != rad.moments.end()) {
      const auto& gateData = it->second.data;
      size_t numG = gateData.size();
      for (size_t g = 0; g < max_gates; ++g) {
        if (g < numG) {
          float val = gateData[g];
          if ((rawMomentName == "REF") && (val == Constants::RangeFolded)) {
            val = Constants::DataUnavailable;
          }
          (*outputArray)[r][g] = val;
        } else {
          (*outputArray)[r][g] = Constants::MissingData;
        }
      }
    } else {
      for (size_t g = 0; g < max_gates; ++g) {
        (*outputArray)[r][g] = Constants::MissingData;
      }
    }
  }

  // 5. Azimuthal spacing / beamwidth adjustment (curr to next difference)
  if (num_radials > 1) {
    for (size_t r = 0; r < num_radials - 1; ++r) {
      float nextAz = (*azDegs)[r + 1];
      float curAz  = (*azDegs)[r];
      float diff   = nextAz - curAz;
      if (diff < 0.0f) { diff += 360.0f; }
      if (diff >= 360.0f) { diff -= 360.0f; }
      float nominalSpacing = myCurrentRadials[r].getAzimuthSpacing();
      if ((diff > 0.01f) && (diff <= 1.25f * nominalSpacing)) {
        (*bwDegs)[r] = diff;
      }
    }
    (*bwDegs)[num_radials - 1] = (*bwDegs)[num_radials - 2];
  }

  // 6. Set sweep and radar attributes
  std::string radarName = myRadarName.empty() ? std::string(firstRad.radar) : myRadarName;

  rs->setRadarName(radarName);
  int vcpNum = (volRad && volRad->has_vol_block) ? volRad->vol_block.vcp : 0;

  if (myUserVCP > 0) {
    vcpNum = myUserVCP;
  } else if ((vcpNum == 0) && myHaveVCP) {
    vcpNum = myVCP.pattern_number;
  }

  if (isCBand && (myUserVCP <= 0)) {
    if ((vcpNum == 1) || (vcpNum == 3)) {
      vcpNum = 90;
    } else if (vcpNum == 2) {
      vcpNum = 80;
    }
  }
  rs->setVCP(vcpNum);
  rs->setElevationDegs(targetElevDegs);

  std::string sailsScanStr = isSAILS ? "true" : "false";

  rs->setString("SAILS_scan", sailsScanStr);
  rs->setDataAttributeValue("SAILS_scan", sailsScanStr, "text");

  std::string sovStr = myCurrentStartOfVolume ? "true" : "false";

  rs->setString("StartOfVolume", sovStr);
  rs->setDataAttributeValue("StartOfVolume", sovStr, "text");

  std::string eovStr = myCurrentEndOfVolume ? "true" : "false";

  rs->setString("EndOfVolume", eovStr);
  rs->setDataAttributeValue("EndOfVolume", eovStr, "text");

  if (myUseVolumeTime && myHaveVolumeStartTime && !isSAILS) {
    std::string volTimeStr = myVolumeStartTime.getString("%Y%m%d-%H%M%S");
    rs->setString(Constants::FilenameDateTime, volTimeStr);
    rs->setDataAttributeValue(Constants::FilenameDateTime, volTimeStr, "dimensionless");
  }

  // Legacy MRMS VCP and elevation attributes
  std::string volElevIdxStr = std::to_string(firstRad.elevation_no);

  rs->setString("volElevIndex", volElevIdxStr);
  rs->setDataAttributeValue("volElevIndex", volElevIdxStr, "dimensionless");

  if (myHaveVCP) {
    size_t numElevations = myVCP.elevations.size();
    std::stringstream tempSS;
    for (size_t elIndex = 0; elIndex < numElevations; ++elIndex) {
      tempSS << myVCP.elevations[elIndex].getElevationDegs((myUserVCP == 0) ? 1 : 2) << " ";
    }
    std::string volElevAngles = tempSS.str();
    if (!volElevAngles.empty() && (volElevAngles.back() == ' ')) {
      volElevAngles.pop_back();
    }
    rs->setString("volElevAngles", volElevAngles);
    rs->setDataAttributeValue("volElevAngles", volElevAngles, "Degrees");

    std::string reportedVcpStr = std::to_string(myVCP.pattern_number);
    rs->setString("reported_vcp", reportedVcpStr);
    rs->setDataAttributeValue("reported_vcp", reportedVcpStr, "Degrees");

    uint16_t sectorSpare = 0;
    int cutIdx = static_cast<int>(firstRad.elevation_no) - 1;
    if ((cutIdx >= 0) && (cutIdx < static_cast<int>(myVCP.elevations.size()))) {
      sectorSpare = myVCP.elevations[cutIdx].sector1_spare;
    } else {
      int matched =
        myVCP.findCutIndex(firstRad.elevation, 0.05f, isBitSet(myVCP.vcp_supplemental_data, HW10_SAILS_VCP));
      if (matched >= 0) {
        sectorSpare = myVCP.elevations[matched].sector1_spare;
      }
    }
    if (effElevRad.elev_block.sector1_spare != 0) {
      sectorSpare |= effElevRad.elev_block.sector1_spare;
    }

    std::string truncStr = isBitSet(myVCP.vcp_supplemental_data, HW09_TRUNCATED_VCP) ? "true" : "false";
    rs->setString("TRUNCATED_VCP", truncStr);
    rs->setDataAttributeValue("TRUNCATED_VCP", truncStr, "text");

    std::string sailsVcpStr = isBitSet(myVCP.vcp_supplemental_data, HW10_SAILS_VCP) ? "true" : "false";
    rs->setString("SAILS_VCP", sailsVcpStr);
    rs->setDataAttributeValue("SAILS_VCP", sailsVcpStr, "text");

    std::string mrleVcpStr = isBitSet(myVCP.vcp_supplemental_data, HW10_MRLE_VCP) ? "true" : "false";
    rs->setString("MRLE_VCP", mrleVcpStr);
    rs->setDataAttributeValue("MRLE_VCP", mrleVcpStr, "text");

    std::string mpdaCutsStr = isBitSet(myVCP.vcp_supplemental_data, HW10_MPDA_CUTS_ADDED) ? "true" : "false";
    rs->setString("MPDA_CUTS_ADDED", mpdaCutsStr);
    rs->setDataAttributeValue("MPDA_CUTS_ADDED", mpdaCutsStr, "text");

    std::string mpdaVcpStr = isBitSet(myVCP.vcp_supplemental_data, HW10_MPDA_VCP) ? "true" : "false";
    rs->setString("MPDA_VCP", mpdaVcpStr);
    rs->setDataAttributeValue("MPDA_VCP", mpdaVcpStr, "text");

    std::string sailsCutStr = isBitSet(sectorSpare, HWE15_SAILS_CUT) ? "true" : "false";
    rs->setString("SAILS_CUT", sailsCutStr);
    rs->setDataAttributeValue("SAILS_CUT", sailsCutStr, "text");

    std::string mrleCutStr = isBitSet(sectorSpare, HWE15_MRLE_CUT) ? "true" : "false";
    rs->setString("MRLE_CUT", mrleCutStr);
    rs->setDataAttributeValue("MRLE_CUT", mrleCutStr, "text");

    std::string mpdaCutStr = isBitSet(sectorSpare, HWE15_MPDA_CUT) ? "true" : "false";
    rs->setString("MPDA_CUT", mpdaCutStr);
    rs->setDataAttributeValue("MPDA_CUT", mpdaCutStr, "text");

    std::string baseTiltCutStr = isBitSet(sectorSpare, HWE15_BASE_TILT_CUT) ? "true" : "false";
    rs->setString("BASE_TILT_CUT", baseTiltCutStr);
    rs->setDataAttributeValue("BASE_TILT_CUT", baseTiltCutStr, "text");
  } else {
    uint16_t sectorSpare = effElevRad.elev_block.sector1_spare;
    rs->setString("TRUNCATED_VCP", "false");
    rs->setDataAttributeValue("TRUNCATED_VCP", "false", "text");
    rs->setString("SAILS_VCP", "false");
    rs->setDataAttributeValue("SAILS_VCP", "false", "text");
    rs->setString("MRLE_VCP", "false");
    rs->setDataAttributeValue("MRLE_VCP", "false", "text");
    rs->setString("MPDA_CUTS_ADDED", "false");
    rs->setDataAttributeValue("MPDA_CUTS_ADDED", "false", "text");
    rs->setString("MPDA_VCP", "false");
    rs->setDataAttributeValue("MPDA_VCP", "false", "text");

    std::string sailsCutStr = isBitSet(sectorSpare, HWE15_SAILS_CUT) ? "true" : "false";
    rs->setString("SAILS_CUT", sailsCutStr);
    rs->setDataAttributeValue("SAILS_CUT", sailsCutStr, "text");

    std::string mrleCutStr = isBitSet(sectorSpare, HWE15_MRLE_CUT) ? "true" : "false";
    rs->setString("MRLE_CUT", mrleCutStr);
    rs->setDataAttributeValue("MRLE_CUT", mrleCutStr, "text");

    std::string mpdaCutStr = isBitSet(sectorSpare, HWE15_MPDA_CUT) ? "true" : "false";
    rs->setString("MPDA_CUT", mpdaCutStr);
    rs->setDataAttributeValue("MPDA_CUT", mpdaCutStr, "text");

    std::string baseTiltCutStr = isBitSet(sectorSpare, HWE15_BASE_TILT_CUT) ? "true" : "false";
    rs->setString("BASE_TILT_CUT", baseTiltCutStr);
    rs->setDataAttributeValue("BASE_TILT_CUT", baseTiltCutStr, "text");
  }

  if (uniformNyquist) {
    rs->setGlobalNyquistVelocity(globalNyq);
  }

  // Dual-pol & RDA attributes matching legacy ldm2netcdf
  float dbz0Cal    = elevRad ? elevRad->elev_block.dbz0_calibration_constant : 0.0f;
  float atmosAtten = elevRad ? (elevRad->elev_block.atmos_attenuation_factor / 1000.0f) : 0.0f;
  float sysZdr     = volRad ? volRad->vol_block.zdr_calibration : 0.0f;
  float minPhiDP   = volRad ? volRad->vol_block.phidp_initial : 0.0f;
  float horTxPower = volRad ? volRad->vol_block.hor_shv_tx_power : 0.0f;
  float verTxPower = volRad ? volRad->vol_block.ver_shv_tx_power : 0.0f;

  std::string cStr = std::to_string(-1.0f * dbz0Cal);

  rs->setString("C", cStr);
  rs->setDataAttributeValue("C", cStr, "dimensionless");

  rs->setString("Cal_Z", "0.0");
  rs->setDataAttributeValue("Cal_Z", "0.0", "dBZ");

  rs->setString("Cal_Zdr", "0.0");
  rs->setDataAttributeValue("Cal_Zdr", "0.0", "dB");

  std::string freezeStr = std::to_string(static_cast<double>(Constants::MissingData));

  rs->setString("Freezing_Level", freezeStr);
  rs->setDataAttributeValue("Freezing_Level", freezeStr, "Meter");

  std::string sysZdrStr = std::to_string(sysZdr);

  rs->setString("System_Zdr", sysZdrStr);
  rs->setDataAttributeValue("System_Zdr", sysZdrStr, "dB");

  std::string minPhiDPStr = std::to_string(minPhiDP);

  rs->setString("Min_Sys_PhiDP", minPhiDPStr);
  rs->setDataAttributeValue("Min_Sys_PhiDP", minPhiDPStr, "Degree");

  std::string unambRangeStr = std::to_string(effRadRad.radial_block.getUnambiguousRangeKm());

  rs->setString("Unambiguous_Range", unambRangeStr);
  rs->setDataAttributeValue("Unambiguous_Range", unambRangeStr, "kilometer");

  std::string hNoiseStr = std::to_string(effRadRad.radial_block.hor_noise_level);

  rs->setString("Horizontal_Noise", hNoiseStr);
  rs->setDataAttributeValue("Horizontal_Noise", hNoiseStr, "dBm");

  std::string vNoiseStr = std::to_string(effRadRad.radial_block.ver_noise_level);

  rs->setString("Vertical_Noise", vNoiseStr);
  rs->setDataAttributeValue("Vertical_Noise", vNoiseStr, "dBm");

  std::string atmosStr = std::to_string(atmosAtten);

  rs->setString("Atmos", atmosStr);
  rs->setDataAttributeValue("Atmos", atmosStr, "dB/km");

  std::string hPowerStr = std::to_string(horTxPower);

  rs->setString("Horizontal_Trans_Power", hPowerStr);
  rs->setDataAttributeValue("Horizontal_Trans_Power", hPowerStr, "kW");

  std::string vPowerStr = std::to_string(verTxPower);

  rs->setString("Vertical_Trans_Power", vPowerStr);
  rs->setDataAttributeValue("Vertical_Trans_Power", vPowerStr, "kW");

  auto itMoment = firstRad.moments.find(rawMomentName);

  if (itMoment != firstRad.moments.end()) {
    std::string toverStr = std::to_string(itMoment->second.threshold_for_overlayed / 10.0f);
    rs->setString("Tover", toverStr);
    rs->setDataAttributeValue("Tover", toverStr, "db");

    std::string snrStr = std::to_string(itMoment->second.snr_threshold / 8.0f);
    rs->setString("SNR_Threshold", snrStr);
    rs->setDataAttributeValue("SNR_Threshold", snrStr, "dB");
  }

  // ZDR Bias Estimate Weighted Mean (Build 20+)
  // Default to 0.000000 matching legacy ldm2netcdf output
  double biasVal = Constants::DataUnavailable;

  if (volRad && (volRad->vol_block.major_version > 2)) {
    int16_t rawBias = volRad->vol_block.zdr_bias_estimate_weighted_mean;
    // ICD 2620002 Table XVII-E comment (33): 0 means not available
    if (rawBias != 0) {
      auto itZDR = firstRad.moments.find("ZDR");
      if (itZDR == firstRad.moments.end()) {
        for (const auto& rad : myCurrentRadials) {
          auto it = rad.moments.find("ZDR");
          if (it != rad.moments.end()) {
            itZDR = it;
            break;
          }
        }
      }
      if (itZDR != firstRad.moments.end()) {
        const auto& zdrM = itZDR->second;
        double scale     = (zdrM.scale != 0.0f) ? static_cast<double>(zdrM.scale) : 1.0;
        double converted = (static_cast<double>(rawBias) - zdrM.offset) / scale;
        // Meteorological bounds check for ZDR bias (typically within [-14.0, 20.0] dB)
        if ((converted >= -14.0) && (converted <= 20.0)) {
          biasVal = converted;
        } else if (rawBias == 1) {
          biasVal = Constants::RangeFolded;
        }
      }
    }
  }
  std::string biasStr = std::to_string(biasVal);

  rs->setString("ZDR_Bias_Estimate_Weighted_Mean", biasStr);
  rs->setDataAttributeValue("ZDR_Bias_Estimate_Weighted_Mean", biasStr, "dB");

  // Format elevation subtype e.g. "00.50"
  std::string elevStr = DataType::formatString(targetElevDegs, 5, 2);

  rs->setSubType(elevStr);
  rs->setString("SubType", elevStr);
  std::string elevAngleStr = std::to_string(targetElevDegs);

  rs->setString("Elevation", elevAngleStr);
  rs->setDataAttributeValue("Elevation", elevAngleStr, "Degrees");

  if (myAlignmentResolution > 0.0f) {
    size_t targetRadials = static_cast<size_t>(std::round(360.0f / myAlignmentResolution));
    if (targetRadials > 0) {
      rs = rs->Remap(rs->getGateWidthKMs() * 1000.0f, targetRadials, rs->getNumGates(), false);
      rs->setSubType(elevStr);
      rs->setString("SubType", elevStr);
    }
  }

  return rs;
} // NEXRADAccumulator::buildRadialSetForMoment

std::shared_ptr<DataTable>
NEXRADAccumulator::buildHVAzNoiseTable()
{
  if (!myWriteHVAN || myCurrentRadials.empty()) {
    return nullptr;
  }

  const auto& firstRad = myCurrentRadials.front();

  const NEXRADMessage31Radial * elevRad = nullptr;
  const NEXRADMessage31Radial * volRad  = nullptr;

  for (const auto& rad : myCurrentRadials) {
    if (!elevRad && rad.has_elev_block) {
      elevRad = &rad;
    }
    if (!volRad && rad.has_vol_block) {
      volRad = &rad;
    }
    if (elevRad && volRad) {
      break;
    }
  }

  // Legacy LevelIIAccumulator only collected HVAN if
  // the elevation cut was Contiguous Surveillance (waveform_type == 1) or Batch (waveform_type == 4).
  uint8_t waveform_type = 0;

  if (myHaveVCP) {
    int cutIdx = static_cast<int>(firstRad.elevation_no) - 1;
    if ((cutIdx >= 0) && (cutIdx < static_cast<int>(myVCP.elevations.size()))) {
      waveform_type = myVCP.elevations[cutIdx].waveform_type;
    } else {
      int matched =
        myVCP.findCutIndex(firstRad.elevation, 0.05f, isBitSet(myVCP.vcp_supplemental_data, HW10_SAILS_VCP));
      if (matched >= 0) {
        waveform_type = myVCP.elevations[matched].waveform_type;
      }
    }
  }
  if ((waveform_type == 0) && elevRad) {
    waveform_type = elevRad->elev_block.waveform_type;
  }
  if (waveform_type == 0) {
    waveform_type = (firstRad.moments.find("VEL") != firstRad.moments.end()) ? 2 : 1;
  }
  if ((waveform_type != 1) && (waveform_type != 4)) {
    // Skip Doppler scans (e.g. waveform_type == 2 or 3)
    return nullptr;
  }

  auto table = std::make_shared<DataTable>();

  float targetElevDegs = firstRad.elevation;

  if (myHaveVCP) {
    int matchedCutIdx = -1;
    int cutIdx        = static_cast<int>(firstRad.elevation_no) - 1;
    if ((cutIdx >= 0) && (cutIdx < static_cast<int>(myVCP.elevations.size()))) {
      matchedCutIdx = cutIdx;
    } else {
      matchedCutIdx = myVCP.findCutIndex(firstRad.elevation, 0.05f, isBitSet(myVCP.vcp_supplemental_data,
          HW10_SAILS_VCP));
    }
    if (matchedCutIdx >= 0) {
      targetElevDegs = myVCP.elevations[matchedCutIdx].getElevationDegs((myUserVCP == 0) ? 1 : 2);
    }
  }

  // Check SAILS / MRLE / MPDA
  bool isSAILS = false;

  if (myHaveVCP) {
    int matchedCutIdx = -1;
    int cutIdx        = static_cast<int>(firstRad.elevation_no) - 1;
    if ((cutIdx >= 0) && (cutIdx < static_cast<int>(myVCP.elevations.size()))) {
      matchedCutIdx = cutIdx;
    } else {
      matchedCutIdx = myVCP.findCutIndex(firstRad.elevation, 0.05f, isBitSet(myVCP.vcp_supplemental_data,
          HW10_SAILS_VCP));
    }
    if (matchedCutIdx >= 0) {
      isSAILS = myVCP.elevations[matchedCutIdx].isSAILSCut() || myVCP.elevations[matchedCutIdx].isMRLECut();
      if (!isSAILS && isBitSet(myVCP.vcp_supplemental_data, HW10_SAILS_VCP) &&
        (firstRad.elevation_no > myVCP.elevations.size()))
      {
        isSAILS = true;
      }
    } else {
      isSAILS = isBitSet(myVCP.vcp_supplemental_data, HW10_SAILS_VCP);
    }
  }
  const auto& effElevRad = elevRad ? *elevRad : firstRad;

  if ((effElevRad.elev_block.sector1_spare & (1 << 0)) ||
    (effElevRad.elev_block.sector1_spare & (1 << 4)) ||
    (effElevRad.elev_block.sector1_spare & (1 << 9)))
  {
    isSAILS = true;
  }

  std::string tmpName = "HVAzNoiseTable";
  int prfIndex        = 1;
  auto itPrf = myCurrentPRF.find("REF");

  if ((itPrf != myCurrentPRF.end()) && (itPrf->second > 1)) {
    prfIndex = itPrf->second;
    tmpName += "PRF" + std::to_string(prfIndex);
  }

  if (isSAILS && mySeparateSAILS) {
    tmpName += "_EXTRA";
  }

  if (!myAddLower && (targetElevDegs < 0.45f)) {
    if ((tmpName.size() < 6) || (tmpName.rfind("_EXTRA") != tmpName.size() - 6)) {
      tmpName += "_EXTRA";
    }
  }

  table->setTypeName(tmpName);

  LLH location = resolveRadarLocation(volRad ? *volRad : firstRad);

  table->setLocation(location);

  Time sweepTime = firstRad.getTime();

  table->setTime(sweepTime);

  if (myUseVolumeTime && myHaveVolumeStartTime && !isSAILS) {
    std::string volTimeStr = myVolumeStartTime.getString("%Y%m%d-%H%M%S");
    table->setString(Constants::FilenameDateTime, volTimeStr);
    table->setDataAttributeValue(Constants::FilenameDateTime, volTimeStr, "dimensionless");
  }

  std::string radarName = myRadarName;

  if (radarName.empty() && (firstRad.radar[0] != '\0')) {
    radarName = firstRad.radar;
  }
  table->setString("radarName", radarName);
  table->setDataAttributeValue("radarName", radarName, "dimensionless");

  int vcpNum = 0;

  for (const auto& rad : myCurrentRadials) {
    if (rad.has_vol_block && (rad.vol_block.vcp > 0)) {
      vcpNum = rad.vol_block.vcp;
      break;
    }
  }
  if (myUserVCP > 0) {
    vcpNum = myUserVCP;
  } else if ((vcpNum == 0) && myHaveVCP) {
    vcpNum = myVCP.pattern_number;
  }
  std::string checkRadar = myRadarName.empty() ? std::string(myCurrentRadials.front().radar) : myRadarName;
  bool isCBand = (ConfigRadarInfo::getFreqBand(myRadarName) == "C" || ConfigRadarInfo::getFreqBand(checkRadar) == "C");

  if (!isCBand && !ConfigRadarInfo::haveRadar(checkRadar)) {
    if ((checkRadar.size() == 4) && ((checkRadar[0] == 'T') || (checkRadar[0] == 't'))) {
      isCBand = true;
    }
  }
  if (isCBand && (myUserVCP <= 0)) {
    if ((vcpNum == 1) || (vcpNum == 3)) {
      vcpNum = 90;
    } else if (vcpNum == 2) {
      vcpNum = 80;
    }
  }
  std::string vcpStr = std::to_string(vcpNum);

  table->setString("vcp", vcpStr);
  table->setDataAttributeValue("vcp", vcpStr, "dimensionless");

  std::string elevStr = DataType::formatString(targetElevDegs, 5, 2);

  table->setSubType(elevStr);
  table->setString("SubType", elevStr);
  table->setDataAttributeValue("SubType", elevStr, "dimensionless");
  std::string elevAngleStr = std::to_string(targetElevDegs);

  table->setString("Elevation", elevAngleStr);
  table->setDataAttributeValue("Elevation", elevAngleStr, "Degrees");
  std::string eovStr = myCurrentEndOfVolume ? "true" : "false";

  table->setString("EndOfVolume", eovStr);
  table->setDataAttributeValue("EndOfVolume", eovStr, "text");
  std::string sailsScanStr = isSAILS ? "true" : "false";

  table->setString("SAILS_scan", sailsScanStr);
  table->setDataAttributeValue("SAILS_scan", sailsScanStr, "text");

  for (const auto& rad : myCurrentRadials) {
    if (rad.has_vol_block) {
      std::string sysZdrStr = std::to_string(rad.vol_block.zdr_calibration);
      table->setString("System_Zdr", sysZdrStr);
      table->setDataAttributeValue("System_Zdr", sysZdrStr, "dB");
      break;
    }
  }

  std::vector<std::pair<std::string, DataColumn::Type> > schema = {
    { "Azimuth",         DataColumn::Type::Float         },
    { "HorizontalNoise", DataColumn::Type::Float         },
    { "VerticalNoise",   DataColumn::Type::Float         }
  };

  table->setSchema(schema);

  for (const auto& rad : myCurrentRadials) {
    std::map<std::string, DataValue> row;
    float currAz = rad.azimuth - 0.5f * rad.getAzimuthSpacing();
    if (currAz < 0.0f) {
      currAz += 360.0f;
    } else if (currAz >= 360.0f) {
      currAz -= 360.0f;
    }
    row["Azimuth"]         = currAz;
    row["HorizontalNoise"] = rad.radial_block.hor_noise_level;
    row["VerticalNoise"]   = rad.radial_block.ver_noise_level;
    table->addRow(row);
  }

  return table;
} // NEXRADAccumulator::buildHVAzNoiseTable

void
NEXRADAccumulator::processMessage2(const NEXRADMessage2RDAStatus& msg2)
{
  myPendingMsg2 = msg2;
}

void
NEXRADAccumulator::processMessage3(const NEXRADMessage3Data& msg3)
{
  myPendingMsg3 = msg3;
}

void
NEXRADAccumulator::materializePendingTables()
{
  if (!myPendingMsg2.has_value() && !myPendingMsg3.has_value()) {
    return;
  }

  const NEXRADMessage31Radial * volRad      = nullptr;
  const NEXRADMessage31Radial * firstRadPtr = nullptr;

  if (!myCurrentRadials.empty()) {
    firstRadPtr = &myCurrentRadials.front();
    for (const auto& rad : myCurrentRadials) {
      if (rad.has_vol_block) {
        volRad = &rad;
        break;
      }
    }
  }

  std::string radarName = myRadarName;

  if (radarName.empty()) {
    if (volRad && (volRad->radar[0] != '\0')) {
      radarName = volRad->radar;
    } else if (firstRadPtr && (firstRadPtr->radar[0] != '\0')) {
      radarName = firstRadPtr->radar;
    }
  }

  LLH loc = (volRad || firstRadPtr) ? resolveRadarLocation(volRad ? *volRad : *firstRadPtr) :
    resolveRadarLocation(NEXRADMessage31Radial());

  if (myPendingMsg2.has_value()) {
    const auto& msg2 = *myPendingMsg2;
    auto table       = std::make_shared<DataTable>();
    table->setTypeName("RDAStatusTable");

    Time t = firstRadPtr ? firstRadPtr->getTime() : msg2.time;
    if (t.getSecondsSinceEpoch() <= 0) {
      if (myHaveVolumeStartTime) {
        t = myVolumeStartTime;
      } else {
        t = Time::CurrentTime();
      }
    }
    table->setLocation(loc);
    table->setTime(t);
    if (myUseVolumeTime && myHaveVolumeStartTime) {
      std::string volTimeStr = myVolumeStartTime.getString("%Y%m%d-%H%M%S");
      table->setString(Constants::FilenameDateTime, volTimeStr);
      table->setDataAttributeValue(Constants::FilenameDateTime, volTimeStr, "dimensionless");
    }
    table->setSubType("00.00");
    table->setString("SubType", "00.00");
    table->setDataAttributeValue("SubType", "00.00", "dimensionless");
    table->setString("radarName", radarName);
    table->setDataAttributeValue("radarName", radarName, "dimensionless");

    std::vector<std::pair<std::string, DataColumn::Type> > schema = {
      { "RDAStatus",                       DataColumn::Type::Integer                       },
      { "OperabilityStatus",               DataColumn::Type::Integer                       },
      { "ControlStatus",                   DataColumn::Type::Integer                       },
      { "AuxillaryPowerGeneratorState",    DataColumn::Type::Integer                       },
      { "AverageTransmitterPower",         DataColumn::Type::Integer                       },
      { "HorzZHCalibrationCorrection",     DataColumn::Type::Float                         },
      { "DataTransmissionEnabled",         DataColumn::Type::Integer                       },
      { "VCPNumber",                       DataColumn::Type::Integer                       },
      { "RDAControlAuthorization",         DataColumn::Type::Integer                       },
      { "RDABuildNumber",                  DataColumn::Type::Float                         },
      { "OperationalMode",                 DataColumn::Type::Integer                       },
      { "SuperResolutionStatus",           DataColumn::Type::Integer                       },
      { "ClutterMitigationDecisionStatus", DataColumn::Type::Integer                       },
      { "AVSET-EBC-LogStatus",             DataColumn::Type::Integer                       },
      { "RDAAlarmSummary",                 DataColumn::Type::Integer                       },
      { "CommandAcknowledgment",           DataColumn::Type::Integer                       },
      { "ChannelControlStatus",            DataColumn::Type::Integer                       },
      { "SpotBlankingStatus",              DataColumn::Type::Integer                       },
      { "BypassMapGenerationDate",         DataColumn::Type::Integer                       },
      { "BypassMapGenerationTime",         DataColumn::Type::Integer                       },
      { "ClutterFilterMapGenerationDate",  DataColumn::Type::Integer                       },
      { "ClutterFilterMapGenerationTime",  DataColumn::Type::Integer                       },
      { "VertZHCalibrationCorrection",     DataColumn::Type::Float                         },
      { "TransitionPowerSourceStatus",     DataColumn::Type::Integer                       },
      { "RMSControlStatus",                DataColumn::Type::Integer                       },
      { "PerformanceCheckStatus",          DataColumn::Type::Integer                       },
      { "AlarmCodes",                      DataColumn::Type::Integer                       },
      { "SignalProcessingOptions",         DataColumn::Type::Integer                       },
      { "StatusVersion",                   DataColumn::Type::Integer                       }
    };
    table->setSchema(schema);

    float build_number = static_cast<float>(msg2.rda_build_number);
    if ((build_number / 100.0f) > 2.0f) {
      build_number /= 100.0f;
    } else {
      build_number /= 10.0f;
    }

    std::map<std::string, DataValue> row;
    row["RDAStatus"]         = static_cast<int>(msg2.rda_status);
    row["OperabilityStatus"] = static_cast<int>(msg2.operability_status);
    row["ControlStatus"]     = static_cast<int>(msg2.control_status);
    row["AuxillaryPowerGeneratorState"] = static_cast<int>(msg2.aux_power_generator_state);
    row["AverageTransmitterPower"]      = static_cast<int>(msg2.avg_transmiter_power);
    row["HorzZHCalibrationCorrection"]  = static_cast<float>(msg2.horz_reflecitivty_calibration_correction) / 100.0f;
    row["DataTransmissionEnabled"]      = static_cast<int>(msg2.data_transmission_enabled);
    row["VCPNumber"] = static_cast<int>(msg2.volume_coverage_pattern_number);
    row["RDAControlAuthorization"] = static_cast<int>(msg2.rda_control_auth);
    row["RDABuildNumber"]        = build_number;
    row["OperationalMode"]       = static_cast<int>(msg2.operational_mode);
    row["SuperResolutionStatus"] = static_cast<int>(msg2.super_resolution_status);
    row["ClutterMitigationDecisionStatus"] = static_cast<int>(msg2.clutter_mitigation_decision_status);
    row["AVSET-EBC-LogStatus"]            = static_cast<int>(msg2.avset_ebc_log_status);
    row["RDAAlarmSummary"]                = static_cast<int>(msg2.rda_alarm_summary);
    row["CommandAcknowledgment"]          = static_cast<int>(msg2.command_acknowledgment);
    row["ChannelControlStatus"]           = static_cast<int>(msg2.channel_control_status);
    row["SpotBlankingStatus"]             = static_cast<int>(msg2.spot_blanking_status);
    row["BypassMapGenerationDate"]        = static_cast<int>(msg2.bypass_map_gen_date);
    row["BypassMapGenerationTime"]        = static_cast<int>(msg2.bypass_map_gen_time);
    row["ClutterFilterMapGenerationDate"] = static_cast<int>(msg2.clutter_filter_map_gen_date);
    row["ClutterFilterMapGenerationTime"] = static_cast<int>(msg2.clutter_filter_map_gen_time);
    row["VertZHCalibrationCorrection"]    = static_cast<float>(msg2.vertical_reflectivity_calibration_correction)
      / 100.0f;
    row["TransitionPowerSourceStatus"] = static_cast<int>(msg2.transition_power_source_status);
    row["RMSControlStatus"]        = static_cast<int>(msg2.rms_control_status);
    row["PerformanceCheckStatus"]  = static_cast<int>(msg2.performance_check_status);
    row["SignalProcessingOptions"] = static_cast<int>(msg2.signal_processing_options);
    row["StatusVersion"] = static_cast<int>(msg2.status_version);

    for (size_t i = 0; i < 14; ++i) {
      row["AlarmCodes"] = static_cast<int>(msg2.alarm_codes[i]);
      table->addRow(row);
    }
    myPendingDataTables.push_back(table);
    myPendingMsg2.reset();
  }

  if (myPendingMsg3.has_value()) {
    const auto& msg3 = *myPendingMsg3;
    auto table       = std::make_shared<DataTable>();
    table->setTypeName("MSG3DataTable");

    Time t = firstRadPtr ? firstRadPtr->getTime() : msg3.time;
    if (t.getSecondsSinceEpoch() <= 0) {
      if (myHaveVolumeStartTime) {
        t = myVolumeStartTime;
      } else {
        t = Time::CurrentTime();
      }
    }
    table->setLocation(loc);
    table->setTime(t);
    if (myUseVolumeTime && myHaveVolumeStartTime) {
      std::string volTimeStr = myVolumeStartTime.getString("%Y%m%d-%H%M%S");
      table->setString(Constants::FilenameDateTime, volTimeStr);
      table->setDataAttributeValue(Constants::FilenameDateTime, volTimeStr, "dimensionless");
    }
    table->setSubType("00.00");
    table->setString("SubType", "00.00");
    table->setDataAttributeValue("SubType", "00.00", "dimensionless");
    table->setString("radarName", radarName);
    table->setDataAttributeValue("radarName", radarName, "dimensionless");

    std::vector<std::pair<std::string, DataColumn::Type> > schema = {
      { "loop_back_test_status",                     DataColumn::Type::Integer                                     },
      { "t1_output_frames",                          DataColumn::Type::Integer                                     },
      { "t1_input_frames",                           DataColumn::Type::Integer                                     },
      { "router_memory_used",                        DataColumn::Type::Integer                                     },
      { "router_memory_free",                        DataColumn::Type::Integer                                     },
      { "router_memory_utilization",                 DataColumn::Type::Integer                                     },
      { "route_to_rpg",                              DataColumn::Type::Integer                                     },
      { "csu_loss_of_signal",                        DataColumn::Type::Integer                                     },
      { "csu_loss_of_frames",                        DataColumn::Type::Integer                                     },
      { "csu_yellow_alarms",                         DataColumn::Type::Integer                                     },
      { "csu_blue_alarms",                           DataColumn::Type::Integer                                     },
      { "csu_24hr_errored_seconds",                  DataColumn::Type::Integer                                     },
      { "csu_24hr_severely_errored_seconds",         DataColumn::Type::Integer                                     },
      { "csu_24hr_severely_errored_framing_seconds", DataColumn::Type::Integer                                     },
      { "csu_24hr_unavailable_seconds",              DataColumn::Type::Integer                                     },
      { "csu_24hr_controlled_slip_seconds",          DataColumn::Type::Integer                                     },
      { "csu_24hr_path_coding_violations",           DataColumn::Type::Integer                                     },
      { "csu_24hr_line_errored_seconds",             DataColumn::Type::Integer                                     },
      { "csu_24hr_bursty_errored_seconds",           DataColumn::Type::Integer                                     },
      { "csu_24hr_degraded_minutes",                 DataColumn::Type::Integer                                     },
      { "lan_switch_cpu_utilization",                DataColumn::Type::Integer                                     },
      { "lan_switch_memory_utilization",             DataColumn::Type::Integer                                     },
      { "ifdr_chasis_temperature",                   DataColumn::Type::Integer                                     },
      { "ifdr_fpga_temperature",                     DataColumn::Type::Integer                                     },
      { "gps_satellites",                            DataColumn::Type::Integer                                     },
      { "ipc_status",                                DataColumn::Type::Integer                                     },
      { "commanded_channel_control",                 DataColumn::Type::Integer                                     },
      { "polarization",                              DataColumn::Type::Integer                                     },
      { "ame_internal_temperature",                  DataColumn::Type::Float                                       },
      { "ame_receiver_module_temperature",           DataColumn::Type::Float                                       },
      { "ame_bite_cal_module_temperature",           DataColumn::Type::Float                                       },
      { "ame_peltier_pulse_width_modulation",        DataColumn::Type::Integer                                     },
      { "ame_peltier_status",                        DataColumn::Type::Integer                                     },
      { "ame_a_d_converter_status",                  DataColumn::Type::Integer                                     },
      { "ame_state",                                 DataColumn::Type::Integer                                     },
      { "ame_pos3dot3v_ps_voltage",                  DataColumn::Type::Float                                       },
      { "ame_pos5v_ps_voltage",                      DataColumn::Type::Float                                       },
      { "ame_pos6dot5v_ps_voltage",                  DataColumn::Type::Float                                       },
      { "ame_pos15v_ps_voltage",                     DataColumn::Type::Float                                       },
      { "ame_pos48v_ps_voltage",                     DataColumn::Type::Float                                       },
      { "ame_stalo_power",                           DataColumn::Type::Float                                       },
      { "peltier_current",                           DataColumn::Type::Float                                       },
      { "adc_calibration_reference_voltage",         DataColumn::Type::Float                                       },
      { "ame_mode",                                  DataColumn::Type::Integer                                     },
      { "ame_peltier_mode",                          DataColumn::Type::Integer                                     },
      { "ame_peltier_inside_fan_current",            DataColumn::Type::Float                                       },
      { "ame_peltier_outside_fan_current",           DataColumn::Type::Float                                       },
      { "horizontal_tr_limiter_voltage",             DataColumn::Type::Float                                       },
      { "vertical_tr_limiter_voltage",               DataColumn::Type::Float                                       },
      { "adc_calibration_offset_voltage",            DataColumn::Type::Float                                       },
      { "adc_calibration_gain_correction",           DataColumn::Type::Float                                       },
      { "rcp_status",                                DataColumn::Type::Integer                                     },
      { "rcp_string",                                DataColumn::Type::String                                      },
      { "spip_power_buttons",                        DataColumn::Type::Integer                                     },
      { "master_power_administrator_load",           DataColumn::Type::Float                                       },
      { "expansion_power_administrator_load",        DataColumn::Type::Float                                       },
      { "pos5_vdc_ps",                               DataColumn::Type::Integer                                     },
      { "pos15_vdc_ps",                              DataColumn::Type::Integer                                     },
      { "pos28_vdc_ps",                              DataColumn::Type::Integer                                     },
      { "neg15_vdc_ps",                              DataColumn::Type::Integer                                     },
      { "pos45_vdc_ps",                              DataColumn::Type::Integer                                     },
      { "filament_ps_voltage",                       DataColumn::Type::Integer                                     },
      { "vacuum_pump_ps_voltage",                    DataColumn::Type::Integer                                     },
      { "focus_coil_ps_voltage",                     DataColumn::Type::Integer                                     },
      { "filament_ps",                               DataColumn::Type::Integer                                     },
      { "klystron_warmup",                           DataColumn::Type::Integer                                     },
      { "transmitter_available",                     DataColumn::Type::Integer                                     },
      { "wg_switch_position",                        DataColumn::Type::Integer                                     },
      { "wg_pfn_transfer_interlock",                 DataColumn::Type::Integer                                     },
      { "maintenance_mode",                          DataColumn::Type::Integer                                     },
      { "maintenance_required",                      DataColumn::Type::Integer                                     },
      { "pfn_switch_position",                       DataColumn::Type::Integer                                     },
      { "modulator_overload",                        DataColumn::Type::Integer                                     },
      { "modulator_inv_current",                     DataColumn::Type::Integer                                     },
      { "modulator_switch_fail",                     DataColumn::Type::Integer                                     },
      { "main_power_voltage",                        DataColumn::Type::Integer                                     },
      { "charging_system_fail",                      DataColumn::Type::Integer                                     },
      { "inverse_diode_current",                     DataColumn::Type::Integer                                     },
      { "trigger_amplifier",                         DataColumn::Type::Integer                                     },
      { "circulator_temperature",                    DataColumn::Type::Integer                                     },
      { "spectrum_filter_pressure",                  DataColumn::Type::Integer                                     },
      { "wg_arc_vswr",                               DataColumn::Type::Integer                                     },
      { "cabinet_interlock",                         DataColumn::Type::Integer                                     },
      { "cabinet_air_temperature",                   DataColumn::Type::Integer                                     },
      { "cabinet_airflow",                           DataColumn::Type::Integer                                     },
      { "klystron_current",                          DataColumn::Type::Integer                                     },
      { "klystron_filament_current",                 DataColumn::Type::Integer                                     },
      { "klystron_vacion_current",                   DataColumn::Type::Integer                                     },
      { "klystron_air_temperature",                  DataColumn::Type::Integer                                     },
      { "klystron_airflow",                          DataColumn::Type::Integer                                     },
      { "modulator_switch_maintenance",              DataColumn::Type::Integer                                     },
      { "post_charge_regulator_maintenance",         DataColumn::Type::Integer                                     },
      { "wg_pressure_humidity",                      DataColumn::Type::Integer                                     },
      { "transmitter_overvoltage",                   DataColumn::Type::Integer                                     },
      { "transmitter_overcurrent",                   DataColumn::Type::Integer                                     },
      { "focus_coil_current",                        DataColumn::Type::Integer                                     },
      { "focus_coil_airflow",                        DataColumn::Type::Integer                                     },
      { "oil_temperature",                           DataColumn::Type::Integer                                     },
      { "prf_limit",                                 DataColumn::Type::Integer                                     },
      { "transmitter_oil_level",                     DataColumn::Type::Integer                                     },
      { "transmitter_battery_charging",              DataColumn::Type::Integer                                     },
      { "high_voltage_hv_status",                    DataColumn::Type::Integer                                     },
      { "transmitter_recycling_summary",             DataColumn::Type::Integer                                     },
      { "transmitter_inoperable",                    DataColumn::Type::Integer                                     },
      { "transmitter_air_filter",                    DataColumn::Type::Integer                                     },
      { "zero_test_bit_0",                           DataColumn::Type::Integer                                     },
      { "zero_test_bit_1",                           DataColumn::Type::Integer                                     },
      { "zero_test_bit_2",                           DataColumn::Type::Integer                                     },
      { "zero_test_bit_3",                           DataColumn::Type::Integer                                     },
      { "zero_test_bit_4",                           DataColumn::Type::Integer                                     },
      { "zero_test_bit_5",                           DataColumn::Type::Integer                                     },
      { "zero_test_bit_6",                           DataColumn::Type::Integer                                     },
      { "zero_test_bit_7",                           DataColumn::Type::Integer                                     },
      { "one_test_bit_0",                            DataColumn::Type::Integer                                     },
      { "one_test_bit_1",                            DataColumn::Type::Integer                                     },
      { "one_test_bit_2",                            DataColumn::Type::Integer                                     },
      { "one_test_bit_3",                            DataColumn::Type::Integer                                     },
      { "one_test_bit_4",                            DataColumn::Type::Integer                                     },
      { "one_test_bit_5",                            DataColumn::Type::Integer                                     },
      { "one_test_bit_6",                            DataColumn::Type::Integer                                     },
      { "one_test_bit_7",                            DataColumn::Type::Integer                                     },
      { "xmtr_spip_interface",                       DataColumn::Type::Integer                                     },
      { "transmitter_summary_status",                DataColumn::Type::Integer                                     },
      { "transmitter_rf_power_sensor",               DataColumn::Type::Float                                       },
      { "horizontal_xmtr_peak_power",                DataColumn::Type::Float                                       },
      { "xmtr_peak_power",                           DataColumn::Type::Float                                       },
      { "vertical_xmtr_peak_power",                  DataColumn::Type::Float                                       },
      { "xmtr_rf_avg_power",                         DataColumn::Type::Float                                       },
      { "xmtr_recycle_count",                        DataColumn::Type::Integer                                     },
      { "receiver_bias_measurement",                 DataColumn::Type::Float                                       },
      { "transmit_imbalance",                        DataColumn::Type::Float                                       },
      { "xmtr_power_meter_zero",                     DataColumn::Type::Float                                       },
      { "ac_unit_1_compressor_shut_off",             DataColumn::Type::Integer                                     },
      { "ac_unit_2_compressor_shut_off",             DataColumn::Type::Integer                                     },
      { "generator_maintenance_required",            DataColumn::Type::Integer                                     },
      { "generator_battery_voltage",                 DataColumn::Type::Integer                                     },
      { "generator_engine",                          DataColumn::Type::Integer                                     },
      { "generator_volt_frequency",                  DataColumn::Type::Integer                                     },
      { "power_source",                              DataColumn::Type::Integer                                     },
      { "transitional_power_source_tps",             DataColumn::Type::Integer                                     },
      { "generator_auto_run_off_switch",             DataColumn::Type::Integer                                     },
      { "aircraft_hazard_lighting",                  DataColumn::Type::Integer                                     },
      { "equipment_shelter_fire_detection_system",   DataColumn::Type::Integer                                     },
      { "equipment_shelter_fire_smoke",              DataColumn::Type::Integer                                     },
      { "generator_shelter_fire_smoke",              DataColumn::Type::Integer                                     },
      { "utility_voltage_frequency",                 DataColumn::Type::Integer                                     },
      { "site_security_alarm",                       DataColumn::Type::Integer                                     },
      { "security_equipment",                        DataColumn::Type::Integer                                     },
      { "security_system",                           DataColumn::Type::Integer                                     },
      { "receiver_connected_to_antenna",             DataColumn::Type::Integer                                     },
      { "radome_hatch",                              DataColumn::Type::Integer                                     },
      { "ac_unit_1_filter_dirty",                    DataColumn::Type::Integer                                     },
      { "ac_unit_2_filter_dirty",                    DataColumn::Type::Integer                                     },
      { "equipment_shelter_temperature",             DataColumn::Type::Float                                       },
      { "outside_ambient_temperature",               DataColumn::Type::Float                                       },
      { "transmitter_leaving_air_temp",              DataColumn::Type::Float                                       },
      { "ac_unit_1_discharge_air_temp",              DataColumn::Type::Float                                       },
      { "generator_shelter_temperature",             DataColumn::Type::Float                                       },
      { "radome_air_temperature",                    DataColumn::Type::Float                                       },
      { "ac_unit_2_discharge_air_temp",              DataColumn::Type::Float                                       },
      { "spip_pos15v_ps",                            DataColumn::Type::Float                                       },
      { "spip_neg15v_ps",                            DataColumn::Type::Float                                       },
      { "spip_pos28v_ps_status",                     DataColumn::Type::Integer                                     },
      { "spip_pos5v_ps",                             DataColumn::Type::Float                                       },
      { "converted_generator_fuel_level",            DataColumn::Type::Integer                                     },
      { "elevation_pos_dead_limit",                  DataColumn::Type::Integer                                     },
      { "pos150v_overvoltage",                       DataColumn::Type::Integer                                     },
      { "pos150v_undervoltage",                      DataColumn::Type::Integer                                     },
      { "elevation_servo_amp_inhibit",               DataColumn::Type::Integer                                     },
      { "elevation_servo_amp_short_circuit",         DataColumn::Type::Integer                                     },
      { "elevation_servo_amp_overtemp",              DataColumn::Type::Integer                                     },
      { "elevation_motor_overtemp",                  DataColumn::Type::Integer                                     },
      { "elevation_stow_pin",                        DataColumn::Type::Integer                                     },
      { "elevation_housing_5v_ps",                   DataColumn::Type::Integer                                     },
      { "elevation_negdead_limit",                   DataColumn::Type::Integer                                     },
      { "elevation_posnormal_limit",                 DataColumn::Type::Integer                                     },
      { "elevation_neg_normal_limit",                DataColumn::Type::Integer                                     },
      { "elevation_encoder_light",                   DataColumn::Type::Integer                                     },
      { "elevation_gearbox_oil",                     DataColumn::Type::Integer                                     },
      { "elevation_handwheel",                       DataColumn::Type::Integer                                     },
      { "elevation_amp_ps",                          DataColumn::Type::Integer                                     },
      { "azimuth_servo_amp_inhibit",                 DataColumn::Type::Integer                                     },
      { "azimuth_servo_amp_short_circuit",           DataColumn::Type::Integer                                     },
      { "azimuth_servo_amp_overtemp",                DataColumn::Type::Integer                                     },
      { "azimuth_motor_overtemp",                    DataColumn::Type::Integer                                     },
      { "azimuth_stow_pin",                          DataColumn::Type::Integer                                     },
      { "azimuth_housing_5v_ps",                     DataColumn::Type::Integer                                     },
      { "azimuth_encoder_light",                     DataColumn::Type::Integer                                     },
      { "azimuth_gearbox_oil",                       DataColumn::Type::Integer                                     },
      { "azimuth_bull_gear_oil",                     DataColumn::Type::Integer                                     },
      { "azimuth_handwheel",                         DataColumn::Type::Integer                                     },
      { "azimuth_servo_amp_ps",                      DataColumn::Type::Integer                                     },
      { "servo",                                     DataColumn::Type::Integer                                     },
      { "pedestal_interlock_switch",                 DataColumn::Type::Integer                                     },
      { "coho_clock",                                DataColumn::Type::Integer                                     },
      { "rf_generator_frequency_select_oscillator",  DataColumn::Type::Integer                                     },
      { "rf_generator_rf_stalo",                     DataColumn::Type::Integer                                     },
      { "rf_generator_phase_shifted_coho",           DataColumn::Type::Integer                                     },
      { "pos9v_receiver_ps",                         DataColumn::Type::Integer                                     },
      { "pos5v_receiver_ps",                         DataColumn::Type::Integer                                     },
      { "pos_or_neg18v_receiver_ps",                 DataColumn::Type::Integer                                     },
      { "neg9v_receiver_ps",                         DataColumn::Type::Integer                                     },
      { "pos5v_single_channel_rdaiu_ps",             DataColumn::Type::Integer                                     },
      { "horizontal_short_pulse_noise",              DataColumn::Type::Float                                       },
      { "horizontal_long_pulse_noise",               DataColumn::Type::Float                                       },
      { "horizontal_noise_temperature",              DataColumn::Type::Float                                       },
      { "vertical_short_pulse_noise",                DataColumn::Type::Float                                       },
      { "vertical_long_pulse_noise",                 DataColumn::Type::Float                                       },
      { "vertical_noise_temperature",                DataColumn::Type::Float                                       },
      { "horizontal_linearity",                      DataColumn::Type::Float                                       },
      { "horizontal_dynamic_range",                  DataColumn::Type::Float                                       },
      { "horizontal_delta_dbz0",                     DataColumn::Type::Float                                       },
      { "vertical_delta_dbz0",                       DataColumn::Type::Float                                       },
      { "kd_peak_measured",                          DataColumn::Type::Float                                       },
      { "short_pulse_horizontal_dbz0",               DataColumn::Type::Float                                       },
      { "long_pulse_horizontal_dbz0",                DataColumn::Type::Float                                       },
      { "velocity_processed",                        DataColumn::Type::Integer                                     },
      { "width_processed",                           DataColumn::Type::Integer                                     },
      { "velocity_rf_gen",                           DataColumn::Type::Integer                                     },
      { "width_rf_gen",                              DataColumn::Type::Integer                                     },
      { "horizontal_i0",                             DataColumn::Type::Float                                       },
      { "vertical_i0",                               DataColumn::Type::Float                                       },
      { "vertical_dynamic_range",                    DataColumn::Type::Float                                       },
      { "short_pulse_vertical_dbz0",                 DataColumn::Type::Float                                       },
      { "long_pulse_vertical_dbz0",                  DataColumn::Type::Float                                       },
      { "horizontal_power_sense",                    DataColumn::Type::Float                                       },
      { "vertical_power_sense",                      DataColumn::Type::Float                                       },
      { "zdr_bias",                                  DataColumn::Type::Float                                       },
      { "clutter_suppression_delta",                 DataColumn::Type::Float                                       },
      { "clutter_suppression_unfiltered_power",      DataColumn::Type::Float                                       },
      { "clutter_suppression_filtered_power",        DataColumn::Type::Float                                       },
      { "vertical_linearity",                        DataColumn::Type::Float                                       },
      { "state_file_read_status",                    DataColumn::Type::Integer                                     },
      { "state_file_write_status",                   DataColumn::Type::Integer                                     },
      { "bypass_map_file_read_status",               DataColumn::Type::Integer                                     },
      { "bypass_map_file_write_status",              DataColumn::Type::Integer                                     },
      { "current_adaptation_file_read_status",       DataColumn::Type::Integer                                     },
      { "current_adaptation_file_write_status",      DataColumn::Type::Integer                                     },
      { "censor_zone_file_read_status",              DataColumn::Type::Integer                                     },
      { "censor_zone_file_write_status",             DataColumn::Type::Integer                                     },
      { "remote_vcp_file_read_status",               DataColumn::Type::Integer                                     },
      { "remote_vcp_file_write_status",              DataColumn::Type::Integer                                     },
      { "baseline_adaptation_file_read_status",      DataColumn::Type::Integer                                     },
      { "read_status_of_prf_sets",                   DataColumn::Type::Integer                                     },
      { "clutter_filter_map_file_read_status",       DataColumn::Type::Integer                                     },
      { "clutter_filter_map_file_write_status",      DataColumn::Type::Integer                                     },
      { "general_disk_io_error",                     DataColumn::Type::Integer                                     },
      { "rsp_status",                                DataColumn::Type::Integer                                     },
      { "motherboard_temperature",                   DataColumn::Type::Integer                                     },
      { "cpu_1_temperature",                         DataColumn::Type::Integer                                     },
      { "cpu_2_temperature",                         DataColumn::Type::Integer                                     },
      { "cpu_1_fan_speed",                           DataColumn::Type::Integer                                     },
      { "cpu_2_fan_speed",                           DataColumn::Type::Integer                                     },
      { "rsp_fan_1_speed",                           DataColumn::Type::Integer                                     },
      { "rsp_fan_2_speed",                           DataColumn::Type::Integer                                     },
      { "rsp_fan_3_speed",                           DataColumn::Type::Integer                                     },
      { "spip_comm_status",                          DataColumn::Type::Integer                                     },
      { "hci_comm_status",                           DataColumn::Type::Integer                                     },
      { "signal_processor_command_status",           DataColumn::Type::Integer                                     },
      { "ame_communication_status",                  DataColumn::Type::Integer                                     },
      { "rms_link_status",                           DataColumn::Type::Integer                                     },
      { "rpg_link_status",                           DataColumn::Type::Integer                                     },
      { "interpanel_link_status",                    DataColumn::Type::Integer                                     },
      { "performance_check_time",                    DataColumn::Type::Integer                                     },
      { "version",                                   DataColumn::Type::Integer                                     }
    };
    table->setSchema(schema);

    std::map<std::string, DataValue> row;
    row["loop_back_test_status"]     = static_cast<int>(msg3.communications.loop_back_test_status);
    row["t1_output_frames"]          = static_cast<int>(msg3.communications.t1_output_frames);
    row["t1_input_frames"]           = static_cast<int>(msg3.communications.t1_input_frames);
    row["router_memory_used"]        = static_cast<int>(msg3.communications.router_memory_used);
    row["router_memory_free"]        = static_cast<int>(msg3.communications.router_memory_free);
    row["router_memory_utilization"] = static_cast<int>(msg3.communications.router_memory_utilization);
    row["route_to_rpg"]             = static_cast<int>(msg3.communications.route_to_rpg);
    row["csu_loss_of_signal"]       = static_cast<int>(msg3.communications.csu_loss_of_signal);
    row["csu_loss_of_frames"]       = static_cast<int>(msg3.communications.csu_loss_of_frames);
    row["csu_yellow_alarms"]        = static_cast<int>(msg3.communications.csu_yellow_alarms);
    row["csu_blue_alarms"]          = static_cast<int>(msg3.communications.csu_blue_alarms);
    row["csu_24hr_errored_seconds"] = static_cast<int>(msg3.communications.csu_24hr_errored_seconds);
    row["csu_24hr_severely_errored_seconds"] =
      static_cast<int>(msg3.communications.csu_24hr_severely_errored_seconds);
    row["csu_24hr_severely_errored_framing_seconds"] =
      static_cast<int>(msg3.communications.csu_24hr_severely_errored_framing_seconds);
    row["csu_24hr_unavailable_seconds"]     = static_cast<int>(msg3.communications.csu_24hr_unavailable_seconds);
    row["csu_24hr_controlled_slip_seconds"] = static_cast<int>(msg3.communications.csu_24hr_controlled_slip_seconds);
    row["csu_24hr_path_coding_violations"]  = static_cast<int>(msg3.communications.csu_24hr_path_coding_violations);
    row["csu_24hr_line_errored_seconds"]    = static_cast<int>(msg3.communications.csu_24hr_line_errored_seconds);
    row["csu_24hr_bursty_errored_seconds"]  = static_cast<int>(msg3.communications.csu_24hr_bursty_errored_seconds);
    row["csu_24hr_degraded_minutes"]        = static_cast<int>(msg3.communications.csu_24hr_degraded_minutes);
    row["lan_switch_cpu_utilization"]       = static_cast<int>(msg3.communications.lan_switch_cpu_utilization);
    row["lan_switch_memory_utilization"]    = static_cast<int>(msg3.communications.lan_switch_memory_utilization);
    row["ifdr_chasis_temperature"] = static_cast<int>(msg3.communications.ifdr_chasis_temperature);
    row["ifdr_fpga_temperature"]   = static_cast<int>(msg3.communications.ifdr_fpga_temperature);
    row["gps_satellites"] = static_cast<int>(msg3.communications.gps_satellites);
    row["ipc_status"]     = static_cast<int>(msg3.communications.ipc_status);
    row["commanded_channel_control"] = static_cast<int>(msg3.communications.commanded_channel_control);
    row["polarization"] = static_cast<int>(msg3.ame.polarization);
    row["ame_internal_temperature"]           = msg3.ame.ame_internal_temperature;
    row["ame_receiver_module_temperature"]    = msg3.ame.ame_receiver_module_temperature;
    row["ame_bite_cal_module_temperature"]    = msg3.ame.ame_bite_cal_module_temperature;
    row["ame_peltier_pulse_width_modulation"] = static_cast<int>(msg3.ame.ame_peltier_pulse_width_modulation);
    row["ame_peltier_status"]       = static_cast<int>(msg3.ame.ame_peltier_status);
    row["ame_a_d_converter_status"] = static_cast<int>(msg3.ame.ame_a_d_converter_status);
    row["ame_state"] = static_cast<int>(msg3.ame.ame_state);
    row["ame_pos3dot3v_ps_voltage"] = msg3.ame.ame_pos3dot3v_ps_voltage;
    row["ame_pos5v_ps_voltage"]     = msg3.ame.ame_pos5v_ps_voltage;
    row["ame_pos6dot5v_ps_voltage"] = msg3.ame.ame_pos6dot5v_ps_voltage;
    row["ame_pos15v_ps_voltage"]    = msg3.ame.ame_pos15v_ps_voltage;
    row["ame_pos48v_ps_voltage"]    = msg3.ame.ame_pos48v_ps_voltage;
    row["ame_stalo_power"] = msg3.ame.ame_stalo_power;
    row["peltier_current"] = msg3.ame.peltier_current;
    row["adc_calibration_reference_voltage"] = msg3.ame.adc_calibration_reference_voltage;
    row["ame_mode"]         = static_cast<int>(msg3.ame.ame_mode);
    row["ame_peltier_mode"] = static_cast<int>(msg3.ame.ame_peltier_mode);
    row["ame_peltier_inside_fan_current"]  = msg3.ame.ame_peltier_inside_fan_current;
    row["ame_peltier_outside_fan_current"] = msg3.ame.ame_peltier_outside_fan_current;
    row["horizontal_tr_limiter_voltage"]   = msg3.ame.horizontal_tr_limiter_voltage;
    row["vertical_tr_limiter_voltage"]     = msg3.ame.vertical_tr_limiter_voltage;
    row["adc_calibration_offset_voltage"]  = msg3.ame.adc_calibration_offset_voltage;
    row["adc_calibration_gain_correction"] = msg3.ame.adc_calibration_gain_correction;
    row["rcp_status"] = static_cast<int>(msg3.power_button_status.rcp_status);
    row["rcp_string"] =
      std::string(msg3.power_button_status.rcp_string,
        strnlen(msg3.power_button_status.rcp_string, sizeof(msg3.power_button_status.rcp_string)));
    row["spip_power_buttons"] = static_cast<int>(msg3.power_button_status.spip_power_buttons);
    row["master_power_administrator_load"]    = msg3.power.master_power_administrator_load;
    row["expansion_power_administrator_load"] = msg3.power.expansion_power_administrator_load;
    row["pos5_vdc_ps"]                       = static_cast<int>(msg3.transmitter.pos5_vdc_ps);
    row["pos15_vdc_ps"]                      = static_cast<int>(msg3.transmitter.pos15_vdc_ps);
    row["pos28_vdc_ps"]                      = static_cast<int>(msg3.transmitter.pos28_vdc_ps);
    row["neg15_vdc_ps"]                      = static_cast<int>(msg3.transmitter.neg15_vdc_ps);
    row["pos45_vdc_ps"]                      = static_cast<int>(msg3.transmitter.pos45_vdc_ps);
    row["filament_ps_voltage"]               = static_cast<int>(msg3.transmitter.filament_ps_voltage);
    row["vacuum_pump_ps_voltage"]            = static_cast<int>(msg3.transmitter.vacuum_pump_ps_voltage);
    row["focus_coil_ps_voltage"]             = static_cast<int>(msg3.transmitter.focus_coil_ps_voltage);
    row["filament_ps"]                       = static_cast<int>(msg3.transmitter.filament_ps);
    row["klystron_warmup"]                   = static_cast<int>(msg3.transmitter.klystron_warmup);
    row["transmitter_available"]             = static_cast<int>(msg3.transmitter.transmitter_available);
    row["wg_switch_position"]                = static_cast<int>(msg3.transmitter.wg_switch_position);
    row["wg_pfn_transfer_interlock"]         = static_cast<int>(msg3.transmitter.wg_pfn_transfer_interlock);
    row["maintenance_mode"]                  = static_cast<int>(msg3.transmitter.maintenance_mode);
    row["maintenance_required"]              = static_cast<int>(msg3.transmitter.maintenance_required);
    row["pfn_switch_position"]               = static_cast<int>(msg3.transmitter.pfn_switch_position);
    row["modulator_overload"]                = static_cast<int>(msg3.transmitter.modulator_overload);
    row["modulator_inv_current"]             = static_cast<int>(msg3.transmitter.modulator_inv_current);
    row["modulator_switch_fail"]             = static_cast<int>(msg3.transmitter.modulator_switch_fail);
    row["main_power_voltage"]                = static_cast<int>(msg3.transmitter.main_power_voltage);
    row["charging_system_fail"]              = static_cast<int>(msg3.transmitter.charging_system_fail);
    row["inverse_diode_current"]             = static_cast<int>(msg3.transmitter.inverse_diode_current);
    row["trigger_amplifier"]                 = static_cast<int>(msg3.transmitter.trigger_amplifier);
    row["circulator_temperature"]            = static_cast<int>(msg3.transmitter.circulator_temperature);
    row["spectrum_filter_pressure"]          = static_cast<int>(msg3.transmitter.spectrum_filter_pressure);
    row["wg_arc_vswr"]                       = static_cast<int>(msg3.transmitter.wg_arc_vswr);
    row["cabinet_interlock"]                 = static_cast<int>(msg3.transmitter.cabinet_interlock);
    row["cabinet_air_temperature"]           = static_cast<int>(msg3.transmitter.cabinet_air_temperature);
    row["cabinet_airflow"]                   = static_cast<int>(msg3.transmitter.cabinet_airflow);
    row["klystron_current"]                  = static_cast<int>(msg3.transmitter.klystron_current);
    row["klystron_filament_current"]         = static_cast<int>(msg3.transmitter.klystron_filament_current);
    row["klystron_vacion_current"]           = static_cast<int>(msg3.transmitter.klystron_vacion_current);
    row["klystron_air_temperature"]          = static_cast<int>(msg3.transmitter.klystron_air_temperature);
    row["klystron_airflow"]                  = static_cast<int>(msg3.transmitter.klystron_airflow);
    row["modulator_switch_maintenance"]      = static_cast<int>(msg3.transmitter.modulator_switch_maintenance);
    row["post_charge_regulator_maintenance"] = static_cast<int>(msg3.transmitter.post_charge_regulator_maintenance);
    row["wg_pressure_humidity"]              = static_cast<int>(msg3.transmitter.wg_pressure_humidity);
    row["transmitter_overvoltage"]           = static_cast<int>(msg3.transmitter.transmitter_overvoltage);
    row["transmitter_overcurrent"]           = static_cast<int>(msg3.transmitter.transmitter_overcurrent);
    row["focus_coil_current"]                = static_cast<int>(msg3.transmitter.focus_coil_current);
    row["focus_coil_airflow"]                = static_cast<int>(msg3.transmitter.focus_coil_airflow);
    row["oil_temperature"]                   = static_cast<int>(msg3.transmitter.oil_temperature);
    row["prf_limit"] = static_cast<int>(msg3.transmitter.prf_limit);
    row["transmitter_oil_level"]         = static_cast<int>(msg3.transmitter.transmitter_oil_level);
    row["transmitter_battery_charging"]  = static_cast<int>(msg3.transmitter.transmitter_battery_charging);
    row["high_voltage_hv_status"]        = static_cast<int>(msg3.transmitter.high_voltage_hv_status);
    row["transmitter_recycling_summary"] = static_cast<int>(msg3.transmitter.transmitter_recycling_summary);
    row["transmitter_inoperable"]        = static_cast<int>(msg3.transmitter.transmitter_inoperable);
    row["transmitter_air_filter"]        = static_cast<int>(msg3.transmitter.transmitter_air_filter);
    row["zero_test_bit_0"]                = static_cast<int>(msg3.transmitter.zero_test_bit_0);
    row["zero_test_bit_1"]                = static_cast<int>(msg3.transmitter.zero_test_bit_1);
    row["zero_test_bit_2"]                = static_cast<int>(msg3.transmitter.zero_test_bit_2);
    row["zero_test_bit_3"]                = static_cast<int>(msg3.transmitter.zero_test_bit_3);
    row["zero_test_bit_4"]                = static_cast<int>(msg3.transmitter.zero_test_bit_4);
    row["zero_test_bit_5"]                = static_cast<int>(msg3.transmitter.zero_test_bit_5);
    row["zero_test_bit_6"]                = static_cast<int>(msg3.transmitter.zero_test_bit_6);
    row["zero_test_bit_7"]                = static_cast<int>(msg3.transmitter.zero_test_bit_7);
    row["one_test_bit_0"]                 = static_cast<int>(msg3.transmitter.one_test_bit_0);
    row["one_test_bit_1"]                 = static_cast<int>(msg3.transmitter.one_test_bit_1);
    row["one_test_bit_2"]                 = static_cast<int>(msg3.transmitter.one_test_bit_2);
    row["one_test_bit_3"]                 = static_cast<int>(msg3.transmitter.one_test_bit_3);
    row["one_test_bit_4"]                 = static_cast<int>(msg3.transmitter.one_test_bit_4);
    row["one_test_bit_5"]                 = static_cast<int>(msg3.transmitter.one_test_bit_5);
    row["one_test_bit_6"]                 = static_cast<int>(msg3.transmitter.one_test_bit_6);
    row["one_test_bit_7"]                 = static_cast<int>(msg3.transmitter.one_test_bit_7);
    row["xmtr_spip_interface"]            = static_cast<int>(msg3.transmitter.xmtr_spip_interface);
    row["transmitter_summary_status"]     = static_cast<int>(msg3.transmitter.transmitter_summary_status);
    row["transmitter_rf_power_sensor"]    = msg3.transmitter.transmitter_rf_power_sensor;
    row["horizontal_xmtr_peak_power"]     = msg3.transmitter.horizontal_xmtr_peak_power;
    row["xmtr_peak_power"]                = msg3.transmitter.xmtr_peak_power;
    row["vertical_xmtr_peak_power"]       = msg3.transmitter.vertical_xmtr_peak_power;
    row["xmtr_rf_avg_power"]              = msg3.transmitter.xmtr_rf_avg_power;
    row["xmtr_recycle_count"]             = static_cast<int>(msg3.transmitter.xmtr_recycle_count);
    row["receiver_bias_measurement"]      = msg3.transmitter.receiver_bias_measurement;
    row["transmit_imbalance"]             = msg3.transmitter.transmit_imbalance;
    row["xmtr_power_meter_zero"]          = msg3.transmitter.xmtr_power_meter_zero;
    row["ac_unit_1_compressor_shut_off"]  = static_cast<int>(msg3.tower_utilities.ac_unit_1_compressor_shut_off);
    row["ac_unit_2_compressor_shut_off"]  = static_cast<int>(msg3.tower_utilities.ac_unit_2_compressor_shut_off);
    row["generator_maintenance_required"] = static_cast<int>(msg3.tower_utilities.generator_maintenance_required);
    row["generator_battery_voltage"]      = static_cast<int>(msg3.tower_utilities.generator_battery_voltage);
    row["generator_engine"]               = static_cast<int>(msg3.tower_utilities.generator_engine);
    row["generator_volt_frequency"]       = static_cast<int>(msg3.tower_utilities.generator_volt_frequency);
    row["power_source"] = static_cast<int>(msg3.tower_utilities.power_source);
    row["transitional_power_source_tps"] = static_cast<int>(msg3.tower_utilities.transitional_power_source_tps);
    row["generator_auto_run_off_switch"] = static_cast<int>(msg3.tower_utilities.generator_auto_run_off_switch);
    row["aircraft_hazard_lighting"]      = static_cast<int>(msg3.tower_utilities.aircraft_hazard_lighting);
    row["equipment_shelter_fire_detection_system"] =
      static_cast<int>(msg3.equipment_shelter.equipment_shelter_fire_detection_system);
    row["equipment_shelter_fire_smoke"] = static_cast<int>(msg3.equipment_shelter.equipment_shelter_fire_smoke);
    row["generator_shelter_fire_smoke"] = static_cast<int>(msg3.equipment_shelter.generator_shelter_fire_smoke);
    row["utility_voltage_frequency"]    = static_cast<int>(msg3.equipment_shelter.utility_voltage_frequency);
    row["site_security_alarm"] = static_cast<int>(msg3.equipment_shelter.site_security_alarm);
    row["security_equipment"]  = static_cast<int>(msg3.equipment_shelter.security_equipment);
    row["security_system"]     = static_cast<int>(msg3.equipment_shelter.security_system);
    row["receiver_connected_to_antenna"] = static_cast<int>(msg3.equipment_shelter.receiver_connected_to_antenna);
    row["radome_hatch"] = static_cast<int>(msg3.equipment_shelter.radome_hatch);
    row["ac_unit_1_filter_dirty"]        = static_cast<int>(msg3.equipment_shelter.ac_unit_1_filter_dirty);
    row["ac_unit_2_filter_dirty"]        = static_cast<int>(msg3.equipment_shelter.ac_unit_2_filter_dirty);
    row["equipment_shelter_temperature"] = msg3.equipment_shelter.equipment_shelter_temperature;
    row["outside_ambient_temperature"]   = msg3.equipment_shelter.outside_ambient_temperature;
    row["transmitter_leaving_air_temp"]  = msg3.equipment_shelter.transmitter_leaving_air_temp;
    row["ac_unit_1_discharge_air_temp"]  = msg3.equipment_shelter.ac_unit_1_discharge_air_temp;
    row["generator_shelter_temperature"] = msg3.equipment_shelter.generator_shelter_temperature;
    row["radome_air_temperature"]        = msg3.equipment_shelter.radome_air_temperature;
    row["ac_unit_2_discharge_air_temp"]  = msg3.equipment_shelter.ac_unit_2_discharge_air_temp;
    row["spip_pos15v_ps"]        = msg3.equipment_shelter.spip_pos15v_ps;
    row["spip_neg15v_ps"]        = msg3.equipment_shelter.spip_neg15v_ps;
    row["spip_pos28v_ps_status"] = static_cast<int>(msg3.equipment_shelter.spip_pos28v_ps_status);
    row["spip_pos5v_ps"]         = msg3.equipment_shelter.spip_pos5v_ps;
    row["converted_generator_fuel_level"] = static_cast<int>(msg3.equipment_shelter.converted_generator_fuel_level);
    row["elevation_pos_dead_limit"]       = static_cast<int>(msg3.antenna_pedestal.elevation_pos_dead_limit);
    row["pos150v_overvoltage"]               = static_cast<int>(msg3.antenna_pedestal.pos150v_overvoltage);
    row["pos150v_undervoltage"]              = static_cast<int>(msg3.antenna_pedestal.pos150v_undervoltage);
    row["elevation_servo_amp_inhibit"]       = static_cast<int>(msg3.antenna_pedestal.elevation_servo_amp_inhibit);
    row["elevation_servo_amp_short_circuit"] =
      static_cast<int>(msg3.antenna_pedestal.elevation_servo_amp_short_circuit);
    row["elevation_servo_amp_overtemp"] = static_cast<int>(msg3.antenna_pedestal.elevation_servo_amp_overtemp);
    row["elevation_motor_overtemp"]     = static_cast<int>(msg3.antenna_pedestal.elevation_motor_overtemp);
    row["elevation_stow_pin"]         = static_cast<int>(msg3.antenna_pedestal.elevation_stow_pin);
    row["elevation_housing_5v_ps"]    = static_cast<int>(msg3.antenna_pedestal.elevation_housing_5v_ps);
    row["elevation_negdead_limit"]    = static_cast<int>(msg3.antenna_pedestal.elevation_negdead_limit);
    row["elevation_posnormal_limit"]  = static_cast<int>(msg3.antenna_pedestal.elevation_posnormal_limit);
    row["elevation_neg_normal_limit"] = static_cast<int>(msg3.antenna_pedestal.elevation_neg_normal_limit);
    row["elevation_encoder_light"]    = static_cast<int>(msg3.antenna_pedestal.elevation_encoder_light);
    row["elevation_gearbox_oil"]      = static_cast<int>(msg3.antenna_pedestal.elevation_gearbox_oil);
    row["elevation_handwheel"]        = static_cast<int>(msg3.antenna_pedestal.elevation_handwheel);
    row["elevation_amp_ps"] = static_cast<int>(msg3.antenna_pedestal.elevation_amp_ps);
    row["azimuth_servo_amp_inhibit"]       = static_cast<int>(msg3.antenna_pedestal.azimuth_servo_amp_inhibit);
    row["azimuth_servo_amp_short_circuit"] = static_cast<int>(msg3.antenna_pedestal.azimuth_servo_amp_short_circuit);
    row["azimuth_servo_amp_overtemp"]      = static_cast<int>(msg3.antenna_pedestal.azimuth_servo_amp_overtemp);
    row["azimuth_motor_overtemp"] = static_cast<int>(msg3.antenna_pedestal.azimuth_motor_overtemp);
    row["azimuth_stow_pin"]       = static_cast<int>(msg3.antenna_pedestal.azimuth_stow_pin);
    row["azimuth_housing_5v_ps"]  = static_cast<int>(msg3.antenna_pedestal.azimuth_housing_5v_ps);
    row["azimuth_encoder_light"]  = static_cast<int>(msg3.antenna_pedestal.azimuth_encoder_light);
    row["azimuth_gearbox_oil"]    = static_cast<int>(msg3.antenna_pedestal.azimuth_gearbox_oil);
    row["azimuth_bull_gear_oil"]  = static_cast<int>(msg3.antenna_pedestal.azimuth_bull_gear_oil);
    row["azimuth_handwheel"]      = static_cast<int>(msg3.antenna_pedestal.azimuth_handwheel);
    row["azimuth_servo_amp_ps"]   = static_cast<int>(msg3.antenna_pedestal.azimuth_servo_amp_ps);
    row["servo"] = static_cast<int>(msg3.antenna_pedestal.servo);
    row["pedestal_interlock_switch"] = static_cast<int>(msg3.antenna_pedestal.pedestal_interlock_switch);
    row["coho_clock"] = static_cast<int>(msg3.rf_generator_receiver.coho_clock);
    row["rf_generator_frequency_select_oscillator"] =
      static_cast<int>(msg3.rf_generator_receiver.rf_generator_frequency_select_oscillator);
    row["rf_generator_rf_stalo"] = static_cast<int>(msg3.rf_generator_receiver.rf_generator_rf_stalo);
    row["rf_generator_phase_shifted_coho"] =
      static_cast<int>(msg3.rf_generator_receiver.rf_generator_phase_shifted_coho);
    row["pos9v_receiver_ps"]             = static_cast<int>(msg3.rf_generator_receiver.pos9v_receiver_ps);
    row["pos5v_receiver_ps"]             = static_cast<int>(msg3.rf_generator_receiver.pos5v_receiver_ps);
    row["pos_or_neg18v_receiver_ps"]     = static_cast<int>(msg3.rf_generator_receiver.pos_or_neg18v_receiver_ps);
    row["neg9v_receiver_ps"]             = static_cast<int>(msg3.rf_generator_receiver.neg9v_receiver_ps);
    row["pos5v_single_channel_rdaiu_ps"] = static_cast<int>(msg3.rf_generator_receiver.pos5v_single_channel_rdaiu_ps);
    row["horizontal_short_pulse_noise"]  = msg3.rf_generator_receiver.horizontal_short_pulse_noise;
    row["horizontal_long_pulse_noise"]   = msg3.rf_generator_receiver.horizontal_long_pulse_noise;
    row["horizontal_noise_temperature"]  = msg3.rf_generator_receiver.horizontal_noise_temperature;
    row["vertical_short_pulse_noise"]    = msg3.rf_generator_receiver.vertical_short_pulse_noise;
    row["vertical_long_pulse_noise"]     = msg3.rf_generator_receiver.vertical_long_pulse_noise;
    row["vertical_noise_temperature"]    = msg3.rf_generator_receiver.vertical_noise_temperature;
    row["horizontal_linearity"]          = msg3.calibration.horizontal_linearity;
    row["horizontal_dynamic_range"]      = msg3.calibration.horizontal_dynamic_range;
    row["horizontal_delta_dbz0"]         = msg3.calibration.horizontal_delta_dbz0;
    row["vertical_delta_dbz0"]           = msg3.calibration.vertical_delta_dbz0;
    row["kd_peak_measured"] = msg3.calibration.kd_peak_measured;
    row["short_pulse_horizontal_dbz0"] = msg3.calibration.short_pulse_horizontal_dbz0;
    row["long_pulse_horizontal_dbz0"]  = msg3.calibration.long_pulse_horizontal_dbz0;
    row["velocity_processed"]        = static_cast<int>(msg3.calibration.velocity_processed);
    row["width_processed"]           = static_cast<int>(msg3.calibration.width_processed);
    row["velocity_rf_gen"]           = static_cast<int>(msg3.calibration.velocity_rf_gen);
    row["width_rf_gen"]              = static_cast<int>(msg3.calibration.width_rf_gen);
    row["horizontal_i0"]             = msg3.calibration.horizontal_i0;
    row["vertical_i0"]               = msg3.calibration.vertical_i0;
    row["vertical_dynamic_range"]    = msg3.calibration.vertical_dynamic_range;
    row["short_pulse_vertical_dbz0"] = msg3.calibration.short_pulse_vertical_dbz0;
    row["long_pulse_vertical_dbz0"]  = msg3.calibration.long_pulse_vertical_dbz0;
    row["horizontal_power_sense"]    = msg3.calibration.horizontal_power_sense;
    row["vertical_power_sense"]      = msg3.calibration.vertical_power_sense;
    row["zdr_bias"] = msg3.calibration.zdr_bias;
    row["clutter_suppression_delta"] = msg3.calibration.clutter_suppression_delta;
    row["clutter_suppression_unfiltered_power"] = msg3.calibration.clutter_suppression_unfiltered_power;
    row["clutter_suppression_filtered_power"]   = msg3.calibration.clutter_suppression_filtered_power;
    row["vertical_linearity"]                  = msg3.calibration.vertical_linearity;
    row["state_file_read_status"]              = static_cast<int>(msg3.file_status.state_file_read_status);
    row["state_file_write_status"]             = static_cast<int>(msg3.file_status.state_file_write_status);
    row["bypass_map_file_read_status"]         = static_cast<int>(msg3.file_status.bypass_map_file_read_status);
    row["bypass_map_file_write_status"]        = static_cast<int>(msg3.file_status.bypass_map_file_write_status);
    row["current_adaptation_file_read_status"] =
      static_cast<int>(msg3.file_status.current_adaptation_file_read_status);
    row["current_adaptation_file_write_status"] =
      static_cast<int>(msg3.file_status.current_adaptation_file_write_status);
    row["censor_zone_file_read_status"]         = static_cast<int>(msg3.file_status.censor_zone_file_read_status);
    row["censor_zone_file_write_status"]        = static_cast<int>(msg3.file_status.censor_zone_file_write_status);
    row["remote_vcp_file_read_status"]          = static_cast<int>(msg3.file_status.remote_vcp_file_read_status);
    row["remote_vcp_file_write_status"]         = static_cast<int>(msg3.file_status.remote_vcp_file_write_status);
    row["baseline_adaptation_file_read_status"] =
      static_cast<int>(msg3.file_status.baseline_adaptation_file_read_status);
    row["read_status_of_prf_sets"] = static_cast<int>(msg3.file_status.read_status_of_prf_sets);
    row["clutter_filter_map_file_read_status"] =
      static_cast<int>(msg3.file_status.clutter_filter_map_file_read_status);
    row["clutter_filter_map_file_write_status"] =
      static_cast<int>(msg3.file_status.clutter_filter_map_file_write_status);
    row["general_disk_io_error"] = static_cast<int>(msg3.file_status.general_disk_io_error);
    row["rsp_status"] = static_cast<int>(msg3.file_status.rsp_status);
    row["motherboard_temperature"]         = static_cast<int>(msg3.file_status.motherboard_temperature);
    row["cpu_1_temperature"]               = static_cast<int>(msg3.file_status.cpu_1_temperature);
    row["cpu_2_temperature"]               = static_cast<int>(msg3.file_status.cpu_2_temperature);
    row["cpu_1_fan_speed"]                 = static_cast<int>(msg3.file_status.cpu_1_fan_speed);
    row["cpu_2_fan_speed"]                 = static_cast<int>(msg3.file_status.cpu_2_fan_speed);
    row["rsp_fan_1_speed"]                 = static_cast<int>(msg3.file_status.rsp_fan_1_speed);
    row["rsp_fan_2_speed"]                 = static_cast<int>(msg3.file_status.rsp_fan_2_speed);
    row["rsp_fan_3_speed"]                 = static_cast<int>(msg3.file_status.rsp_fan_3_speed);
    row["spip_comm_status"]                = static_cast<int>(msg3.device_status.spip_comm_status);
    row["hci_comm_status"]                 = static_cast<int>(msg3.device_status.hci_comm_status);
    row["signal_processor_command_status"] = static_cast<int>(msg3.device_status.signal_processor_command_status);
    row["ame_communication_status"]        = static_cast<int>(msg3.device_status.ame_communication_status);
    row["rms_link_status"]                 = static_cast<int>(msg3.device_status.rms_link_status);
    row["rpg_link_status"]                 = static_cast<int>(msg3.device_status.rpg_link_status);
    row["interpanel_link_status"]          = static_cast<int>(msg3.device_status.interpanel_link_status);
    row["performance_check_time"]          = static_cast<int>(msg3.device_status.performance_check_time);
    row["version"] = static_cast<int>(msg3.device_status.version);

    table->addRow(row);
    myPendingDataTables.push_back(table);
    myPendingMsg3.reset();
  }
} // NEXRADAccumulator::materializePendingTables
} // namespace rapio
