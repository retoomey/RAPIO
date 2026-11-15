/**
 * @file rTDWRDecoder.cc
 * @brief Modern C++17 binary decoder implementation for TDWR records.
 */

#include "rTDWRDecoder.h"
#include "rTDWRAccumulator.h"
#include <rIOIndex.h>
#include <rIODataType.h>
#include <rPluginNotifier.h>
#include <rFMLRecordNotifier.h>
#include <rError.h>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace rapio {
TDWRDecoder::TDWRDecoder()
  : myAccumulator(std::make_unique<TDWRAccumulator>())
{
  myAccumulator->setKeepCompletedRadialSets(false);
  myAccumulator->setSweepCallback([this](const std::vector<std::shared_ptr<DataType> >& sweepOutputs) {
      if (mySweepCallback) {
        mySweepCallback(sweepOutputs);
      }
    });
}

TDWRDecoder::~TDWRDecoder() = default;

void
TDWRDecoder::setRadarName(const std::string& name)
{
  myRadarName = name;
  if (myAccumulator) {
    myAccumulator->setRadarName(name);
  }
}

void
TDWRDecoder::setUserVCP(int vcp)
{
  myUserVCP = vcp;
  if (myAccumulator) {
    myAccumulator->setUserVCP(vcp);
  }
}

void
TDWRDecoder::setUseVolumeTime(bool flag)
{
  myUseVolumeTime = flag;
  if (myAccumulator) {
    myAccumulator->setUseVolumeTime(flag);
  }
}

void
TDWRDecoder::setNoiseThreshold(float db)
{
  myNoiseThreshold = db;
  if (myAccumulator) {
    myAccumulator->setNoiseThreshold(db);
  }
}

void
TDWRDecoder::setMagneticOffset(const std::string& offsetStr)
{
  myMagneticOffsetStr = offsetStr;
  if (myAccumulator && !offsetStr.empty()) {
    myAccumulator->setMagneticOffset(offsetStr);
  }
}

void
TDWRDecoder::setFrequency(int freqMHz)
{
  myFrequencyMHz = freqMHz;
  if (myAccumulator) {
    myAccumulator->setFrequency(freqMHz);
  }
}

void
TDWRDecoder::setAlignmentResolution(float res)
{
  myAlignmentResolution = res;
  if (myAccumulator) {
    myAccumulator->setAlignmentResolution(res);
  }
}

float
TDWRDecoder::getAlignmentResolution() const
{
  return myAlignmentResolution;
}

void
TDWRDecoder::setKeepCompletedRadialSets(bool keep)
{
  if (myAccumulator) {
    myAccumulator->setKeepCompletedRadialSets(keep);
  }
}

void
TDWRDecoder::setSweepCallback(SweepCallback cb)
{
  mySweepCallback = std::move(cb);
}

void
TDWRDecoder::compactMessageBuffer()
{
  if (myMessageReadPos > 0) {
    if (myMessageReadPos >= myMessageBuffer.size()) {
      myMessageBuffer.clear();
    } else {
      size_t unread = myMessageBuffer.size() - myMessageReadPos;
      std::memmove(myMessageBuffer.data(), myMessageBuffer.data() + myMessageReadPos, unread);
      myMessageBuffer.resize(unread);
    }
    myMessageReadPos = 0;
  }
}

void
TDWRDecoder::processBuffer(const std::vector<char>& data)
{
  processBuffer(data.data(), data.size());
}

void
TDWRDecoder::processBuffer(const char * data, size_t len)
{
  if (!data || (len == 0)) {
    return;
  }

  const auto * udata = reinterpret_cast<const uint8_t *>(data);

  myMessageBuffer.insert(myMessageBuffer.end(), udata, udata + len);
  parseMessageBuffer();
}

static bool
isValidTDWRFrameHeader(const uint8_t * p, size_t avail)
{
  if (!p || (avail < 2)) {
    return false;
  }

  uint16_t msg_id = decodeBE16(p);

  if ((msg_id != TDWR_MSG_COMPRESSED_BASEDATA) && (msg_id != TDWR_MSG_LOWPRF_BASEDATA)) {
    return false;
  }

  if (avail >= 4) {
    uint16_t msg_len = decodeBE16(p + 2);
    if ((msg_len < TDWRConstants::HEADER_TOTAL_SIZE) || (msg_len > 6144)) {
      return false;
    }
  }

  if (avail >= TDWRConstants::HEADER_TOTAL_SIZE) {
    TDWRBaseDataHeader bdh;
    if (!TDWRDecoder::decodeBaseDataHeader(p + TDWRConstants::MESSAGE_HEADER_SIZE,
      avail - TDWRConstants::MESSAGE_HEADER_SIZE,
      bdh))
    {
      return false;
    }

    if (bdh.scan_strategy > 10) {
      return false;
    }
    if (std::isnan(bdh.current_elevation) || std::isnan(bdh.current_azimuth)) {
      return false;
    }
    if ((bdh.current_elevation < -5.0f) || (bdh.current_elevation > 90.0f)) {
      return false;
    }
    if ((bdh.current_azimuth < 0.0f) || (bdh.current_azimuth > 360.0f)) {
      return false;
    }
  }

  return true;
} // isValidTDWRFrameHeader

void
TDWRDecoder::parseMessageBuffer()
{
  while (true) {
    size_t avail = myMessageBuffer.size() - myMessageReadPos;
    if (avail < TDWRConstants::MESSAGE_HEADER_SIZE) {
      break;
    }

    const uint8_t * p = myMessageBuffer.data() + myMessageReadPos;

    if (!isValidTDWRFrameHeader(p, avail)) {
      // Stream desynchronization recovery: find next valid header
      size_t skip = 1;
      while (myMessageReadPos + skip + 1 < myMessageBuffer.size()) {
        const uint8_t * candidate = myMessageBuffer.data() + myMessageReadPos + skip;
        size_t candAvail = myMessageBuffer.size() - (myMessageReadPos + skip);
        if (isValidTDWRFrameHeader(candidate, candAvail)) {
          break;
        }
        skip++;
      }
      myMessageReadPos += skip;
      continue;
    }

    uint16_t msg_id  = decodeBE16(p);
    uint16_t msg_len = decodeBE16(p + 2);

    if (msg_id == TDWR_MSG_COMPRESSED_BASEDATA) {
      size_t expectedSize = TDWRConstants::COMPRESSED_RADIAL_SIZE; // 4852
      if (msg_len >= expectedSize) {
        expectedSize = msg_len;
      }
      if (avail < expectedSize) {
        break; // Wait for more data
      }

      TDWRCompressedRadial rad;
      if (decodeCompressedRadial(p, expectedSize, rad)) {
        if (myAccumulator) {
          myAccumulator->processRadial(rad);
        }
      }
      myMessageReadPos += expectedSize;
    } else if (msg_id == TDWR_MSG_LOWPRF_BASEDATA) {
      size_t expectedSize = TDWRConstants::LOWPRF_RADIAL_SIZE; // 6004
      if (msg_len >= expectedSize) {
        expectedSize = msg_len;
      }
      if (avail < expectedSize) {
        break; // Wait for more data
      }

      TDWRLowPRFRadial rad;
      if (decodeLowPRFRadial(p, expectedSize, rad)) {
        if (myAccumulator) {
          myAccumulator->processRadial(rad);
        }
      }
      myMessageReadPos += expectedSize;
    }
  }

  compactMessageBuffer();
} // TDWRDecoder::parseMessageBuffer

void
TDWRDecoder::flush()
{
  parseMessageBuffer();
  if (myAccumulator) {
    myAccumulator->flush();
  }
}

void
TDWRDecoder::resetStreamState()
{
  parseMessageBuffer();
  myMessageBuffer.clear();
  myMessageReadPos = 0;
}

void
TDWRDecoder::reset()
{
  myMessageBuffer.clear();
  myMessageReadPos = 0;
  if (myAccumulator) {
    myAccumulator->reset();
    if (!myRadarName.empty()) {
      myAccumulator->setRadarName(myRadarName);
    }
    myAccumulator->setUserVCP(myUserVCP);
    myAccumulator->setUseVolumeTime(myUseVolumeTime);
    myAccumulator->setNoiseThreshold(myNoiseThreshold);
    if (!myMagneticOffsetStr.empty()) {
      myAccumulator->setMagneticOffset(myMagneticOffsetStr);
    }
    myAccumulator->setFrequency(myFrequencyMHz);
    myAccumulator->setAlignmentResolution(myAlignmentResolution);
  }
}

TDWRAccumulator&
TDWRDecoder::getAccumulator()
{
  return *myAccumulator;
}

const TDWRAccumulator&
TDWRDecoder::getAccumulator() const
{
  return *myAccumulator;
}

const std::vector<std::shared_ptr<RadialSet> >&
TDWRDecoder::getRadialSets() const
{
  return myAccumulator->getRadialSets();
}

bool
TDWRDecoder::writeSweepToNetcdf(
  const std::vector<std::shared_ptr<DataType> >& sweepOutputs,
  const std::string                            & outputDir,
  bool                                         flatDir,
  bool                                         uncompressed,
  float                                        alignmentResolution)
{
  if (sweepOutputs.empty() || outputDir.empty()) {
    return false;
  }

  static bool initNotifiers = []() {
      RecordNotifier::introduceSelf();
      return true;
    }();

  (void) initNotifiers;

  IOConfig notifyCfg;

  notifyCfg.set("outputfolder", outputDir);

  auto dispatchNotification = [&](const Record& rec) {
      if (!PluginNotifier::theNotifiers.empty()) {
        for (auto& notifier : PluginNotifier::theNotifiers) {
          if (notifier) {
            notifier->writeRecord(notifyCfg, rec);
          }
        }
      } else {
        static FMLRecordNotifier defaultFML;
        defaultFML.writeRecord(notifyCfg, rec);
      }
    };

  auto notifyAs = [&](const Record& baseRec, const std::string& newType) {
      Record recCopy = baseRec;
      std::vector<std::string> sels = recCopy.getSelections();

      if (sels.size() > 1) {
        sels[1] = newType;
        recCopy.setSelections(sels);
      }
      dispatchNotification(recCopy);
    };

  // Emit synthetic NewVolume and NewElevation notifications
  auto firstRs = std::dynamic_pointer_cast<RadialSet>(sweepOutputs.front());

  if (firstRs) {
    Time sweepTime    = firstRs->getTime();
    std::string radar = firstRs->getRadarName();

    std::string volIdxStr;
    bool isNewVolume = false;
    if ((firstRs->getString("volElevIndex", volIdxStr) && (volIdxStr == "0")) ||
      (firstRs->getString("SOV", volIdxStr) && (volIdxStr == "1") ))
    {
      isNewVolume = true;
    }

    if (isNewVolume) {
      Record vrec;
      vrec.setTime(sweepTime);
      if (!radar.empty()) { vrec.setSourceName(radar); }
      std::vector<std::string> vparams = { "Event", "0" };
      vrec.setParams(vparams);
      std::vector<std::string> vsels = { sweepTime.getString(Message::RECORD_TIMESTAMP), "NewVolume", "" };
      vrec.setSelections(vsels);
      vrec.setValue("MessageText", "NewVolume");
      vrec.setValue("Event", "NewVolume");
      dispatchNotification(vrec);
    }

    Record erec;
    erec.setTime(sweepTime);
    if (!radar.empty()) { erec.setSourceName(radar); }
    std::vector<std::string> eparams = { "Event", "0" };
    erec.setParams(eparams);
    std::vector<std::string> esels = { sweepTime.getString(Message::RECORD_TIMESTAMP), "NewElevation", "" };
    erec.setSelections(esels);
    erec.setValue("MessageText", "NewElevation");
    erec.setValue("Event", "NewElevation");
    dispatchNotification(erec);
  }

  bool allSuccess = true;

  for (const auto& rawDt : sweepOutputs) {
    if (!rawDt) { continue; }

    std::shared_ptr<DataType> dt = rawDt;
    auto rs = std::dynamic_pointer_cast<RadialSet>(dt);
    if (rs && (alignmentResolution > 0.0f)) {
      size_t targetRadials = static_cast<size_t>(std::round(360.0f / alignmentResolution));
      const auto& azs      = rs->getAzimuthRef();
      bool unaligned       = (rs->getNumRadials() != targetRadials) || azs.empty() || (std::abs(azs[0]) > 1e-4f);
      if ((targetRadials > 0) && unaligned) {
        rs = rs->Remap(rs->getGateWidthKMs() * 1000.0f, targetRadials, rs->getNumGates(), false);
        dt = rs;
      }
    }

    std::string typeName = dt->getTypeName();
    std::string elevStr  = rs ? DataType::formatString(rs->getElevationDegs(), 5, 2) : "";
    std::string dateStr;
    if (!dt->getString(Constants::FilenameDateTime, dateStr) || dateStr.empty()) {
      dateStr = dt->getTime().getString("%Y%m%d-%H%M%S");
    }

    fs::path targetPath;
    if (flatDir) {
      std::string fname;
      if (elevStr.empty()) {
        fname = typeName + "_" + dateStr + ".netcdf";
      } else {
        fname = typeName + "_" + elevStr + "_" + dateStr + ".netcdf";
      }
      targetPath = fs::path(outputDir) / fname;
    } else {
      std::string fname = dateStr + ".netcdf";
      if (elevStr.empty()) {
        targetPath = fs::path(outputDir) / typeName / fname;
      } else {
        targetPath = fs::path(outputDir) / typeName / elevStr / fname;
      }
    }

    try {
      fs::create_directories(targetPath.parent_path());
    } catch (const std::exception& e) {
      fLogSevere("TDWRDecoder: Failed to create directories for {}: {}", targetPath.string(), e.what());
      allSuccess = false;
      continue;
    }

    IOConfig cfg;
    cfg.set("filepathmode", "direct");
    cfg.set("outputfolder", targetPath.string());
    if (uncompressed) {
      cfg.set("compress", "false");
      cfg.set("compression", "");
    } else {
      cfg.set("compress", "true");
      cfg.set("compression", "gz");
    }

    std::vector<Record> records;
    std::string factory = "netcdf";
    bool ok = IODataType::write(dt, targetPath.string(), records, factory, cfg);
    if (!ok) {
      fLogSevere("TDWRDecoder: Failed writing {} to {}", typeName, targetPath.string());
      allSuccess = false;
    } else {
      fLogInfo("TDWRDecoder: Successfully wrote sweep NetCDF {}", targetPath.string());
      std::string indexLocation = IOIndex::getIndexPath(outputDir);
      for (auto rec : records) {
        if ((rec.getParams().size() == 2) && (rec.getParams()[1].rfind(indexLocation, 0) == 0)) {
          const std::string& fullPath = rec.getParams()[1];
          std::string relPath;
          if (fullPath.rfind(outputDir, 0) == 0) {
            relPath = fullPath.substr(outputDir.size());
          } else {
            relPath = fs::relative(fullPath, outputDir).string();
          }
          while (!relPath.empty() && (relPath.front() == '/' || relPath.front() == '\\')) {
            relPath.erase(0, 1);
          }
          std::string fact = rec.getParams()[0].empty() ? factory : rec.getParams()[0];
          std::vector<std::string> newParams = { fact, indexLocation, relPath };
          rec.setParams(newParams);
        }
        dispatchNotification(rec);

        if ((typeName == "Velocity") || (typeName == "ConditionedVelocity")) {
          notifyAs(rec, "SRVel");
          notifyAs(rec, "StormRelVel");
        }
        if (rs && (rs->getElevationDegs() > 0.49f) && (rs->getElevationDegs() < 0.51f)) {
          notifyAs(rec, typeName + "_at0.5");
          if ((typeName == "Velocity") || (typeName == "ConditionedVelocity")) {
            notifyAs(rec, "SRVel_at0.5");
            notifyAs(rec, "StormRelVel_at0.5");
          }
        }
      }
    }
  }

  return allSuccess;
} // TDWRDecoder::writeSweepToNetcdf

bool
TDWRDecoder::decodeMessageHeader(const uint8_t * ptr, size_t len, TDWRMessageHeader& out)
{
  if (!ptr || (len < TDWRConstants::MESSAGE_HEADER_SIZE)) {
    return false;
  }
  out.msg_id  = decodeBE16(ptr);
  out.msg_len = decodeBE16(ptr + 2);
  return true;
}

bool
TDWRDecoder::decodeBaseDataHeader(const uint8_t * ptr, size_t len, TDWRBaseDataHeader& out)
{
  if (!ptr || (len < TDWRConstants::BASE_DATA_HEADER_SIZE)) {
    return false;
  }

  // Word 1 (bytes 0-3): volume_scan_count, flags, scan_strategy
  out.volume_scan_count = decodeBE16(ptr);

  uint8_t b2 = ptr[2];

  out.EOV      = ((b2 >> 7) & 1) != 0;
  out.SOV      = ((b2 >> 6) & 1) != 0;
  out.nullScan = b2 & 0x3F;

  out.scan_strategy = ptr[3];

  // Word 2 (bytes 4-7): peak_transmitter_power, site_id, MD, IDLE, SLD, SPD, null_flags
  out.peak_transmitter_power = decodeBE16(ptr + 4);

  uint16_t hw2 = decodeBE16(ptr + 6);

  out.site_id    = (hw2 >> 10) & 0x3F;
  out.MD         = (hw2 >> 9) & 1;
  out.null_flags = (hw2 >> 3) & 0x3F;
  out.IDLE       = ((hw2 >> 2) & 1) != 0;
  out.SLD        = ((hw2 >> 1) & 1) != 0;
  out.SPD        = (hw2 & 1) != 0;

  // Word 3 (bytes 8-11): scan_number, EOE, SOE, CRMN, scan_id, OF, SR, DA, SS, MA, PR, WS, LE, MB, GF, LP
  out.scan_number = ptr[8];

  uint8_t b9 = ptr[9];

  out.EOE  = ((b9 >> 7) & 1) != 0;
  out.SOE  = ((b9 >> 6) & 1) != 0;
  out.CRMN = (b9 >> 3) & 7;

  uint8_t b10 = ptr[10];

  out.scan_id = ((b9 & 0x07) << 3) | ((b10 >> 5) & 0x07);
  out.OF      = ((b10 >> 4) & 1) != 0;
  out.SR      = ((b10 >> 3) & 1) != 0;
  out.DA      = (b10 >> 1) & 3;
  out.SS      = (b10 & 1) != 0;

  uint8_t b11 = ptr[11];

  out.MA = ((b11 >> 7) & 1) != 0;
  out.PR = (b11 >> 5) & 3;
  out.WS = ((b11 >> 4) & 1) != 0;
  out.LE = ((b11 >> 3) & 1) != 0;
  out.MB = ((b11 >> 2) & 1) != 0;
  out.GF = ((b11 >> 1) & 1) != 0;
  out.LP = (b11 & 1) != 0;

  // Word 4 (bytes 12-15): current_elevation
  out.current_elevation = decodeBEFloat(ptr + 12);

  // Word 5 (bytes 16-19): angular_scan_rate
  out.angular_scan_rate = decodeBEFloat(ptr + 16);

  // Word 6 (bytes 20-23): NP, MP, PRI, dwell_id, SI, TST, pulses_per_dwell
  uint16_t hw6_1 = decodeBE16(ptr + 20);

  out.NP  = ((hw6_1 >> 15) & 1) != 0;
  out.MP  = ((hw6_1 >> 14) & 1) != 0;
  out.PRI = hw6_1 & 0x3FFF;

  uint16_t hw6_2 = decodeBE16(ptr + 22);

  out.dwell_id         = (hw6_2 >> 14) & 3;
  out.SI               = ((hw6_2 >> 13) & 1) != 0;
  out.TST              = ((hw6_2 >> 12) & 1) != 0;
  out.pulses_per_dwell = hw6_2 & 0x0FFF;

  // Word 7 (bytes 24-27): final_range_sample, range_samples_per_dwell
  out.final_range_sample      = decodeBE16(ptr + 24);
  out.range_samples_per_dwell = decodeBE16(ptr + 26);

  // Word 8 (bytes 28-31): current_azimuth
  out.current_azimuth = decodeBEFloat(ptr + 28);

  // Word 9 (bytes 32-35): total_noise_power
  out.total_noise_power = decodeBEFloat(ptr + 32);

  // Word 10 (bytes 36-39): UTC timestamp
  out.timestamp   = decodeBE32(ptr + 36);
  out.radial_time = Time(static_cast<time_t>(out.timestamp), 0.0);

  // Word 11 (bytes 40-43): base_data_type, CH, VN, WI, IVS, IES, VSR, ESR
  out.base_data_type = decodeBE16(ptr + 40);

  uint16_t hw11 = decodeBE16(ptr + 42);

  out.CH  = static_cast<uint8_t>((hw11 >> 9) & 1);
  out.VN  = (hw11 >> 5) & 0x0F;
  out.WI  = ((hw11 >> 4) & 1) != 0;
  out.IVS = ((hw11 >> 3) & 1) != 0;
  out.IES = ((hw11 >> 2) & 1) != 0;
  out.VSR = ((hw11 >> 1) & 1) != 0;
  out.ESR = (hw11 & 1) != 0;

  // Word 12 (bytes 44-47): integer_azimuth, load_shed_final_sample
  out.integer_azimuth        = decodeBE16(ptr + 44);
  out.load_shed_final_sample = decodeBE16(ptr + 46);

  return true;
} // TDWRDecoder::decodeBaseDataHeader

bool
TDWRDecoder::decodeCompressedRadial(const uint8_t * ptr, size_t len, TDWRCompressedRadial& out)
{
  if (!ptr || (len < TDWRConstants::HEADER_TOTAL_SIZE)) {
    return false;
  }

  if (!decodeMessageHeader(ptr, len, out.msg_hdr)) {
    return false;
  }

  if (!out.msg_hdr.isCompressedBaseData()) {
    return false;
  }

  if (!decodeBaseDataHeader(ptr + TDWRConstants::MESSAGE_HEADER_SIZE,
    len - TDWRConstants::MESSAGE_HEADER_SIZE,
    out.hdr))
  {
    return false;
  }

  // Determine gate count: respect final_range_sample bounded by MAX_COMPRESSED_GATES (600)
  size_t num_gates = TDWRConstants::MAX_COMPRESSED_GATES;

  if ((out.hdr.final_range_sample > 0) && (out.hdr.final_range_sample < num_gates)) {
    num_gates = out.hdr.final_range_sample;
  }

  // Check available payload bytes
  size_t payload_bytes = len - TDWRConstants::HEADER_TOTAL_SIZE;
  size_t max_avail     = payload_bytes / TDWRConstants::COMPRESSED_GATE_SIZE;

  if (num_gates > max_avail) {
    num_gates = max_avail;
  }

  out.gates.clear();
  out.gates.reserve(num_gates);

  const uint8_t * gp = ptr + TDWRConstants::HEADER_TOTAL_SIZE;

  for (size_t i = 0; i < num_gates; ++i) {
    TDWRCompressedGate gate;
    gate.Z   = gp[0];
    gate.SNR = gp[1];
    gate.VEL = decodeBES16(gp + 2);
    gate.W   = gp[4];

    uint8_t flags = gp[5];
    gate.GCF = (flags >> 6) & 3;
    gate.CAF = (flags >> 3) & 7;
    gate.CTF = ((flags >> 2) & 1) != 0;
    gate.CVF = ((flags >> 1) & 1) != 0;
    gate.CCV = (flags & 1) != 0;

    gate.dealVEL = decodeBES16(gp + 6);

    out.gates.push_back(gate);
    gp += TDWRConstants::COMPRESSED_GATE_SIZE;
  }

  return true;
} // TDWRDecoder::decodeCompressedRadial

bool
TDWRDecoder::decodeLowPRFRadial(const uint8_t * ptr, size_t len, TDWRLowPRFRadial& out)
{
  if (!ptr || (len < TDWRConstants::HEADER_TOTAL_SIZE)) {
    return false;
  }

  if (!decodeMessageHeader(ptr, len, out.msg_hdr)) {
    return false;
  }

  if (!out.msg_hdr.isLowPRFBaseData()) {
    return false;
  }

  if (!decodeBaseDataHeader(ptr + TDWRConstants::MESSAGE_HEADER_SIZE,
    len - TDWRConstants::MESSAGE_HEADER_SIZE,
    out.hdr))
  {
    return false;
  }

  // Low PRF has up to 1984 gates (900 at 150m + 1084 at 300m)
  size_t num_gates = TDWRConstants::MAX_LOWPRF_GATES;

  if ((out.hdr.final_range_sample > 0) && (out.hdr.final_range_sample < num_gates)) {
    num_gates = out.hdr.final_range_sample;
  }

  size_t payload_bytes     = len - TDWRConstants::HEADER_TOTAL_SIZE;
  const uint8_t * z_ptr    = ptr + TDWRConstants::HEADER_TOTAL_SIZE;
  const uint8_t * snr_ptr  = nullptr;
  const uint8_t * flag_ptr = nullptr;

  if (payload_bytes >= TDWRConstants::MAX_LOWPRF_GATES * 3) {
    snr_ptr  = z_ptr + TDWRConstants::MAX_LOWPRF_GATES;
    flag_ptr = snr_ptr + TDWRConstants::MAX_LOWPRF_GATES;
  } else {
    if (payload_bytes < num_gates * 3) {
      num_gates = payload_bytes / 3;
    }
    snr_ptr  = z_ptr + num_gates;
    flag_ptr = snr_ptr + num_gates;
  }

  out.Z.resize(num_gates);
  out.SNR.resize(num_gates);
  out.flags.resize(num_gates);

  std::memcpy(out.Z.data(), z_ptr, num_gates);
  std::memcpy(out.SNR.data(), snr_ptr, num_gates);

  for (size_t i = 0; i < num_gates; ++i) {
    uint8_t c = flag_ptr[i];
    TDWRLowPRFFlag f;
    f.GCF        = ((c >> 7) & 1) != 0;
    f.CV         = ((c >> 6) & 1) != 0;
    f.bd_nul2    = (c >> 3) & 7;
    f.CTF        = ((c >> 2) & 1) != 0;
    f.bd_nul3    = c & 3;
    out.flags[i] = f;
  }

  return true;
} // TDWRDecoder::decodeLowPRFRadial
} // namespace rapio
