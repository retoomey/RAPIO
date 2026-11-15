#include "rBOOSTTest.h"

#include "rTDWRDecoder.h"
#include "rTDWRAccumulator.h"
#include "rTDWRStreamReader.h"
#include <rRadialSet.h>
#include <rConstants.h>
#include <rConfigRadarInfo.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <cstring>
#include <cmath>
#include <endian.h>

using namespace rapio;

namespace {
void
appendBE16(std::vector<uint8_t>& buf, uint16_t val)
{
  uint16_t be    = htobe16(val);
  const auto * p = reinterpret_cast<const uint8_t *>(&be);

  buf.insert(buf.end(), p, p + 2);
}

void
appendBE32(std::vector<uint8_t>& buf, uint32_t val)
{
  uint32_t be    = htobe32(val);
  const auto * p = reinterpret_cast<const uint8_t *>(&be);

  buf.insert(buf.end(), p, p + 4);
}

void
appendBEFloat(std::vector<uint8_t>& buf, float val)
{
  uint32_t u;

  std::memcpy(&u, &val, sizeof(u));
  appendBE32(buf, u);
}

void
appendBaseDataHeader(
  std::vector<uint8_t>& buf,
  uint16_t vol_scan_count, bool EOV, bool SOV, uint8_t strategy,
  uint16_t peak_power, uint8_t scan_no, bool EOE, bool SOE,
  float elevation, float az_rate, uint16_t PRI, uint16_t dwell_id, uint16_t pulses,
  uint16_t num_gates, float azimuth, float noise_power, uint32_t ts)
{
  appendBE16(buf, vol_scan_count);
  uint8_t b2 = (EOV ? (1 << 7) : 0) | (SOV ? (1 << 6) : 0);

  buf.push_back(b2);
  buf.push_back(strategy);
  appendBE16(buf, peak_power);
  appendBE16(buf, 0);
  buf.push_back(scan_no);
  uint8_t b9 = (EOE ? (1 << 7) : 0) | (SOE ? (1 << 6) : 0);

  buf.push_back(b9);
  buf.push_back(0);
  buf.push_back(0);
  appendBEFloat(buf, elevation);
  appendBEFloat(buf, az_rate);
  appendBE16(buf, PRI & 0x3FFF);
  appendBE16(buf, (dwell_id << 14) | (pulses & 0x0FFF));
  appendBE16(buf, num_gates);
  appendBE16(buf, num_gates);
  appendBEFloat(buf, azimuth);
  appendBEFloat(buf, noise_power);
  appendBE32(buf, ts);
  appendBE16(buf, 0x07);
  appendBE16(buf, 0);
  appendBE16(buf, static_cast<uint16_t>(azimuth));
  appendBE16(buf, num_gates);
}

std::vector<uint8_t>
createSyntheticCompressedRadial(
  float azimuth, float elevation, bool SOE, bool EOE, bool SOV, bool EOV,
  uint8_t z_val, uint8_t snr_val, int16_t vel_val, uint8_t w_val, int16_t deal_vel_val,
  uint8_t caf, bool cvf, bool ccv, bool ctf,
  uint16_t num_gates = 600, uint16_t PRI = 800)
{
  std::vector<uint8_t> buf;

  buf.reserve(TDWRConstants::COMPRESSED_RADIAL_SIZE);
  appendBE16(buf, TDWR_MSG_COMPRESSED_BASEDATA);
  appendBE16(buf, static_cast<uint16_t>(TDWRConstants::COMPRESSED_RADIAL_SIZE));
  appendBaseDataHeader(buf, 1, EOV, SOV, 2, 50000, 1, EOE, SOE,
    elevation, 20.0f, PRI, 1, 32, num_gates, azimuth, -100.0f, 1700000000);

  uint8_t flagByte = ((caf & 7) << 3) | (ctf ? 4 : 0) | (cvf ? 2 : 0) | (ccv ? 1 : 0);

  for (size_t g = 0; g < 600; ++g) {
    buf.push_back(z_val);
    buf.push_back(snr_val);
    appendBE16(buf, static_cast<uint16_t>(vel_val));
    buf.push_back(w_val);
    buf.push_back(flagByte);
    appendBE16(buf, static_cast<uint16_t>(deal_vel_val));
  }
  return buf;
}

std::vector<uint8_t>
createSyntheticLowPRFRadial(
  float azimuth, float elevation, bool SOE, bool EOE, bool SOV, bool EOV,
  uint8_t z_val, uint8_t snr_val, bool cv, bool ctf,
  uint16_t num_gates = 1984, uint16_t PRI = 3000)
{
  std::vector<uint8_t> buf;

  buf.reserve(TDWRConstants::LOWPRF_RADIAL_SIZE);
  appendBE16(buf, TDWR_MSG_LOWPRF_BASEDATA);
  appendBE16(buf, static_cast<uint16_t>(TDWRConstants::LOWPRF_RADIAL_SIZE));
  appendBaseDataHeader(buf, 1, EOV, SOV, 1, 50000, 1, EOE, SOE,
    elevation, 15.0f, PRI, 1, 32, num_gates, azimuth, -105.0f, 1700000000);

  buf.insert(buf.end(), 1984, z_val);
  buf.insert(buf.end(), 1984, snr_val);
  uint8_t f = (cv ? (1 << 6) : 0) | (ctf ? (1 << 2) : 0);

  buf.insert(buf.end(), 1984, f);
  return buf;
}
} // namespace

BOOST_AUTO_TEST_SUITE(rTestTDWRSuite)

// 1. Magnetic declination parsing, Nyquist formula, and bitfield header unpacking
BOOST_AUTO_TEST_CASE(TestTDWRHeaderMathAndBitfields)
{
  BOOST_CHECK_CLOSE(TDWRAccumulator::parseMagneticOffsetString("10W"), -10.0f, 0.001f);
  BOOST_CHECK_CLOSE(TDWRAccumulator::parseMagneticOffsetString("7E"), 7.0f, 0.001f);
  BOOST_CHECK_CLOSE(TDWRAccumulator::parseMagneticOffsetString("-8.2"), -8.2f, 0.001f);

  TDWRBaseDataHeader hdr;

  hdr.PRI = 800;
  BOOST_CHECK_CLOSE(hdr.getNyquistVelocity(5603), 16.7321f, 0.05f);
  hdr.PRI = 0;
  BOOST_CHECK_EQUAL(hdr.getNyquistVelocity(5603), 0.0f);

  // Verify split scan_id bit unpacking across bytes 9 and 10
  std::vector<uint8_t> buf(TDWRConstants::HEADER_TOTAL_SIZE, 0);

  buf[0]      = 0x2b;
  buf[1]      = 0x00;
  buf[2]      = 0x12;
  buf[3]      = 0xf4;
  buf[4 + 9]  = 0x05;
  buf[4 + 10] = 0x60;
  BOOST_REQUIRE(TDWRDecoder::decodeBaseDataHeader(buf.data() + 4, buf.size() - 4, hdr));
  BOOST_CHECK_EQUAL(hdr.scan_id, 43);
}

// 2. Compressed Base Data (0x2b00) decoding, moment scaling, and magnetic rotation
BOOST_AUTO_TEST_CASE(TestTDWRCompressedDecodingAndScaling)
{
  TDWRAccumulator acc;

  acc.setRadarName("TOKC");
  acc.setMagneticOffset("7E");
  acc.setFrequency(5603);

  std::vector<std::shared_ptr<DataType> > completedSweep;

  acc.setSweepCallback([&](const std::vector<std::shared_ptr<DataType> >& sweep) {
    completedSweep = sweep;
  });

  auto b0 = createSyntheticCompressedRadial(0.0f, 0.5f, true, false, true, false, 100, 40, 400, 16, 360, 0, true, true,
      false);
  auto b1 = createSyntheticCompressedRadial(0.5f, 0.5f, false, true, false, false, 100, 40, 400, 16, 360, 0, true, true,
      false);

  TDWRCompressedRadial r0, r1;

  BOOST_REQUIRE(TDWRDecoder::decodeCompressedRadial(b0.data(), b0.size(), r0));
  BOOST_REQUIRE(TDWRDecoder::decodeCompressedRadial(b1.data(), b1.size(), r1));

  acc.processRadial(r0);
  acc.processRadial(r1);
  BOOST_REQUIRE_EQUAL(completedSweep.size(), 5);

  std::map<std::string, std::shared_ptr<RadialSet> > moments;

  for (const auto& item : completedSweep) {
    auto rs = std::dynamic_pointer_cast<RadialSet>(item);
    moments[rs->getTypeName()] = rs;
  }

  auto refSet = moments["Reflectivity"];

  BOOST_REQUIRE(refSet != nullptr);
  BOOST_CHECK_CLOSE((*refSet->getFloat1DPtr(RadialSet::Azimuth))[0], 7.0f, 0.001f);
  BOOST_CHECK_CLOSE((*refSet->getFloat1DPtr(RadialSet::BeamWidth))[0], 0.5f, 0.001f); // First radial matches second

  BOOST_CHECK_CLOSE((*refSet->getFloat2DPtr())[0][0], 20.0f, 0.001f);
  BOOST_CHECK_CLOSE((*moments["ConditionedVelocity"]->getFloat2DPtr())[0][0], 10.0f, 0.001f);
  BOOST_CHECK_CLOSE((*moments["AliasedVelocity"]->getFloat2DPtr())[0][0], 20.0f, 0.001f);
  BOOST_CHECK_CLOSE((*moments["SpectrumWidth"]->getFloat2DPtr())[0][0], 4.0f, 0.001f);
  BOOST_CHECK_CLOSE((*moments["SignalToNoise"]->getFloat2DPtr())[0][0], 20.0f, 0.001f);
}

// 3. Low PRF Base Data (0x2b01) fixed 1984-byte stride unpacking and 3068-gate expansion
BOOST_AUTO_TEST_CASE(TestTDWRLowPRFDecodingAndGateExpansion)
{
  // Verify fixed 1984-byte stride even when final_range_sample < 1984
  auto partialBytes = createSyntheticLowPRFRadial(180.0f, 0.6f, true, true, false, false, 100, 45, true, false, 500);
  TDWRLowPRFRadial partialRad;

  BOOST_REQUIRE(TDWRDecoder::decodeLowPRFRadial(partialBytes.data(), partialBytes.size(), partialRad));
  BOOST_CHECK_EQUAL(partialRad.Z.size(), 500);
  BOOST_CHECK_EQUAL(partialRad.SNR[499], 45);
  BOOST_CHECK_EQUAL(partialRad.flags[499].CV, true);

  // Verify 3068-gate expansion (900 inner + 1084 * 2 outer)
  TDWRAccumulator acc;

  acc.setRadarName("TDFW");
  acc.setMagneticOffset("10W");
  acc.setNoiseThreshold(1.0f);

  std::vector<std::shared_ptr<DataType> > completedSweep;

  acc.setSweepCallback([&](const std::vector<std::shared_ptr<DataType> >& sweep) {
    completedSweep = sweep;
  });

  auto fullBytes = createSyntheticLowPRFRadial(5.0f, 0.6f, true, true, true, false, 110, 40, true, false);
  TDWRLowPRFRadial r0;

  TDWRDecoder::decodeLowPRFRadial(fullBytes.data(), fullBytes.size(), r0);
  r0.Z[900] = 120; // First outer 300m gate -> 30.0 dBZ
  r0.Z[901] = 140; // Second outer 300m gate -> 40.0 dBZ
  acc.processRadial(r0);

  BOOST_REQUIRE_EQUAL(completedSweep.size(), 2);
  auto refSet = std::dynamic_pointer_cast<RadialSet>(completedSweep[0]);

  BOOST_REQUIRE(refSet != nullptr);
  BOOST_CHECK_EQUAL(refSet->getNumGates(), 3068);
  BOOST_CHECK_CLOSE((*refSet->getFloat1DPtr(RadialSet::Azimuth))[0], 355.0f, 0.001f);

  auto refData = refSet->getFloat2DPtr();

  BOOST_CHECK_CLOSE((*refData)[0][899], 25.0f, 0.001f);
  BOOST_CHECK_CLOSE((*refData)[0][900], 30.0f, 0.001f);
  BOOST_CHECK_CLOSE((*refData)[0][901], 30.0f, 0.001f);
  BOOST_CHECK_CLOSE((*refData)[0][902], 40.0f, 0.001f);
  BOOST_CHECK_CLOSE((*refData)[0][903], 40.0f, 0.001f);
}

// 4. Sweep & Volume Boundary Transitions (SOE without EOE, Scan/Strategy Change, SOV tracking)
BOOST_AUTO_TEST_CASE(TestTDWRSweepAndVolumeTransitions)
{
  TDWRAccumulator acc;

  acc.setRadarName("TOKC");
  acc.setKeepCompletedRadialSets(true);
  size_t sweepCount = 0;

  acc.setSweepCallback([&](const std::vector<std::shared_ptr<DataType> >&) {
    sweepCount++;
  });

  auto b0 = createSyntheticCompressedRadial(0.0f, 0.5f, true, false, false, false, 100, 40, 400, 16, 360, 0, true, true,
      false);
  TDWRCompressedRadial r0;

  TDWRDecoder::decodeCompressedRadial(b0.data(), b0.size(), r0);
  r0.hdr.scan_number   = 1;
  r0.hdr.scan_strategy = 1;

  // 1. Start sweep 1 (no EOE)
  acc.processRadial(r0);
  BOOST_CHECK_EQUAL(sweepCount, 0);

  // 2. Scan number change without EOE flushes sweep 1
  TDWRCompressedRadial r1 = r0;

  r1.hdr.scan_number = 2;
  r1.hdr.SOE         = false;
  acc.processRadial(r1);
  BOOST_CHECK_EQUAL(sweepCount, 1);

  // 3. Scan strategy change flushes sweep 2
  TDWRCompressedRadial r2 = r1;

  r2.hdr.scan_strategy = 2;
  acc.processRadial(r2);
  BOOST_CHECK_EQUAL(sweepCount, 2);

  // 4. Transition from Compressed (0x2b00) to LowPRF (0x2b01) flushes sweep 3 and sets SOV=1 on LowPRF
  auto lpBytes = createSyntheticLowPRFRadial(0.0f, 0.6f, true, true, false, false, 100, 40, true, false);
  TDWRLowPRFRadial lr0;

  TDWRDecoder::decodeLowPRFRadial(lpBytes.data(), lpBytes.size(), lr0);
  acc.processRadial(lr0);
  BOOST_CHECK_EQUAL(sweepCount, 4);

  std::string sovStr;

  BOOST_CHECK(acc.getRadialSets().back()->getString("SOV", sovStr));
  BOOST_CHECK_EQUAL(sovStr, "1");
}

// 5. Stream Framing, tdwrLogIngest Padded Frames, and False-Positive Resynchronization
BOOST_AUTO_TEST_CASE(TestTDWRStreamingAndResync)
{
  // A. Test TDWRStreamReader with tdwrLogIngest padded frames (internal msg_len < frame size)
  TDWRStreamOptions opts;

  opts.radarName = "TOKC";
  TDWRStreamReader reader(opts);
  size_t sweepsDelivered = 0;

  reader.setSweepCallback([&](const std::vector<std::shared_ptr<DataType> >& sweep) {
    if (!sweep.empty()) { sweepsDelivered++; }
  });

  auto rad1 = createSyntheticCompressedRadial(10.0f, 0.5f, true, true, true, false, 100, 40, 400, 16, 360, 0, true,
      true, false);
  uint16_t len1 = htobe16(2452); // Internal header msg_len smaller than 4852-byte padded frame

  std::memcpy(rad1.data() + 2, &len1, 2);

  auto rad2     = createSyntheticLowPRFRadial(11.0f, 0.5f, true, true, false, true, 100, 40, true, false);
  uint16_t len2 = htobe16(5000); // Internal header msg_len smaller than 6004-byte padded frame

  std::memcpy(rad2.data() + 2, &len2, 2);

  std::vector<uint8_t> streamData;

  streamData.insert(streamData.end(), rad1.begin(), rad1.end());
  streamData.insert(streamData.end(), rad2.begin(), rad2.end());

  for (size_t off = 0; off < streamData.size(); off += 512) {
    reader.pushBytes(reinterpret_cast<const char *>(streamData.data() + off),
      std::min<size_t>(512, streamData.size() - off));
  }
  reader.flush();
  BOOST_CHECK_EQUAL(sweepsDelivered, 2);

  // B. Test TDWRDecoder false-positive resynchronization past corrupted bytes (0x2b 0x00 0xFF 0xFF)
  TDWRDecoder decoder;

  decoder.setRadarName("TOKC");
  size_t resyncSweeps = 0;

  decoder.setSweepCallback([&](const std::vector<std::shared_ptr<DataType> >& sweep) {
    if (!sweep.empty()) { resyncSweeps++; }
  });

  std::vector<uint8_t> noisyBuf = { 0xAA, 0xBB, 0x2B, 0x00, 0xFF, 0xFF, 0x01, 0x02 };
  auto validRad = createSyntheticCompressedRadial(0.0f, 0.5f, true, true, true, false, 100, 40, 400, 16, 360, 0, true,
      true, false);

  noisyBuf.insert(noisyBuf.end(), validRad.begin(), validRad.end());

  decoder.processBuffer(reinterpret_cast<const char *>(noisyBuf.data()), noisyBuf.size());
  decoder.flush();
  BOOST_CHECK_EQUAL(resyncSweeps, 1);
}

// 6. Azimuthal Alignment (-A) and Variable/Uniform Nyquist Toggle
BOOST_AUTO_TEST_CASE(TestTDWRAlignmentAndNyquistToggle)
{
  TDWRAccumulator acc;

  acc.setRadarName("TOKC");
  acc.setFrequency(5603);
  acc.setAlignmentResolution(1.0f);

  // Two radials with different PRI (800 vs 400) -> triggers 1D NyquistVelocity array
  auto b0 = createSyntheticCompressedRadial(0.0f, 0.5f, true, false, true, false, 100, 40, 400, 16, 360, 0, true, true,
      false, 600, 800);
  auto b1 = createSyntheticCompressedRadial(1.0f, 0.5f, false, true, false, false, 100, 40, 400, 16, 360, 0, true, true,
      false, 600, 400);
  TDWRCompressedRadial r0, r1;

  TDWRDecoder::decodeCompressedRadial(b0.data(), b0.size(), r0);
  TDWRDecoder::decodeCompressedRadial(b1.data(), b1.size(), r1);

  acc.processRadial(r0);
  acc.processRadial(r1);

  const auto& sets = acc.getRadialSets();

  BOOST_REQUIRE(!sets.empty());
  auto rs = sets.front();

  BOOST_CHECK_EQUAL(rs->getNumRadials(), 360);
  BOOST_CHECK(rs->haveNyquistArray());
}

// 7. Volume Time (-V), Multi-Directory Output (-o), and FML Notification Aliases (_at0.5, SRVel)
BOOST_AUTO_TEST_CASE(TestTDWRNetcdfMultiDirAndFMLNotifications)
{
  namespace fs = std::filesystem;
  std::string dir1 = (fs::temp_directory_path() / "test_tdwr_odir1").string();
  std::string dir2 = (fs::temp_directory_path() / "test_tdwr_odir2").string();

  fs::remove_all(dir1);
  fs::remove_all(dir2);

  TDWRStreamOptions opts;

  opts.radarName     = "TOKC";
  opts.outputDir     = dir1 + " " + dir2; // Dual output: dir1 compressed, dir2 uncompressed
  opts.useVolumeTime = true;

  TDWRStreamReader reader(opts);
  auto rad1 = createSyntheticCompressedRadial(100.0f, 0.5f, true, true, true, false, 100, 40, 400, 16, 360, 0, true,
      true, false);

  reader.pushBytes(reinterpret_cast<const char *>(rad1.data()), rad1.size());
  reader.flush();

  bool hasGzInDir1 = false, hasUncompressedInDir2 = false, foundAt05FML = false;

  for (const auto& entry : fs::recursive_directory_iterator(dir1)) {
    if (entry.path().string().find(".netcdf.gz") != std::string::npos) { hasGzInDir1 = true; }
    if (entry.path().extension() == ".fml") {
      std::ifstream ifs(entry.path().string());
      std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
      if (content.find("_at0.5") != std::string::npos) { foundAt05FML = true; }
    }
  }
  for (const auto& entry : fs::recursive_directory_iterator(dir2)) {
    if (entry.path().extension() == ".netcdf") { hasUncompressedInDir2 = true; }
  }

  BOOST_CHECK(hasGzInDir1);
  BOOST_CHECK(hasUncompressedInDir2);
  BOOST_CHECK(foundAt05FML);

  fs::remove_all(dir1);
  fs::remove_all(dir2);
}

BOOST_AUTO_TEST_SUITE_END()
