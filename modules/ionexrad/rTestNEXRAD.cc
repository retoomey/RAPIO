// Add this at top for any BOOST test
#include "rBOOSTTest.h"

#include "rNEXRADDecoder.h"
#include "rNEXRADAccumulator.h"
#include "rNEXRADFileReader.h"
#include "rNEXRADStreamReader.h"
#include "rIONEXRAD.h"
#include <rDataFilter.h>
#include <rRadialSet.h>
#include <rDataTable.h>
#include <rMultiDataType.h>
#include <rIODataType.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <filesystem>
#include <endian.h>

#ifndef NEXRAD_TEST_FILE
# define NEXRAD_TEST_FILE "KATX20261110_065712_V06.ar2v"
#endif

using namespace rapio;

namespace {
void
writeBE16(std::vector<char>& buf, uint16_t val)
{
  uint16_t be    = htobe16(val);
  const char * p = reinterpret_cast<const char *>(&be);

  buf.insert(buf.end(), p, p + 2);
}

void
writeBE32(std::vector<char>& buf, uint32_t val)
{
  uint32_t be    = htobe32(val);
  const char * p = reinterpret_cast<const char *>(&be);

  buf.insert(buf.end(), p, p + 4);
}

void
writeBEFloat(std::vector<char>& buf, float val)
{
  uint32_t u;

  std::memcpy(&u, &val, sizeof(u));
  writeBE32(buf, u);
}

std::vector<char>
createSyntheticM31RadialBytes(
  float az, float elev, uint8_t radial_status, uint16_t num_blocks = 4,
  int r_index = 1, uint32_t radial_time_msecs = 43200000)
{
  std::vector<char> buf;

  for (int i = 0; i < 6; ++i) {
    writeBE16(buf, 0); // 12-byte CTM
  }
  size_t msg_hdr_pos = buf.size();

  writeBE16(buf, 0); // msg_size placeholder
  buf.push_back(0);  // channel_id
  buf.push_back(31); // message_type = 31
  writeBE16(buf, static_cast<uint16_t>(r_index));
  writeBE16(buf, 18765);
  writeBE32(buf, radial_time_msecs);
  writeBE16(buf, 1);
  writeBE16(buf, 1);

  size_t m31_start = buf.size();

  buf.insert(buf.end(), { 'K', 'T', 'L', 'X' });
  writeBE32(buf, radial_time_msecs);
  writeBE16(buf, 18765);
  writeBE16(buf, static_cast<uint16_t>(r_index));
  writeBEFloat(buf, az);
  buf.push_back(0);
  buf.push_back(0);
  writeBE16(buf, 0);
  buf.push_back(1); // az_spacing = 1 (0.5 deg)
  buf.push_back(radial_status);
  buf.push_back(1); // elevation_no = 1
  buf.push_back(0);
  writeBEFloat(buf, elev);
  buf.push_back(0);
  buf.push_back(0);
  writeBE16(buf, num_blocks);

  if (num_blocks > 0) {
    size_t ptrs_pos = buf.size();
    for (int i = 0; i < num_blocks; ++i) {
      writeBE32(buf, 0);
    }

    int32_t vol_offset = static_cast<int32_t>(buf.size() - m31_start);
    buf.insert(buf.end(), { 'R', 'V', 'O', 'L' });
    writeBE16(buf, 44);
    buf.push_back(2);
    buf.push_back(0);
    writeBEFloat(buf, 35.333f);
    writeBEFloat(buf, -97.278f);
    writeBE16(buf, 370);
    writeBE16(buf, 20);
    writeBEFloat(buf, 0.0f);
    writeBEFloat(buf, 450.0f);
    writeBEFloat(buf, 450.0f);
    writeBEFloat(buf, 0.0f);
    writeBEFloat(buf, 0.0f);
    writeBE16(buf, 212);
    writeBE16(buf, 0);

    int32_t elv_offset = static_cast<int32_t>(buf.size() - m31_start);
    buf.insert(buf.end(), { 'R', 'E', 'L', 'V' });
    writeBE16(buf, 12);
    writeBE16(buf, 0);
    writeBEFloat(buf, 0.0f);

    int32_t rad_offset = static_cast<int32_t>(buf.size() - m31_start);
    buf.insert(buf.end(), { 'R', 'R', 'A', 'D' });
    writeBE16(buf, 20);
    writeBE16(buf, 0);
    writeBEFloat(buf, -80.0f);
    writeBEFloat(buf, -80.0f);
    writeBE16(buf, 3000);
    writeBE16(buf, 0);

    int32_t ref_offset = static_cast<int32_t>(buf.size() - m31_start);
    buf.insert(buf.end(), { 'D', 'R', 'E', 'F' });
    writeBE32(buf, 0);
    writeBE16(buf, 4);    // 4 gates
    writeBE16(buf, 2125); // first gate center (2125 - 125 = 2000m leading edge)
    writeBE16(buf, 250);  // gate width
    writeBE16(buf, 0);
    writeBE16(buf, 0);
    buf.push_back(0);
    buf.push_back(8);
    writeBEFloat(buf, 2.0f);
    writeBEFloat(buf, 66.0f);
    buf.push_back(0);   // Missing
    buf.push_back(1);   // RangeFolded -> DataUnavailable for REF
    buf.push_back(66);  // 0.0 dBZ
    buf.push_back(100); // 17.0 dBZ

    uint32_t be_vol = htobe32(vol_offset), be_elv = htobe32(elv_offset);
    uint32_t be_rad = htobe32(rad_offset), be_ref = htobe32(ref_offset);
    std::memcpy(&buf[ptrs_pos + 0], &be_vol, 4);
    std::memcpy(&buf[ptrs_pos + 4], &be_elv, 4);
    std::memcpy(&buf[ptrs_pos + 8], &be_rad, 4);
    std::memcpy(&buf[ptrs_pos + 12], &be_ref, 4);
  }

  uint16_t be_halfwords = htobe16(static_cast<uint16_t>((buf.size() - msg_hdr_pos) / 2));

  std::memcpy(&buf[msg_hdr_pos], &be_halfwords, 2);
  return buf;
} // createSyntheticM31RadialBytes

std::vector<char>
createSyntheticNEXRAD()
{
  std::vector<char> uncompressed = createSyntheticM31RadialBytes(0.0f, 0.5f, 0, 4, 1, 43200000);
  auto r2 = createSyntheticM31RadialBytes(0.5f, 0.5f, 2, 4, 2, 43200100);

  uncompressed.insert(uncompressed.end(), r2.begin(), r2.end());

  BZIP2DataFilter bzFilter;
  std::vector<char> compressed;

  bzFilter.reverse(uncompressed, compressed);

  std::vector<char> ar2vFile;
  const char * vol_hdr = "AR2V0001.00100000000KTLX";

  ar2vFile.insert(ar2vFile.end(), vol_hdr, vol_hdr + 24);
  writeBE32(ar2vFile, static_cast<uint32_t>(compressed.size()));
  ar2vFile.insert(ar2vFile.end(), compressed.begin(), compressed.end());
  return ar2vFile;
}
} // namespace

BOOST_AUTO_TEST_SUITE(NEXRAD_MODULE_TESTS)

// 1. Core Message 31 decoding, geometry, gate scaling, and RangeFolded handling
BOOST_AUTO_TEST_CASE(TEST_NEXRAD_CORE_DECODING_AND_GEOMETRY)
{
  std::vector<char> ar2vData = createSyntheticNEXRAD();
  auto dt = IODataType::readBuffer<DataType>(ar2vData, "ar2v");

  BOOST_REQUIRE(dt != nullptr);

  auto multi = std::dynamic_pointer_cast<MultiDataType>(dt);
  auto rs    = multi ? std::dynamic_pointer_cast<RadialSet>(multi->getDataType(0)) :
    std::dynamic_pointer_cast<RadialSet>(dt);

  BOOST_REQUIRE(rs != nullptr);

  BOOST_CHECK_EQUAL(rs->getTypeName(), "Reflectivity");
  BOOST_CHECK_EQUAL(rs->getUnits(), "dBZ");
  BOOST_CHECK_EQUAL(rs->getRadarName(), "KTLX");
  BOOST_CHECK_EQUAL(rs->getVCP(), 212);
  BOOST_CHECK_CLOSE(rs->getElevationDegs(), 0.5f, 0.01f);
  BOOST_CHECK_CLOSE(rs->getDistanceToFirstGateM(), 2000.0, 0.01); // 2125 - 0.5 * 250
  BOOST_CHECK_CLOSE(rs->getLocation().getLatitudeDeg(), 35.333f, 0.001f);
  BOOST_CHECK_CLOSE(rs->getLocation().getLongitudeDeg(), -97.278f, 0.001f);
  BOOST_CHECK_CLOSE(rs->getLocation().getHeightKM(), 0.370f, 0.001f);

  // Azimuth center offset: 0.0 - 0.5 * 0.5 = 359.75
  BOOST_CHECK_CLOSE(rs->getAzimuthRef()[0], 359.75f, 0.01f);

  const float * dataPtr = rs->getFloat2DRef().data();

  BOOST_CHECK_EQUAL(dataPtr[0], Constants::MissingData);
  BOOST_CHECK_EQUAL(dataPtr[1], Constants::DataUnavailable); // RangeFolded -> DataUnavailable on REF
  BOOST_CHECK_CLOSE(dataPtr[2], 0.0f, 0.1f);
  BOOST_CHECK_CLOSE(dataPtr[3], 17.0f, 0.1f);

  BOOST_REQUIRE(rs->haveRadialTimeArray());
  BOOST_CHECK_EQUAL(rs->getRadialTimeRef()[0], 0);
  BOOST_CHECK_EQUAL(rs->getRadialTimeRef()[1], 100);
}

// 2. Real Archive-II (.ar2v) file ingest (static & streaming)
BOOST_AUTO_TEST_CASE(TEST_NEXRAD_REAL_ARCHIVE_FILE)
{
  std::string filePath = NEXRAD_TEST_FILE;

  if (!std::filesystem::exists(filePath)) { filePath = "KATX20261110_065712_V06.ar2v"; }
  if (!std::filesystem::exists(filePath)) { filePath = "modules/ionexrad/KATX20261110_065712_V06.ar2v"; }
  BOOST_REQUIRE_MESSAGE(std::filesystem::exists(filePath), "Test file not found: " << filePath);

  IOConfig config;

  config.setParams(filePath);
  config.set("vcp", "12");
  config.set("useVolumeTime", "true");

  auto dt    = IODataType::read("ar2v", config);
  auto multi = std::dynamic_pointer_cast<MultiDataType>(dt);

  BOOST_REQUIRE(multi != nullptr);
  BOOST_CHECK_GT(multi->size(), 10);

  bool foundRef = false, foundVel = false, foundZdr = false;

  for (size_t i = 0; i < multi->size(); ++i) {
    auto rs = std::dynamic_pointer_cast<RadialSet>(multi->getDataType(i));
    if (!rs) { continue; }
    BOOST_CHECK_EQUAL(rs->getVCP(), 12);
    if (rs->getTypeName() == "Reflectivity") { foundRef = true; } else if (rs->getTypeName() == "AliasedVelocity") {
      foundVel = true;
    } else if (rs->getTypeName() == "Zdr") { foundZdr = true; }
  }
  BOOST_CHECK(foundRef && foundVel && foundZdr);
}

// 3. LDM Streaming: fragmented chunks, negative record lengths, raw BZh, null padding, and resync
BOOST_AUTO_TEST_CASE(TEST_NEXRAD_LDM_STREAMING_AND_RESYNC)
{
  std::vector<char> validData = createSyntheticNEXRAD();

  // A. Fragmented 11-byte streaming with negative LDM record length and trailing null padding
  std::vector<char> negLenData = validData;
  uint32_t origLen = decodeBE32(negLenData.data() + 24);
  uint32_t beNeg   = htobe32(static_cast<uint32_t>(-static_cast<int32_t>(origLen)));

  std::memcpy(negLenData.data() + 24, &beNeg, 4);
  negLenData.insert(negLenData.end(), 19, 0); // Trailing null spool padding

  NEXRADStreamOptions opts;

  opts.radarName = "KTLX";
  NEXRADStreamReader streamReader(opts);
  size_t sweepCount = 0;

  streamReader.setSweepCallback([&](const std::vector<std::shared_ptr<DataType> >&) {
    sweepCount++;
  });

  for (size_t off = 0; off < negLenData.size(); off += 11) {
    streamReader.pushBytes(negLenData.data() + off, std::min<size_t>(11, negLenData.size() - off));
  }
  streamReader.flush();
  BOOST_CHECK_EQUAL(sweepCount, 1);

  // B. Corrupted stream resynchronization (unaligned noise + false BZh magic + concatenated volume)
  streamReader.reset();
  sweepCount = 0;
  std::vector<char> noisyStream(validData.begin(), validData.begin() + 24);

  noisyStream.insert(noisyStream.end(), { 'X', 'B', 'Z', 'h', '0', 'Y' }); // False BZh magic
  noisyStream.insert(noisyStream.end(), validData.begin() + 24, validData.end());
  noisyStream.insert(noisyStream.end(), validData.begin(), validData.end()); // Second volume

  streamReader.pushBytes(noisyStream.data(), noisyStream.size());
  streamReader.flush();
  BOOST_CHECK_EQUAL(sweepCount, 2);

  // C. Headerless raw BZh stream (-H)
  opts.headerMissing = true;
  NEXRADStreamReader rawBzhReader(opts);

  sweepCount = 0;
  rawBzhReader.setSweepCallback([&](const std::vector<std::shared_ptr<DataType> >&) {
    sweepCount++;
  });
  rawBzhReader.pushBytes(validData.data() + 28, validData.size() - 28);
  rawBzhReader.flush();
  BOOST_CHECK_EQUAL(sweepCount, 1);
}

// 4. VCP, Split-Cut Multi-PRF, SAILS/MRLE/MPDA, Lower Tilts, and TDWR SPG rules
BOOST_AUTO_TEST_CASE(TEST_NEXRAD_VCP_SAILS_AND_MULTI_PRF)
{
  NEXRADAccumulator acc;

  acc.setRadarName("KTLX");
  acc.setSeparateSAILS(true);
  acc.setAddLower(false);
  acc.setKeepCompletedRadialSets(true);

  NEXRADMessage5VCP vcp;

  vcp.pattern_number        = 212;
  vcp.vcp_supplemental_data = HW10_SAILS_VCP;

  // Populate all 4 cuts in VCP so elevation_no <= vcp.elevations.size()
  NEXRADMessage5VCP::VCPElevation cut1;

  cut1.elevation_angle = 88; // 0.483 deg -> rounds to 0.5 deg when userVCP == 0
  cut1.waveform_type   = 1;
  vcp.elevations.push_back(cut1); // Cut 1: Surveillance 0.5 deg

  NEXRADMessage5VCP::VCPElevation cut2 = cut1;

  cut2.waveform_type = 2;
  vcp.elevations.push_back(cut2); // Cut 2: Doppler 0.5 deg

  NEXRADMessage5VCP::VCPElevation cut3 = cut1;

  cut3.sector1_spare = HWE15_SAILS_CUT;
  vcp.elevations.push_back(cut3); // Cut 3: SAILS 0.5 deg

  NEXRADMessage5VCP::VCPElevation cut4;

  cut4.elevation_angle = 36; // ~0.2 deg
  cut4.waveform_type   = 1;
  vcp.elevations.push_back(cut4); // Cut 4: Lower tilt 0.2 deg

  acc.setVCP(vcp);

  NEXRADMessage31MomentBlock mb;

  mb.num_gates = 4;
  mb.data.assign(4, 25.0f);

  auto sendTwoRadialSweep = [&](uint8_t elev_no, float elev, uint16_t sector_spare, bool withVel) {
      NEXRADMessage31Radial r;

      std::memcpy(r.radar, "KTLX", 4);
      r.elevation_no   = elev_no;
      r.elevation      = elev;
      r.radial_status  = 0;
      r.has_elev_block = true;
      r.elev_block.sector1_spare = sector_spare;
      r.moments["REF"] = mb;
      if (withVel) { r.moments["VEL"] = mb; }
      acc.processRadial(r);
      r.radial_status = 2;
      acc.processRadial(r);
    };

  // Cut 1: 0.5 deg surveillance -> Reflectivity
  sendTwoRadialSweep(1, 0.5f, 0, false);
  // Cut 2: 0.5 deg Doppler split cut -> ReflectivityPRF2 + AliasedVelocity
  sendTwoRadialSweep(2, 0.5f, 0, true);
  // Cut 3: SAILS cut at 0.5 deg -> resets PRF to 1 -> Reflectivity_EXTRA
  sendTwoRadialSweep(3, 0.5f, HWE15_SAILS_CUT, false);
  // Cut 4: Lower tilt (0.2 deg) with addLower=false -> Reflectivity_EXTRA
  sendTwoRadialSweep(4, 0.2f, 0, false);

  const auto& sets = acc.getRadialSets();

  BOOST_REQUIRE_EQUAL(sets.size(), 5);
  BOOST_CHECK_EQUAL(sets[0]->getTypeName(), "Reflectivity");
  BOOST_CHECK_EQUAL(sets[1]->getTypeName(), "ReflectivityPRF2");
  BOOST_CHECK_EQUAL(sets[2]->getTypeName(), "AliasedVelocity");
  BOOST_CHECK_EQUAL(sets[3]->getTypeName(), "Reflectivity_EXTRA");
  BOOST_CHECK_EQUAL(sets[4]->getTypeName(), "Reflectivity_EXTRA");

  // Verify TDWR SPG (C-Band over Message 31): strategy 1 -> VCP 90, waveform 1 -> ReflectivityLowPRF
  acc.reset();
  acc.setRadarName("TOKC");
  NEXRADMessage31Radial tdwrRad;

  std::memcpy(tdwrRad.radar, "TOKC", 4);
  tdwrRad.elevation_no   = 1;
  tdwrRad.elevation      = 0.6f;
  tdwrRad.radial_status  = 3;
  tdwrRad.has_vol_block  = true;
  tdwrRad.vol_block.vcp  = 1;
  tdwrRad.has_elev_block = true;
  tdwrRad.elev_block.waveform_type = 1;
  tdwrRad.moments["REF"] = mb;
  acc.processRadial(tdwrRad);
  tdwrRad.radial_status = 4;
  acc.processRadial(tdwrRad);

  BOOST_REQUIRE_EQUAL(acc.getRadialSets().size(), 1);
  BOOST_CHECK_EQUAL(acc.getRadialSets()[0]->getTypeName(), "ReflectivityLowPRF");
  BOOST_CHECK_EQUAL(acc.getRadialSets()[0]->getVCP(), 90);
}

// 5. Legacy Message 1 radial decoding and conversion
BOOST_AUTO_TEST_CASE(TEST_NEXRAD_LEGACY_MESSAGE1)
{
  std::vector<char> rawRadial(2432, 0);
  uint16_t msg_size = htobe16(1208);

  std::memcpy(&rawRadial[12], &msg_size, 2);
  rawRadial[15] = 1; // Message 1
  uint16_t num_segs = htobe16(1);

  std::memcpy(&rawRadial[24], &num_segs, 2);
  std::memcpy(&rawRadial[26], &num_segs, 2);

  uint16_t az_raw = htobe16(static_cast<uint16_t>(180.0f * (4096.0f / 180.0f) * 8.0f));
  uint16_t el_raw = htobe16(static_cast<uint16_t>(0.5f * (4096.0f / 180.0f) * 8.0f));
  int16_t range_1000 = htobe16(1000), gw_1000 = htobe16(1000), gates_3 = htobe16(3), ptr_100 = htobe16(100);

  std::memcpy(&rawRadial[36], &az_raw, 2);
  std::memcpy(&rawRadial[42], &el_raw, 2);
  std::memcpy(&rawRadial[44], &num_segs, 2);
  std::memcpy(&rawRadial[46], &range_1000, 2);
  std::memcpy(&rawRadial[50], &gw_1000, 2);
  std::memcpy(&rawRadial[54], &gates_3, 2);
  std::memcpy(&rawRadial[64], &ptr_100, 2);

  rawRadial[128 + 0] = 0;  // Missing
  rawRadial[128 + 1] = 1;  // RangeFolded
  rawRadial[128 + 2] = 66; // 0.0 dBZ

  NEXRADMessage1Radial m1;

  BOOST_REQUIRE(NEXRADMessage1Radial::read(rawRadial.data(), rawRadial.size(), m1));
  NEXRADMessage31Radial m31 = m1.toMessage31Radial("KTLX");

  BOOST_CHECK_EQUAL(m31.moments["REF"].dist_to_center_of_first_gate, 1500);
  BOOST_CHECK_EQUAL(m31.moments["REF"].data[0], Constants::MissingData);
  BOOST_CHECK_EQUAL(m31.moments["REF"].data[1], Constants::RangeFolded);
  BOOST_CHECK_CLOSE(m31.moments["REF"].data[2], 0.0f, 0.01f);
}

// 6. Metadata DataTables: Message 2 (RDAStatus), Message 3 (Performance), and HVAzNoise
BOOST_AUTO_TEST_CASE(TEST_NEXRAD_METADATA_TABLES)
{
  NEXRADAccumulator acc;

  acc.setWriteHVAN(true);
  Time volTime(2023, 5, 20, 14, 0, 0);

  acc.setUseVolumeTime(true);
  acc.setVolumeStartTime(volTime);

  NEXRADMessage2RDAStatus msg2;

  msg2.rda_status       = 16;
  msg2.rda_build_number = 1900;
  acc.processMessage2(msg2);

  NEXRADMessage3Data msg3;

  msg3.communications.loop_back_test_status = 1;
  msg3.ame.ame_internal_temperature         = 24.5f;
  acc.processMessage3(msg3);

  std::vector<std::shared_ptr<DataType> > outputs;

  acc.setSweepCallback([&](const std::vector<std::shared_ptr<DataType> >& sweep) {
    outputs = sweep;
  });

  for (int r = 0; r < 2; ++r) {
    NEXRADMessage31Radial rad;
    std::memcpy(rad.radar, "KTLX", 4);
    rad.elevation_no                 = 1;
    rad.elevation                    = 0.5f;
    rad.azimuth                      = r * 90.0f;
    rad.radial_status                = (r == 0) ? 0 : 2;
    rad.radial_time_days             = 19498;
    rad.radial_time_msecs            = 50730000 + r * 100;
    rad.has_vol_block                = true;
    rad.vol_block.site_lat           = 35.333f;
    rad.vol_block.site_lon           = -97.278f;
    rad.has_elev_block               = true;
    rad.elev_block.waveform_type     = 1;
    rad.has_rad_block                = true;
    rad.radial_block.hor_noise_level = -75.0f;
    rad.radial_block.ver_noise_level = -76.0f;
    NEXRADMessage31MomentBlock mb;
    mb.num_gates       = 2;
    mb.data            = { 20.0f, 25.0f };
    rad.moments["REF"] = mb;
    acc.processRadial(rad);
  }

  bool foundHVAN = false, foundMsg2 = false, foundMsg3 = false;

  for (const auto& dt : outputs) {
    if (auto tbl = std::dynamic_pointer_cast<DataTable>(dt)) {
      std::string fdt;
      BOOST_CHECK(tbl->getString(Constants::FilenameDateTime, fdt));
      BOOST_CHECK_EQUAL(fdt, "20230520-140000");
      if (tbl->getTypeName() == "HVAzNoiseTable") {
        foundHVAN = true;
        BOOST_CHECK_EQUAL(tbl->getRowCount(), 2);
      } else if (tbl->getTypeName() == "RDAStatusTable") {
        foundMsg2 = true;
        BOOST_CHECK_EQUAL(tbl->getRowCount(), 14);
        BOOST_CHECK_CLOSE(tbl->getColumn("RDABuildNumber").getCellAsFloat(0), 19.0f, 0.01f);
      } else if (tbl->getTypeName() == "MSG3DataTable") {
        foundMsg3 = true;
        BOOST_CHECK_EQUAL(tbl->getRowCount(), 1);
        BOOST_CHECK_CLOSE(tbl->getColumn("ame_internal_temperature").getCellAsFloat(0), 24.5f, 0.01f);
      }
    }
  }
  BOOST_CHECK(foundHVAN && foundMsg2 && foundMsg3);
}

// 7. Azimuthal Alignment (-A) and Nyquist Mutually Exclusive Storage
BOOST_AUTO_TEST_CASE(TEST_NEXRAD_ALIGNMENT_AND_NYQUIST)
{
  NEXRADAccumulator acc;

  acc.setRadarName("KTLX");
  acc.setAlignmentResolution(1.0f);

  for (int i = 0; i < 360; ++i) {
    NEXRADMessage31Radial rad;
    std::memcpy(rad.radar, "KTLX", 4);
    rad.elevation_no  = 1;
    rad.elevation     = 0.5f;
    rad.azimuth       = std::fmod(180.0f + static_cast<float>(i), 360.0f);
    rad.az_spacing    = 2;
    rad.radial_status = (i == 0) ? 0 : ((i == 359) ? 2 : 1);
    rad.has_rad_block = true;
    rad.radial_block.nyquist_vel = (i < 180) ? 2500 : 3200; // Variable Nyquist -> 1D array
    NEXRADMessage31MomentBlock mb;
    mb.num_gates = 2;
    mb.dist_to_center_of_first_gate = 1000;
    mb.range_sample_interval        = 250;
    mb.data.assign(2, 100.0f + static_cast<float>(i));
    rad.moments["REF"] = mb;
    acc.processRadial(rad);
  }
  acc.flush();

  const auto& rsets = acc.getRadialSets();

  BOOST_REQUIRE_EQUAL(rsets.size(), 1);
  auto rs = rsets.front();

  BOOST_CHECK_EQUAL(rs->getNumRadials(), 360);
  BOOST_CHECK_CLOSE(rs->getAzimuthRef()[0], 0.0f, 1e-4f);
  BOOST_CHECK_CLOSE(rs->getFloat2DRef()[0][0], 280.0f, 1e-3f);
  BOOST_CHECK_CLOSE(rs->getFloat2DRef()[180][0], 100.0f, 1e-3f);
  BOOST_CHECK(rs->haveNyquistArray());
}

// 8. Directory Listener, NetCDF Serialization, and FML Notification Formatting
BOOST_AUTO_TEST_CASE(TEST_NEXRAD_DIRECTORY_AND_NETCDF_FML_OUTPUT)
{
  namespace fs = std::filesystem;
  fs::path tempInDir  = fs::temp_directory_path() / "test_nexrad_in";
  fs::path tempOutDir = fs::temp_directory_path() / "test_nexrad_out";

  fs::remove_all(tempInDir);
  fs::remove_all(tempOutDir);
  fs::create_directories(tempInDir);
  fs::create_directories(tempOutDir);

  std::vector<char> ar2vData = createSyntheticNEXRAD();
  {
    std::ofstream ofs((tempInDir / "KATX_match.ar2v").string(), std::ios::binary);
    ofs.write(ar2vData.data(), ar2vData.size());
  }
  {
    std::ofstream ofs((tempInDir / "OTHER_KATX_ignore.ar2v").string(), std::ios::binary);
    ofs.write(ar2vData.data(), ar2vData.size());
  }

  NEXRADStreamOptions opts;

  opts.inputDir  = tempInDir.string();
  opts.outputDir = tempOutDir.string();
  opts.radarName = "KATX";
  opts.pattern   = "KATX"; // Strict prefix match
  opts.readAll   = true;
  opts.onceOnly  = true;
  opts.killFiles = true;

  NEXRADStreamReader streamReader(opts);

  BOOST_CHECK_EQUAL(streamReader.listenDirectory(), 1);
  BOOST_CHECK(!fs::exists(tempInDir / "KATX_match.ar2v"));
  BOOST_CHECK(fs::exists(tempInDir / "OTHER_KATX_ignore.ar2v"));

  bool foundFML = false;

  for (const auto& entry : fs::directory_iterator(tempOutDir / "code_index.fam")) {
    if (entry.path().extension() == ".fml") {
      std::ifstream ifs(entry.path().string());
      std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
      if ((content.find("<params>netcdf {indexlocation}") != std::string::npos) &&
        (content.find("Reflectivity/00.50/") != std::string::npos) )
      {
        foundFML = true;
      }
    }
  }
  BOOST_CHECK(foundFML);

  fs::remove_all(tempInDir);
  fs::remove_all(tempOutDir);
}

BOOST_AUTO_TEST_SUITE_END()
