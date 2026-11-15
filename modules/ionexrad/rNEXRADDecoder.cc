#include "rNEXRADDecoder.h"
#include "rNEXRADAccumulator.h"
#include <rLDMStreamReader.h>
#include <rIOIndex.h>
#include <rDataFilter.h>
#include <rIODataType.h>
#include <rPluginNotifier.h>
#include <rFMLRecordNotifier.h>
#include <rError.h>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace rapio {
// ----------------------------------------------------------------------
// NEXRADVolumeHeader
// ----------------------------------------------------------------------
bool
NEXRADVolumeHeader::isValid() const
{
  return (std::strncmp(archive, "AR2V00", 6) == 0 ||
         std::strncmp(archive, "ARCHIVE2", 8) == 0);
}

Time
NEXRADVolumeHeader::getVolumeStartTime() const
{
  return nexradTimeToRapioTime(static_cast<uint16_t>(create_days), create_msecs);
}

bool
NEXRADVolumeHeader::read(const char * buf, size_t buflen, NEXRADVolumeHeader& out)
{
  if (buflen < size()) {
    return false;
  }
  std::memcpy(out.archive, buf, 9);
  std::memcpy(out.extension, buf + 9, 3);
  out.create_days = decodeBES32(buf + 12);
  if (out.create_days > 0xffff) {
    out.create_days = ((out.create_days & 0x0000FFFF) << 16)
      | ((static_cast<uint32_t>(out.create_days) & 0xFFFF0000U) >> 16);
  }
  out.create_msecs = decodeBES32(buf + 16);
  std::memcpy(out.radar_name, buf + 20, 4);
  out.radar_name[4] = '\0';
  return out.isValid();
}

// ----------------------------------------------------------------------
// NEXRADMessageHeader
// ----------------------------------------------------------------------
size_t
NEXRADMessageHeader::getTotalBytes() const
{
  if (message_type == 31) {
    // msg_size is in halfwords from this header to end of record
    int data_length = msg_size * 2 - 16;
    if (data_length < 0) { data_length = 0; }
    return 12 + 16 + static_cast<size_t>(data_length);
  }
  // Constant padded length for legacy message 1 / other messages (2432 bytes total including 12-byte CTM)
  return 2432;
}

bool
NEXRADMessageHeader::read(const char * buf, size_t buflen, NEXRADMessageHeader& out)
{
  // buf points to start of 12-byte CTM transmission header + 16-byte MessageHeader
  if (buflen < 28) {
    return false;
  }
  const char * p = buf + 12; // Skip 12-byte CTM header

  out.msg_size       = decodeBE16(p);
  out.channel_id     = static_cast<uint8_t>(p[2]);
  out.message_type   = static_cast<uint8_t>(p[3]);
  out.seq_id         = decodeBE16(p + 4);
  out.gen_time_days  = decodeBE16(p + 6);
  out.gen_time_msecs = decodeBES32(p + 8);
  out.num_segments   = decodeBES16(p + 12);
  out.segment_no     = decodeBES16(p + 14);
  return true;
}

// NEXRADMessage31Radial
// ----------------------------------------------------------------------
bool
NEXRADMessage31Radial::read(const char * buf, size_t buflen, NEXRADMessage31Radial& out)
{
  if (buflen < 28 + 32) {
    return false;
  }
  const char * msg_start = buf + 28; // Start of Message 31 data structures

  std::memcpy(out.radar, msg_start, 4);
  out.radar[4] = '\0';
  out.radial_time_msecs = decodeBES32(msg_start + 4);
  out.radial_time_days  = decodeBE16(msg_start + 8);
  out.az_no                       = decodeBE16(msg_start + 10);
  out.azimuth                     = decodeBEFloat(msg_start + 12);
  out.compress_flag               = static_cast<uint8_t>(msg_start[16]);
  out.spare                       = static_cast<uint8_t>(msg_start[17]);
  out.uncompressed_length         = decodeBES16(msg_start + 18);
  out.az_spacing                  = static_cast<uint8_t>(msg_start[20]);
  out.radial_status               = static_cast<uint8_t>(msg_start[21]);
  out.elevation_no                = static_cast<uint8_t>(msg_start[22]);
  out.sector_no                   = static_cast<uint8_t>(msg_start[23]);
  out.elevation                   = decodeBEFloat(msg_start + 24);
  out.radial_spot_blanking_status = static_cast<uint8_t>(msg_start[28]);
  out.azimuth_indexing_mode       = static_cast<uint8_t>(msg_start[29]);
  out.num_data_blocks             = decodeBES16(msg_start + 30);

  if ((out.num_data_blocks <= 0) || (out.num_data_blocks > 16)) {
    return false;
  }

  size_t ptrs_offset = 32;

  if (buflen < 28 + ptrs_offset + static_cast<size_t>(out.num_data_blocks) * 4) {
    return false;
  }

  std::vector<int32_t> block_pointers;

  block_pointers.reserve(out.num_data_blocks);
  for (int i = 0; i < out.num_data_blocks; ++i) {
    block_pointers.push_back(decodeBES32(msg_start + ptrs_offset + i * 4));
  }

  for (int i = 0; i < out.num_data_blocks; ++i) {
    int32_t ptr = block_pointers[i];
    if (ptr <= 0) {
      continue;
    }
    size_t uptr = static_cast<size_t>(ptr);
    if ((28 + uptr + 4) > buflen) {
      continue;
    }
    const char * bp = msg_start + uptr;
    char block_name[4];
    std::memcpy(block_name, bp + 1, 3);
    block_name[3] = '\0';

    if (std::strcmp(block_name, "VOL") == 0) {
      if ((28 + uptr + 6) <= buflen) {
        uint16_t blockSize = decodeBE16(bp + 4);
        if ((blockSize >= 44) && ((28 + uptr + blockSize) <= buflen)) {
          out.vol_block.data_block_size = blockSize;
          out.vol_block.major_version   = static_cast<uint8_t>(bp[6]);
          out.vol_block.minor_version   = static_cast<uint8_t>(bp[7]);
          out.vol_block.site_lat        = decodeBEFloat(bp + 8);
          out.vol_block.site_lon        = decodeBEFloat(bp + 12);
          out.vol_block.site_ht         = decodeBES16(bp + 16);
          out.vol_block.feedhorn_ht_above_ground  = decodeBES16(bp + 18);
          out.vol_block.dbz0_calibration_constant = decodeBEFloat(bp + 20);
          out.vol_block.hor_shv_tx_power = decodeBEFloat(bp + 24);
          out.vol_block.ver_shv_tx_power = decodeBEFloat(bp + 28);
          out.vol_block.zdr_calibration  = decodeBEFloat(bp + 32);
          out.vol_block.phidp_initial    = decodeBEFloat(bp + 36);
          out.vol_block.vcp = decodeBES16(bp + 40);
          out.vol_block.processing_status = decodeBES16(bp + 42);
          if ((out.vol_block.major_version > 2) && (blockSize >= 46)) {
            out.vol_block.zdr_bias_estimate_weighted_mean = decodeBES16(bp + 44);
          }
          out.has_vol_block = true;
        }
      }
    } else if (std::strcmp(block_name, "ELV") == 0) {
      if ((28 + uptr + 6) <= buflen) {
        uint16_t blockSize = decodeBE16(bp + 4);
        if ((blockSize >= 12) && ((28 + uptr + blockSize) <= buflen)) {
          out.elev_block.data_block_size = blockSize;
          out.elev_block.atmos_attenuation_factor  = decodeBES16(bp + 6);
          out.elev_block.dbz0_calibration_constant = decodeBEFloat(bp + 8);
          out.has_elev_block = true;
        }
      }
    } else if (std::strcmp(block_name, "RAD") == 0) {
      if ((28 + uptr + 6) <= buflen) {
        uint16_t blockSize = decodeBE16(bp + 4);
        if ((blockSize >= 20) && ((28 + uptr + blockSize) <= buflen)) {
          out.radial_block.data_block_size = blockSize;
          out.radial_block.unamb_range     = decodeBES16(bp + 6);
          out.radial_block.hor_noise_level = decodeBEFloat(bp + 8);
          out.radial_block.ver_noise_level = decodeBEFloat(bp + 12);
          out.radial_block.nyquist_vel     = decodeBES16(bp + 16);
          if (blockSize >= 28) {
            out.radial_block.cal_const_hor = decodeBEFloat(bp + 20);
            out.radial_block.cal_const_ver = decodeBEFloat(bp + 24);
          }
          out.has_rad_block = true;
        }
      }
    } else {
      // Moment block (REF, VEL, SW, ZDR, PHI, RHO, SNR, SND)
      if ((28 + uptr + 28) <= buflen) {
        NEXRADMessage31MomentBlock mb;
        std::string name_str(block_name);
        while (!name_str.empty() && (name_str.back() == ' ' || name_str.back() == '\0')) {
          name_str.pop_back();
        }
        mb.data_name = name_str;
        mb.reserved  = decodeBE32(bp + 4);
        mb.num_gates = decodeBE16(bp + 8);
        mb.dist_to_center_of_first_gate = decodeBES16(bp + 10);
        mb.range_sample_interval        = decodeBES16(bp + 12);
        mb.threshold_for_overlayed      = decodeBES16(bp + 14);
        mb.snr_threshold  = decodeBES16(bp + 16);
        mb.ctrl_flags     = static_cast<uint8_t>(bp[18]);
        mb.data_word_size = static_cast<uint8_t>(bp[19]);
        mb.scale  = decodeBEFloat(bp + 20);
        mb.offset = decodeBEFloat(bp + 24);

        if ((mb.num_gates > 0) &&
          ((mb.data_word_size == 8) || (mb.data_word_size == 16) || (mb.data_word_size == 32)))
        {
          size_t gateBytes =
            (mb.data_word_size == 32) ? (static_cast<size_t>(mb.num_gates) * 4) :
            ((mb.data_word_size == 16) ? (static_cast<size_t>(mb.num_gates) * 2) :
            static_cast<size_t>(mb.num_gates));
          size_t evaluatedBlockSize = 28 + gateBytes;
          if ((28 + uptr + evaluatedBlockSize) <= buflen) {
            mb.data.resize(mb.num_gates);
            const char * gate_data = bp + 28;
            const float scale      = (mb.scale == 0.0f) ? 1.0f : mb.scale;
            if (mb.data_word_size == 8) {
              for (size_t g = 0; g < mb.num_gates; ++g) {
                uint8_t raw = static_cast<uint8_t>(gate_data[g]);
                if (raw == 0) {
                  mb.data[g] = Constants::MissingData;
                } else if (raw == 1) {
                  mb.data[g] = Constants::RangeFolded;
                } else {
                  mb.data[g] = (raw - mb.offset) / scale;
                }
              }
            } else if (mb.data_word_size == 16) {
              for (size_t g = 0; g < mb.num_gates; ++g) {
                uint16_t raw = decodeBE16(gate_data + g * 2);
                if (raw == 0) {
                  mb.data[g] = Constants::MissingData;
                } else if (raw == 1) {
                  mb.data[g] = Constants::RangeFolded;
                } else {
                  mb.data[g] = (raw - mb.offset) / scale;
                }
              }
            } else if (mb.data_word_size == 32) {
              for (size_t g = 0; g < mb.num_gates; ++g) {
                float val = decodeBEFloat(gate_data + g * 4);
                if (mb.scale == 0.0f) {
                  mb.data[g] = val;
                } else {
                  mb.data[g] = (val - mb.offset) / scale;
                }
              }
            }
            out.moments[name_str] = std::move(mb);
          }
        }
      }
    }
  }

  return true;
} // NEXRADMessage31Radial::read

// ----------------------------------------------------------------------
// NEXRADMessage1Radial
// ----------------------------------------------------------------------
bool
NEXRADMessage1Radial::read(const char * buf, size_t buflen, NEXRADMessage1Radial& out)
{
  if (buflen < 128) {
    return false;
  }
  const char * p = buf + 28; // Start of Message 1 RadialHeader

  out.radial_time_msecs = decodeBES32(p);
  out.radial_time_days  = decodeBE16(p + 4);
  out.unamb_range       = decodeBES16(p + 6);
  out.azimuth       = decodeBE16(p + 8);
  out.azimuth_no    = decodeBE16(p + 10);
  out.radial_status = decodeBE16(p + 12);
  out.elevation     = decodeBE16(p + 14);
  out.elevation_no  = decodeBES16(p + 16);
  out.range_to_first_gate_ref = decodeBES16(p + 18);
  out.range_to_first_gate_vel = decodeBES16(p + 20);
  out.gate_width_ref = decodeBES16(p + 22);
  out.gate_width_vel = decodeBES16(p + 24);
  out.num_bins_ref   = decodeBE16(p + 26);
  out.num_bins_vel   = decodeBE16(p + 28);
  out.sector_no      = decodeBE16(p + 30);
  out.calib_const    = decodeBEFloat(p + 32);
  out.ref_offset     = decodeBE16(p + 36);
  out.vel_offset     = decodeBE16(p + 38);
  out.spw_offset     = decodeBE16(p + 40);
  out.vel_resolution = decodeBE16(p + 42);
  out.vcp         = decodeBES16(p + 44);
  out.nyquist_vel = decodeBES16(p + 60);

  // Read REF gates
  out.ref_data.clear();
  if ((out.ref_offset > 0) && (out.num_bins_ref > 0)) {
    size_t refStart = 28 + static_cast<size_t>(out.ref_offset);
    if (refStart + out.num_bins_ref <= buflen) {
      out.ref_data.resize(out.num_bins_ref);
      const auto * dp = reinterpret_cast<const uint8_t *>(buf + refStart);
      for (size_t i = 0; i < out.num_bins_ref; ++i) {
        uint8_t raw = dp[i];
        if (raw == 0) {
          out.ref_data[i] = Constants::MissingData;
        } else if (raw == 1) {
          out.ref_data[i] = Constants::RangeFolded;
        } else {
          out.ref_data[i] = (static_cast<float>(raw) - 2.0f) / 2.0f - 32.0f;
        }
      }
    }
  }

  // Read VEL gates
  out.vel_data.clear();
  if ((out.vel_offset > 0) && (out.num_bins_vel > 0)) {
    size_t velStart = 28 + static_cast<size_t>(out.vel_offset);
    if (velStart + out.num_bins_vel <= buflen) {
      out.vel_data.resize(out.num_bins_vel);
      const auto * dp = reinterpret_cast<const uint8_t *>(buf + velStart);
      float divisor   = (out.vel_resolution == 2) ? 2.0f : 1.0f;
      float offset    = (out.vel_resolution == 2) ? 63.5f : 127.0f;
      for (size_t i = 0; i < out.num_bins_vel; ++i) {
        uint8_t raw = dp[i];
        if (raw == 0) {
          out.vel_data[i] = Constants::MissingData;
        } else if (raw == 1) {
          out.vel_data[i] = Constants::RangeFolded;
        } else {
          out.vel_data[i] = (static_cast<float>(raw) - 2.0f) / divisor - offset;
        }
      }
    }
  }

  // Read SPW gates
  out.spw_data.clear();
  if ((out.spw_offset > 0) && (out.num_bins_vel > 0)) {
    size_t spwStart = 28 + static_cast<size_t>(out.spw_offset);
    if (spwStart + out.num_bins_vel <= buflen) {
      out.spw_data.resize(out.num_bins_vel);
      const auto * dp = reinterpret_cast<const uint8_t *>(buf + spwStart);
      for (size_t i = 0; i < out.num_bins_vel; ++i) {
        uint8_t raw = dp[i];
        if (raw == 0) {
          out.spw_data[i] = Constants::MissingData;
        } else if (raw == 1) {
          out.spw_data[i] = Constants::RangeFolded;
        } else {
          out.spw_data[i] = (static_cast<float>(raw) - 2.0f) / 2.0f - 63.5f;
        }
      }
    }
  }

  return true;
} // NEXRADMessage1Radial::read

NEXRADMessage31Radial
NEXRADMessage1Radial::toMessage31Radial(const std::string& radarName) const
{
  NEXRADMessage31Radial rad;

  if (!radarName.empty()) {
    std::strncpy(rad.radar, radarName.c_str(), 4);
    rad.radar[4] = '\0';
  } else {
    std::memset(rad.radar, 0, sizeof(rad.radar));
  }

  rad.radial_time_msecs = radial_time_msecs;
  rad.radial_time_days  = radial_time_days;
  rad.az_no         = azimuth_no;
  rad.azimuth       = getAzimuth();
  rad.az_spacing    = 2; // 1.0 deg
  rad.radial_status = static_cast<uint8_t>(radial_status);
  rad.elevation_no  = (elevation_no > 0) ? static_cast<uint8_t>(elevation_no) : 1;
  rad.sector_no     = static_cast<uint8_t>(sector_no);
  rad.elevation     = getElevation();

  rad.has_rad_block = true;
  rad.radial_block.unamb_range   = unamb_range;
  rad.radial_block.nyquist_vel   = nyquist_vel;
  rad.radial_block.cal_const_hor = calib_const;

  rad.has_vol_block = true;
  rad.vol_block.vcp = vcp;

  rad.has_elev_block = true;
  rad.elev_block.waveform_type = (num_bins_vel > 0) ? 2 : 1; // 1=surveillance, 2=Doppler
  rad.elev_block.dbz0_calibration_constant = calib_const;

  if (!ref_data.empty()) {
    NEXRADMessage31MomentBlock mb;
    mb.data_name = "REF";
    mb.num_gates = static_cast<uint16_t>(ref_data.size());
    mb.dist_to_center_of_first_gate = static_cast<int16_t>(range_to_first_gate_ref + gate_width_ref / 2);
    mb.range_sample_interval        = gate_width_ref;
    mb.scale  = 2.0f;
    mb.offset = 66.0f;
    mb.data   = ref_data;
    rad.moments["REF"] = std::move(mb);
  }

  if (!vel_data.empty()) {
    NEXRADMessage31MomentBlock mb;
    mb.data_name = "VEL";
    mb.num_gates = static_cast<uint16_t>(vel_data.size());
    mb.dist_to_center_of_first_gate = static_cast<int16_t>(range_to_first_gate_vel + gate_width_vel / 2);
    mb.range_sample_interval        = gate_width_vel;
    mb.scale  = (vel_resolution == 2) ? 2.0f : 1.0f;
    mb.offset = 129.0f;
    mb.data   = vel_data;
    rad.moments["VEL"] = std::move(mb);
  }

  if (!spw_data.empty()) {
    NEXRADMessage31MomentBlock mb;
    mb.data_name = "SW";
    mb.num_gates = static_cast<uint16_t>(spw_data.size());
    mb.dist_to_center_of_first_gate = static_cast<int16_t>(range_to_first_gate_vel + gate_width_vel / 2);
    mb.range_sample_interval        = gate_width_vel;
    mb.scale          = 2.0f;
    mb.offset         = 129.0f;
    mb.data           = spw_data;
    rad.moments["SW"] = std::move(mb);
  }

  return rad;
} // NEXRADMessage1Radial::toMessage31Radial

// NEXRADMessage5VCP
// ----------------------------------------------------------------------
bool
NEXRADMessage5VCP::read(const char * buf, size_t buflen, NEXRADMessage5VCP& out)
{
  if (buflen < 28 + 22) {
    return false;
  }
  const char * p = buf + 28;

  out.msg_size              = decodeBES16(p);
  out.pattern_type          = decodeBES16(p + 2);
  out.pattern_number        = decodeBES16(p + 4);
  out.num_elevation_cuts    = decodeBES16(p + 6);
  out.clutter_map_group_num = decodeBES16(p + 8);
  out.doppler_vel_res       = static_cast<uint8_t>(p[10]);
  out.pulse_width           = static_cast<uint8_t>(p[11]);
  // HW 7 (offset 12), HW 8 (offset 14) are spares
  out.vcp_sequencing        = decodeBE16(p + 16); // HW 9
  out.vcp_supplemental_data = decodeBE16(p + 18); // HW 10
  // HW 11 (offset 20) is spare

  // Each elevation cut is 23 halfwords = 46 bytes, starting at byte offset 22
  size_t elev_offset = 22;
  constexpr size_t cut_stride = 46;

  out.elevations.clear();
  if ((out.num_elevation_cuts > 0) && (out.num_elevation_cuts <= 32)) {
    out.elevations.reserve(out.num_elevation_cuts);
    for (int i = 0; i < out.num_elevation_cuts; ++i) {
      if ((28 + elev_offset + (i + 1) * cut_stride) > buflen) {
        break;
      }
      const char * ep = p + elev_offset + i * cut_stride;
      VCPElevation el;
      el.elevation_angle          = decodeBES16(ep);
      el.channel_config           = static_cast<uint8_t>(ep[2]);
      el.waveform_type            = static_cast<uint8_t>(ep[3]);
      el.super_res                = static_cast<uint8_t>(ep[4]);
      el.surveillance_prf_num     = static_cast<uint8_t>(ep[5]);
      el.surveillance_pulse_count = decodeBES16(ep + 6);
      // Sector 1 spare (E15, offset 28 in cut definition) holds SAILS / MRLE cut flags
      el.sector1_spare = decodeBE16(ep + 28);
      out.elevations.push_back(el);
    }
  }
  return true;
} // NEXRADMessage5VCP::read

// NEXRADMessage2RDAStatus
// ----------------------------------------------------------------------
bool
NEXRADMessage2RDAStatus::read(const char * buf, size_t buflen, NEXRADMessage2RDAStatus& out)
{
  if (!buf || (buflen < 120)) {
    return false;
  }

  const char * p = buf;

  if ((buflen >= 28) && (static_cast<uint8_t>(buf[15]) == 2)) {
    if (buflen < 28 + 120) {
      return false;
    }
    uint16_t days = decodeBE16(buf + 18);
    int32_t msecs = decodeBES32(buf + 20);
    out.time = nexradTimeToRapioTime(days, msecs);
    p        = buf + 28;
  }

  out.rda_status                = decodeBES16(p);
  out.operability_status        = decodeBES16(p + 2);
  out.control_status            = decodeBES16(p + 4);
  out.aux_power_generator_state = decodeBES16(p + 6);
  out.avg_transmiter_power      = decodeBE16(p + 8);
  out.horz_reflecitivty_calibration_correction = decodeBES16(p + 10);
  out.data_transmission_enabled      = decodeBES16(p + 12);
  out.volume_coverage_pattern_number = decodeBES16(p + 14);
  out.rda_control_auth        = decodeBES16(p + 16);
  out.rda_build_number        = decodeBE16(p + 18);
  out.operational_mode        = decodeBES16(p + 20);
  out.super_resolution_status = decodeBES16(p + 22);
  out.clutter_mitigation_decision_status = decodeBES16(p + 24);
  out.avset_ebc_log_status        = decodeBES16(p + 26);
  out.rda_alarm_summary           = decodeBES16(p + 28);
  out.command_acknowledgment      = decodeBES16(p + 30);
  out.channel_control_status      = decodeBES16(p + 32);
  out.spot_blanking_status        = decodeBES16(p + 34);
  out.bypass_map_gen_date         = decodeBE16(p + 36);
  out.bypass_map_gen_time         = decodeBE16(p + 38);
  out.clutter_filter_map_gen_date = decodeBE16(p + 40);
  out.clutter_filter_map_gen_time = decodeBE16(p + 42);
  out.vertical_reflectivity_calibration_correction = decodeBES16(p + 44);
  out.transition_power_source_status = decodeBE16(p + 46);
  out.rms_control_status       = decodeBES16(p + 48);
  out.performance_check_status = decodeBES16(p + 50);

  for (size_t i = 0; i < 14; ++i) {
    out.alarm_codes[i] = decodeBE16(p + 52 + i * 2);
  }

  out.signal_processing_options = decodeBES16(p + 80);

  for (size_t i = 0; i < 18; ++i) {
    out.msg2_spares[i] = decodeBE16(p + 82 + i * 2);
  }

  out.status_version = decodeBE16(p + 118);
  return true;
} // NEXRADMessage2RDAStatus::read

// ----------------------------------------------------------------------
// NEXRADMessage3Data
// ----------------------------------------------------------------------
bool
NEXRADMessage3Data::read(const char * buf, size_t buflen, NEXRADMessage3Data& out)
{
  if (!buf || (buflen < 960)) {
    return false;
  }

  const char * p = buf;

  if ((buflen >= 28) && (static_cast<uint8_t>(buf[15]) == 3)) {
    if (buflen < 28 + 960) {
      return false;
    }
    uint16_t days = decodeBE16(buf + 18);
    int32_t msecs = decodeBES32(buf + 20);
    out.time = nexradTimeToRapioTime(days, msecs);
    p        = buf + 28;
  }

  // Sub-struct: NEXRADMessage3Communications (out.communications)
  p += 2; // skip 1 spare halfword
  out.communications.loop_back_test_status = decodeBE16(p);
  p += 2;
  out.communications.t1_output_frames = decodeBE32(p);
  p += 4;
  out.communications.t1_input_frames = decodeBE32(p);
  p += 4;
  out.communications.router_memory_used = decodeBE32(p);
  p += 4;
  out.communications.router_memory_free = decodeBE32(p);
  p += 4;
  out.communications.router_memory_utilization = decodeBE16(p);
  p += 2;
  out.communications.route_to_rpg = decodeBE16(p);
  p += 2;
  out.communications.csu_loss_of_signal = decodeBE32(p);
  p += 4;
  out.communications.csu_loss_of_frames = decodeBE32(p);
  p += 4;
  out.communications.csu_yellow_alarms = decodeBE32(p);
  p += 4;
  out.communications.csu_blue_alarms = decodeBE32(p);
  p += 4;
  out.communications.csu_24hr_errored_seconds = decodeBE32(p);
  p += 4;
  out.communications.csu_24hr_severely_errored_seconds = decodeBE32(p);
  p += 4;
  out.communications.csu_24hr_severely_errored_framing_seconds = decodeBE32(p);
  p += 4;
  out.communications.csu_24hr_unavailable_seconds = decodeBE32(p);
  p += 4;
  out.communications.csu_24hr_controlled_slip_seconds = decodeBE32(p);
  p += 4;
  out.communications.csu_24hr_path_coding_violations = decodeBE32(p);
  p += 4;
  out.communications.csu_24hr_line_errored_seconds = decodeBE32(p);
  p += 4;
  out.communications.csu_24hr_bursty_errored_seconds = decodeBE32(p);
  p += 4;
  out.communications.csu_24hr_degraded_minutes = decodeBE32(p);
  p += 4;
  p += 2 * 2; // skip 2 spare halfwords
  out.communications.lan_switch_cpu_utilization = decodeBE32(p);
  p += 4;
  out.communications.lan_switch_memory_utilization = decodeBE16(p);
  p += 2;
  p += 2; // skip 1 spare halfword
  out.communications.ifdr_chasis_temperature = decodeBE16(p);
  p += 2;
  out.communications.ifdr_fpga_temperature = decodeBE16(p);
  p += 2;
  p += 2 * 2; // skip 2 spare halfwords
  out.communications.gps_satellites = decodeBE32(p);
  p += 4;
  p += 2 * 2; // skip 2 spare halfwords
  out.communications.ipc_status = decodeBE16(p);
  p += 2;
  out.communications.commanded_channel_control = decodeBE16(p);
  p += 2;
  p += 3 * 2; // skip 3 spare halfwords
  // Sub-struct: NEXRADMessage3AME (out.ame)
  out.ame.polarization = decodeBE16(p);
  p += 2;
  out.ame.ame_internal_temperature = decodeBEFloat(p);
  p += 4;
  out.ame.ame_receiver_module_temperature = decodeBEFloat(p);
  p += 4;
  out.ame.ame_bite_cal_module_temperature = decodeBEFloat(p);
  p += 4;
  out.ame.ame_peltier_pulse_width_modulation = decodeBE16(p);
  p += 2;
  out.ame.ame_peltier_status = decodeBE16(p);
  p += 2;
  out.ame.ame_a_d_converter_status = decodeBE16(p);
  p += 2;
  out.ame.ame_state = decodeBE16(p);
  p += 2;
  out.ame.ame_pos3dot3v_ps_voltage = decodeBEFloat(p);
  p += 4;
  out.ame.ame_pos5v_ps_voltage = decodeBEFloat(p);
  p += 4;
  out.ame.ame_pos6dot5v_ps_voltage = decodeBEFloat(p);
  p += 4;
  out.ame.ame_pos15v_ps_voltage = decodeBEFloat(p);
  p += 4;
  out.ame.ame_pos48v_ps_voltage = decodeBEFloat(p);
  p += 4;
  out.ame.ame_stalo_power = decodeBEFloat(p);
  p += 4;
  out.ame.peltier_current = decodeBEFloat(p);
  p += 4;
  out.ame.adc_calibration_reference_voltage = decodeBEFloat(p);
  p += 4;
  out.ame.ame_mode = decodeBE16(p);
  p += 2;
  out.ame.ame_peltier_mode = decodeBE16(p);
  p += 2;
  out.ame.ame_peltier_inside_fan_current = decodeBEFloat(p);
  p += 4;
  out.ame.ame_peltier_outside_fan_current = decodeBEFloat(p);
  p += 4;
  out.ame.horizontal_tr_limiter_voltage = decodeBEFloat(p);
  p += 4;
  out.ame.vertical_tr_limiter_voltage = decodeBEFloat(p);
  p += 4;
  out.ame.adc_calibration_offset_voltage = decodeBEFloat(p);
  p += 4;
  out.ame.adc_calibration_gain_correction = decodeBEFloat(p);
  p += 4;
  // Sub-struct: NEXRADMessage3RCP_SPIP_PowerButtonStatus (out.power_button_status)
  out.power_button_status.rcp_status = decodeBE16(p);
  p += 2;
  std::memcpy(out.power_button_status.rcp_string, p, 16);
  p += 16;
  out.power_button_status.spip_power_buttons = decodeBE16(p);
  p += 2;
  p += 2 * 2; // skip 2 spare halfwords
  // Sub-struct: NEXRADMessage3Power (out.power)
  out.power.master_power_administrator_load = decodeBEFloat(p);
  p += 4;
  out.power.expansion_power_administrator_load = decodeBEFloat(p);
  p += 4;
  p += 22 * 2; // skip 22 spare halfwords
  // Sub-struct: NEXRADMessage3Transmitter (out.transmitter)
  out.transmitter.pos5_vdc_ps = decodeBE16(p);
  p += 2;
  out.transmitter.pos15_vdc_ps = decodeBE16(p);
  p += 2;
  out.transmitter.pos28_vdc_ps = decodeBE16(p);
  p += 2;
  out.transmitter.neg15_vdc_ps = decodeBE16(p);
  p += 2;
  out.transmitter.pos45_vdc_ps = decodeBE16(p);
  p += 2;
  out.transmitter.filament_ps_voltage = decodeBE16(p);
  p += 2;
  out.transmitter.vacuum_pump_ps_voltage = decodeBE16(p);
  p += 2;
  out.transmitter.focus_coil_ps_voltage = decodeBE16(p);
  p += 2;
  out.transmitter.filament_ps = decodeBE16(p);
  p += 2;
  out.transmitter.klystron_warmup = decodeBE16(p);
  p += 2;
  out.transmitter.transmitter_available = decodeBE16(p);
  p += 2;
  out.transmitter.wg_switch_position = decodeBE16(p);
  p += 2;
  out.transmitter.wg_pfn_transfer_interlock = decodeBE16(p);
  p += 2;
  out.transmitter.maintenance_mode = decodeBE16(p);
  p += 2;
  out.transmitter.maintenance_required = decodeBE16(p);
  p += 2;
  out.transmitter.pfn_switch_position = decodeBE16(p);
  p += 2;
  out.transmitter.modulator_overload = decodeBE16(p);
  p += 2;
  out.transmitter.modulator_inv_current = decodeBE16(p);
  p += 2;
  out.transmitter.modulator_switch_fail = decodeBE16(p);
  p += 2;
  out.transmitter.main_power_voltage = decodeBE16(p);
  p += 2;
  out.transmitter.charging_system_fail = decodeBE16(p);
  p += 2;
  out.transmitter.inverse_diode_current = decodeBE16(p);
  p += 2;
  out.transmitter.trigger_amplifier = decodeBE16(p);
  p += 2;
  out.transmitter.circulator_temperature = decodeBE16(p);
  p += 2;
  out.transmitter.spectrum_filter_pressure = decodeBE16(p);
  p += 2;
  out.transmitter.wg_arc_vswr = decodeBE16(p);
  p += 2;
  out.transmitter.cabinet_interlock = decodeBE16(p);
  p += 2;
  out.transmitter.cabinet_air_temperature = decodeBE16(p);
  p += 2;
  out.transmitter.cabinet_airflow = decodeBE16(p);
  p += 2;
  out.transmitter.klystron_current = decodeBE16(p);
  p += 2;
  out.transmitter.klystron_filament_current = decodeBE16(p);
  p += 2;
  out.transmitter.klystron_vacion_current = decodeBE16(p);
  p += 2;
  out.transmitter.klystron_air_temperature = decodeBE16(p);
  p += 2;
  out.transmitter.klystron_airflow = decodeBE16(p);
  p += 2;
  out.transmitter.modulator_switch_maintenance = decodeBE16(p);
  p += 2;
  out.transmitter.post_charge_regulator_maintenance = decodeBE16(p);
  p += 2;
  out.transmitter.wg_pressure_humidity = decodeBE16(p);
  p += 2;
  out.transmitter.transmitter_overvoltage = decodeBE16(p);
  p += 2;
  out.transmitter.transmitter_overcurrent = decodeBE16(p);
  p += 2;
  out.transmitter.focus_coil_current = decodeBE16(p);
  p += 2;
  out.transmitter.focus_coil_airflow = decodeBE16(p);
  p += 2;
  out.transmitter.oil_temperature = decodeBE16(p);
  p += 2;
  out.transmitter.prf_limit = decodeBE16(p);
  p += 2;
  out.transmitter.transmitter_oil_level = decodeBE16(p);
  p += 2;
  out.transmitter.transmitter_battery_charging = decodeBE16(p);
  p += 2;
  out.transmitter.high_voltage_hv_status = decodeBE16(p);
  p += 2;
  out.transmitter.transmitter_recycling_summary = decodeBE16(p);
  p += 2;
  out.transmitter.transmitter_inoperable = decodeBE16(p);
  p += 2;
  out.transmitter.transmitter_air_filter = decodeBE16(p);
  p += 2;
  out.transmitter.zero_test_bit_0 = decodeBE16(p);
  p += 2;
  out.transmitter.zero_test_bit_1 = decodeBE16(p);
  p += 2;
  out.transmitter.zero_test_bit_2 = decodeBE16(p);
  p += 2;
  out.transmitter.zero_test_bit_3 = decodeBE16(p);
  p += 2;
  out.transmitter.zero_test_bit_4 = decodeBE16(p);
  p += 2;
  out.transmitter.zero_test_bit_5 = decodeBE16(p);
  p += 2;
  out.transmitter.zero_test_bit_6 = decodeBE16(p);
  p += 2;
  out.transmitter.zero_test_bit_7 = decodeBE16(p);
  p += 2;
  out.transmitter.one_test_bit_0 = decodeBE16(p);
  p += 2;
  out.transmitter.one_test_bit_1 = decodeBE16(p);
  p += 2;
  out.transmitter.one_test_bit_2 = decodeBE16(p);
  p += 2;
  out.transmitter.one_test_bit_3 = decodeBE16(p);
  p += 2;
  out.transmitter.one_test_bit_4 = decodeBE16(p);
  p += 2;
  out.transmitter.one_test_bit_5 = decodeBE16(p);
  p += 2;
  out.transmitter.one_test_bit_6 = decodeBE16(p);
  p += 2;
  out.transmitter.one_test_bit_7 = decodeBE16(p);
  p += 2;
  out.transmitter.xmtr_spip_interface = decodeBE16(p);
  p += 2;
  out.transmitter.transmitter_summary_status = decodeBE16(p);
  p += 2;
  p += 2; // skip 1 spare halfword
  out.transmitter.transmitter_rf_power_sensor = decodeBEFloat(p);
  p += 4;
  out.transmitter.horizontal_xmtr_peak_power = decodeBEFloat(p);
  p += 4;
  out.transmitter.xmtr_peak_power = decodeBEFloat(p);
  p += 4;
  out.transmitter.vertical_xmtr_peak_power = decodeBEFloat(p);
  p += 4;
  out.transmitter.xmtr_rf_avg_power = decodeBEFloat(p);
  p += 4;
  p += 2 * 2; // skip 2 spare halfwords
  out.transmitter.xmtr_recycle_count = decodeBE32(p);
  p += 4;
  out.transmitter.receiver_bias_measurement = decodeBEFloat(p);
  p += 4;
  out.transmitter.transmit_imbalance = decodeBEFloat(p);
  p += 4;
  out.transmitter.xmtr_power_meter_zero = decodeBEFloat(p);
  p += 4;
  p += 4 * 2; // skip 4 spare halfwords
  // Sub-struct: NEXRADMessage3TowerUtilities (out.tower_utilities)
  out.tower_utilities.ac_unit_1_compressor_shut_off = decodeBE16(p);
  p += 2;
  out.tower_utilities.ac_unit_2_compressor_shut_off = decodeBE16(p);
  p += 2;
  out.tower_utilities.generator_maintenance_required = decodeBE16(p);
  p += 2;
  out.tower_utilities.generator_battery_voltage = decodeBE16(p);
  p += 2;
  out.tower_utilities.generator_engine = decodeBE16(p);
  p += 2;
  out.tower_utilities.generator_volt_frequency = decodeBE16(p);
  p += 2;
  out.tower_utilities.power_source = decodeBE16(p);
  p += 2;
  out.tower_utilities.transitional_power_source_tps = decodeBE16(p);
  p += 2;
  out.tower_utilities.generator_auto_run_off_switch = decodeBE16(p);
  p += 2;
  out.tower_utilities.aircraft_hazard_lighting = decodeBE16(p);
  p += 2;
  p += 11 * 2; // skip 11 spare halfwords
  // Sub-struct: NEXRADMessage3EquipmentShelter (out.equipment_shelter)
  out.equipment_shelter.equipment_shelter_fire_detection_system = decodeBE16(p);
  p += 2;
  out.equipment_shelter.equipment_shelter_fire_smoke = decodeBE16(p);
  p += 2;
  out.equipment_shelter.generator_shelter_fire_smoke = decodeBE16(p);
  p += 2;
  out.equipment_shelter.utility_voltage_frequency = decodeBE16(p);
  p += 2;
  out.equipment_shelter.site_security_alarm = decodeBE16(p);
  p += 2;
  out.equipment_shelter.security_equipment = decodeBE16(p);
  p += 2;
  out.equipment_shelter.security_system = decodeBE16(p);
  p += 2;
  out.equipment_shelter.receiver_connected_to_antenna = decodeBE16(p);
  p += 2;
  out.equipment_shelter.radome_hatch = decodeBE16(p);
  p += 2;
  out.equipment_shelter.ac_unit_1_filter_dirty = decodeBE16(p);
  p += 2;
  out.equipment_shelter.ac_unit_2_filter_dirty = decodeBE16(p);
  p += 2;
  out.equipment_shelter.equipment_shelter_temperature = decodeBEFloat(p);
  p += 4;
  out.equipment_shelter.outside_ambient_temperature = decodeBEFloat(p);
  p += 4;
  out.equipment_shelter.transmitter_leaving_air_temp = decodeBEFloat(p);
  p += 4;
  out.equipment_shelter.ac_unit_1_discharge_air_temp = decodeBEFloat(p);
  p += 4;
  out.equipment_shelter.generator_shelter_temperature = decodeBEFloat(p);
  p += 4;
  out.equipment_shelter.radome_air_temperature = decodeBEFloat(p);
  p += 4;
  out.equipment_shelter.ac_unit_2_discharge_air_temp = decodeBEFloat(p);
  p += 4;
  out.equipment_shelter.spip_pos15v_ps = decodeBEFloat(p);
  p += 4;
  out.equipment_shelter.spip_neg15v_ps = decodeBEFloat(p);
  p += 4;
  out.equipment_shelter.spip_pos28v_ps_status = decodeBE16(p);
  p += 2;
  p += 2; // skip 1 spare halfword
  out.equipment_shelter.spip_pos5v_ps = decodeBEFloat(p);
  p += 4;
  out.equipment_shelter.converted_generator_fuel_level = decodeBE16(p);
  p += 2;
  p += 16 * 2; // skip 16 spare halfwords
  // Sub-struct: NEXRADMessage3AntennaPedestal (out.antenna_pedestal)
  out.antenna_pedestal.elevation_pos_dead_limit = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.pos150v_overvoltage = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.pos150v_undervoltage = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.elevation_servo_amp_inhibit = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.elevation_servo_amp_short_circuit = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.elevation_servo_amp_overtemp = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.elevation_motor_overtemp = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.elevation_stow_pin = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.elevation_housing_5v_ps = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.elevation_negdead_limit = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.elevation_posnormal_limit = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.elevation_neg_normal_limit = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.elevation_encoder_light = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.elevation_gearbox_oil = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.elevation_handwheel = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.elevation_amp_ps = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.azimuth_servo_amp_inhibit = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.azimuth_servo_amp_short_circuit = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.azimuth_servo_amp_overtemp = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.azimuth_motor_overtemp = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.azimuth_stow_pin = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.azimuth_housing_5v_ps = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.azimuth_encoder_light = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.azimuth_gearbox_oil = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.azimuth_bull_gear_oil = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.azimuth_handwheel = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.azimuth_servo_amp_ps = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.servo = decodeBE16(p);
  p += 2;
  out.antenna_pedestal.pedestal_interlock_switch = decodeBE16(p);
  p += 2;
  p += 12 * 2; // skip 12 spare halfwords
  // Sub-struct: NEXRADMessage3RF_GeneratorReceiver (out.rf_generator_receiver)
  out.rf_generator_receiver.coho_clock = decodeBE16(p);
  p += 2;
  out.rf_generator_receiver.rf_generator_frequency_select_oscillator = decodeBE16(p);
  p += 2;
  out.rf_generator_receiver.rf_generator_rf_stalo = decodeBE16(p);
  p += 2;
  out.rf_generator_receiver.rf_generator_phase_shifted_coho = decodeBE16(p);
  p += 2;
  out.rf_generator_receiver.pos9v_receiver_ps = decodeBE16(p);
  p += 2;
  out.rf_generator_receiver.pos5v_receiver_ps = decodeBE16(p);
  p += 2;
  out.rf_generator_receiver.pos_or_neg18v_receiver_ps = decodeBE16(p);
  p += 2;
  out.rf_generator_receiver.neg9v_receiver_ps = decodeBE16(p);
  p += 2;
  out.rf_generator_receiver.pos5v_single_channel_rdaiu_ps = decodeBE16(p);
  p += 2;
  p += 2; // skip 1 spare halfword
  out.rf_generator_receiver.horizontal_short_pulse_noise = decodeBEFloat(p);
  p += 4;
  out.rf_generator_receiver.horizontal_long_pulse_noise = decodeBEFloat(p);
  p += 4;
  out.rf_generator_receiver.horizontal_noise_temperature = decodeBEFloat(p);
  p += 4;
  out.rf_generator_receiver.vertical_short_pulse_noise = decodeBEFloat(p);
  p += 4;
  out.rf_generator_receiver.vertical_long_pulse_noise = decodeBEFloat(p);
  p += 4;
  out.rf_generator_receiver.vertical_noise_temperature = decodeBEFloat(p);
  p += 4;
  // Sub-struct: NEXRADMessage3Calibration (out.calibration)
  out.calibration.horizontal_linearity = decodeBEFloat(p);
  p += 4;
  out.calibration.horizontal_dynamic_range = decodeBEFloat(p);
  p += 4;
  out.calibration.horizontal_delta_dbz0 = decodeBEFloat(p);
  p += 4;
  out.calibration.vertical_delta_dbz0 = decodeBEFloat(p);
  p += 4;
  out.calibration.kd_peak_measured = decodeBEFloat(p);
  p += 4;
  p += 2 * 2; // skip 2 spare halfwords
  out.calibration.short_pulse_horizontal_dbz0 = decodeBEFloat(p);
  p += 4;
  out.calibration.long_pulse_horizontal_dbz0 = decodeBEFloat(p);
  p += 4;
  out.calibration.velocity_processed = decodeBE16(p);
  p += 2;
  out.calibration.width_processed = decodeBE16(p);
  p += 2;
  out.calibration.velocity_rf_gen = decodeBE16(p);
  p += 2;
  out.calibration.width_rf_gen = decodeBE16(p);
  p += 2;
  out.calibration.horizontal_i0 = decodeBEFloat(p);
  p += 4;
  out.calibration.vertical_i0 = decodeBEFloat(p);
  p += 4;
  out.calibration.vertical_dynamic_range = decodeBEFloat(p);
  p += 4;
  out.calibration.short_pulse_vertical_dbz0 = decodeBEFloat(p);
  p += 4;
  out.calibration.long_pulse_vertical_dbz0 = decodeBEFloat(p);
  p += 4;
  p += 4 * 2; // skip 4 spare halfwords
  out.calibration.horizontal_power_sense = decodeBEFloat(p);
  p += 4;
  out.calibration.vertical_power_sense = decodeBEFloat(p);
  p += 4;
  out.calibration.zdr_bias = decodeBEFloat(p);
  p += 4;
  p += 6 * 2; // skip 6 spare halfwords
  out.calibration.clutter_suppression_delta = decodeBEFloat(p);
  p += 4;
  out.calibration.clutter_suppression_unfiltered_power = decodeBEFloat(p);
  p += 4;
  out.calibration.clutter_suppression_filtered_power = decodeBEFloat(p);
  p += 4;
  p += 10 * 2; // skip 10 spare halfwords
  out.calibration.vertical_linearity = decodeBEFloat(p);
  p += 4;
  p += 4 * 2; // skip 4 spare halfwords
  // Sub-struct: NEXRADMessage3FileStatus (out.file_status)
  out.file_status.state_file_read_status = decodeBE16(p);
  p += 2;
  out.file_status.state_file_write_status = decodeBE16(p);
  p += 2;
  out.file_status.bypass_map_file_read_status = decodeBE16(p);
  p += 2;
  out.file_status.bypass_map_file_write_status = decodeBE16(p);
  p += 2;
  p += 2 * 2; // skip 2 spare halfwords
  out.file_status.current_adaptation_file_read_status = decodeBE16(p);
  p += 2;
  out.file_status.current_adaptation_file_write_status = decodeBE16(p);
  p += 2;
  out.file_status.censor_zone_file_read_status = decodeBE16(p);
  p += 2;
  out.file_status.censor_zone_file_write_status = decodeBE16(p);
  p += 2;
  out.file_status.remote_vcp_file_read_status = decodeBE16(p);
  p += 2;
  out.file_status.remote_vcp_file_write_status = decodeBE16(p);
  p += 2;
  out.file_status.baseline_adaptation_file_read_status = decodeBE16(p);
  p += 2;
  out.file_status.read_status_of_prf_sets = decodeBE16(p);
  p += 2;
  out.file_status.clutter_filter_map_file_read_status = decodeBE16(p);
  p += 2;
  out.file_status.clutter_filter_map_file_write_status = decodeBE16(p);
  p += 2;
  out.file_status.general_disk_io_error = decodeBE16(p);
  p += 2;
  out.file_status.rsp_status = static_cast<uint8_t>(*p);
  p += 1;
  out.file_status.motherboard_temperature = static_cast<uint8_t>(*p);
  p += 1;
  out.file_status.cpu_1_temperature = static_cast<uint8_t>(*p);
  p += 1;
  out.file_status.cpu_2_temperature = static_cast<uint8_t>(*p);
  p += 1;
  out.file_status.cpu_1_fan_speed = decodeBE16(p);
  p += 2;
  out.file_status.cpu_2_fan_speed = decodeBE16(p);
  p += 2;
  out.file_status.rsp_fan_1_speed = decodeBE16(p);
  p += 2;
  out.file_status.rsp_fan_2_speed = decodeBE16(p);
  p += 2;
  out.file_status.rsp_fan_3_speed = decodeBE16(p);
  p += 2;
  p += 6 * 2; // skip 6 spare halfwords
  // Sub-struct: NEXRADMessage3DeviceStatus (out.device_status)
  out.device_status.spip_comm_status = decodeBE16(p);
  p += 2;
  out.device_status.hci_comm_status = decodeBE16(p);
  p += 2;
  p += 2; // skip 1 spare halfword
  out.device_status.signal_processor_command_status = decodeBE16(p);
  p += 2;
  out.device_status.ame_communication_status = decodeBE16(p);
  p += 2;
  out.device_status.rms_link_status = decodeBE16(p);
  p += 2;
  out.device_status.rpg_link_status = decodeBE16(p);
  p += 2;
  out.device_status.interpanel_link_status = decodeBE16(p);
  p += 2;
  out.device_status.performance_check_time = decodeBE32(p);
  p += 4;
  p += 9 * 2; // skip 9 spare halfwords
  out.device_status.version = decodeBE16(p);
  p += 2;

  return true;
} // NEXRADMessage3Data::read

// ----------------------------------------------------------------------
// NEXRADDecoder (Pure Data Decoder)
// ----------------------------------------------------------------------
NEXRADDecoder::NEXRADDecoder()
  : myAccumulator(std::make_unique<NEXRADAccumulator>())
{
  myAccumulator->setKeepCompletedRadialSets(false);
  myAccumulator->setSweepCallback([this](const std::vector<std::shared_ptr<DataType> >& sweepOutputs) {
      if (mySweepCallback) {
        mySweepCallback(sweepOutputs);
      }
    });
}

NEXRADDecoder::~NEXRADDecoder() = default;

void
NEXRADDecoder::setRadarName(const std::string& name)
{
  myRadarName = name;
  if (myAccumulator) {
    myAccumulator->setRadarName(name);
  }
}

void
NEXRADDecoder::setUserVCP(int vcp)
{
  myUserVCP = vcp;
  if (myAccumulator) {
    myAccumulator->setUserVCP(vcp);
  }
}

void
NEXRADDecoder::setUseVolumeTime(bool flag)
{
  myUseVolumeTime = flag;
  if (myAccumulator) {
    myAccumulator->setUseVolumeTime(flag);
  }
}

void
NEXRADDecoder::setSeparateSAILS(bool flag)
{
  mySeparateSAILS = flag;
  if (myAccumulator) {
    myAccumulator->setSeparateSAILS(flag);
  }
}

void
NEXRADDecoder::setAddLower(bool flag)
{
  myAddLower = flag;
  if (myAccumulator) {
    myAccumulator->setAddLower(flag);
  }
}

void
NEXRADDecoder::setWriteMSG2(bool flag)
{
  myWriteMSG2 = flag;
}

void
NEXRADDecoder::setWriteMSG3(bool flag)
{
  myWriteMSG3 = flag;
}

void
NEXRADDecoder::setLdmBzipped(bool flag)
{
  myLdmBzipped = flag;
}

void
NEXRADDecoder::setHeaderMissing(bool flag)
{
  myHeaderMissing = flag;
  if (myHeaderMissing) {
    myHeaderParsed = true;
    if (myAccumulator) {
      myAccumulator->setVolumeStartTime(Time::CurrentTime());
    }
  }
}

void
NEXRADDecoder::setWriteHVAN(bool flag)
{
  myWriteHVAN = flag;
  if (myAccumulator) {
    myAccumulator->setWriteHVAN(flag);
  }
}

void
NEXRADDecoder::setAlignmentResolution(float res)
{
  myAlignmentResolution = res;
  if (myAccumulator) {
    myAccumulator->setAlignmentResolution(res);
  }
}

float
NEXRADDecoder::getAlignmentResolution() const
{
  return myAlignmentResolution;
}

void
NEXRADDecoder::setKeepCompletedRadialSets(bool keep)
{
  if (myAccumulator) {
    myAccumulator->setKeepCompletedRadialSets(keep);
  }
}

void
NEXRADDecoder::setSweepCallback(SweepCallback cb)
{
  mySweepCallback = std::move(cb);
}

void
NEXRADDecoder::compactRawBuffer()
{
  if (myRawReadPos > 0) {
    if (myRawReadPos >= myRawBuffer.size()) {
      myRawBuffer.clear();
    } else {
      size_t unread = myRawBuffer.size() - myRawReadPos;
      std::memmove(myRawBuffer.data(), myRawBuffer.data() + myRawReadPos, unread);
      myRawBuffer.resize(unread);
    }
    myRawReadPos = 0;
  }
}

void
NEXRADDecoder::compactUncompressedBuffer()
{
  if (myUncompressedReadPos > 0) {
    if (myUncompressedReadPos >= myUncompressedBuffer.size()) {
      myUncompressedBuffer.clear();
    } else {
      size_t unread = myUncompressedBuffer.size() - myUncompressedReadPos;
      std::memmove(myUncompressedBuffer.data(), myUncompressedBuffer.data() + myUncompressedReadPos, unread);
      myUncompressedBuffer.resize(unread);
    }
    myUncompressedReadPos = 0;
  }
}

void
NEXRADDecoder::appendRawBytes(const char * data, size_t len)
{
  if ((len == 0) || (data == nullptr)) { return; }

  if (myRawReadPos > 0) {
    if (myRawReadPos >= myRawBuffer.size()) {
      myRawBuffer.clear();
      myRawReadPos = 0;
    } else {
      size_t remainingCap = (myRawBuffer.capacity() > myRawBuffer.size()) ?
        (myRawBuffer.capacity() - myRawBuffer.size()) :
        0;
      if ((remainingCap < len) || (remainingCap < 65536)) {
        compactRawBuffer();
      }
    }
  }

  myRawBuffer.insert(myRawBuffer.end(), data, data + len);
}

void
NEXRADDecoder::processBuffer(const std::vector<char>& data)
{
  processBuffer(data.data(), data.size());
}

void
NEXRADDecoder::processBuffer(const char * data, size_t len)
{
  if (!data || (len == 0)) {
    return;
  }

  appendRawBytes(data, len);
  parseMessageBuffer();
}

void
NEXRADDecoder::parseMessageBuffer(bool isFlushing)
{
  // 1. Process 24-byte volume header if not yet done
  if (!myHeaderParsed) {
    if (myHeaderMissing) {
      myHeaderParsed = true;
      if (myAccumulator) {
        myAccumulator->setVolumeStartTime(Time::CurrentTime());
      }
    } else {
      size_t avail     = myRawBuffer.size() - myRawReadPos;
      const char * cur = myRawBuffer.data() + myRawReadPos;
      if (avail >= NEXRADVolumeHeader::size()) {
        if (NEXRADVolumeHeader::read(cur, avail, myVolHeader)) {
          myHeaderParsed = true;
          if (myRadarName.empty() && (myVolHeader.radar_name[0] != '\0')) {
            myAccumulator->setRadarName(myVolHeader.radar_name);
          }
          myAccumulator->setVolumeStartTime(myVolHeader.getVolumeStartTime());
          myRawReadPos += NEXRADVolumeHeader::size();
        } else {
          // If it looks like a message header (or not an AR2V file header), proceed to message loop
          myHeaderParsed = true;
        }
      } else if ((avail >= 4) && ((std::strncmp(cur, "AR2V", 4) != 0) && (std::strncmp(cur, "ARCH", 4) != 0))) {
        myHeaderParsed = true;
      }
    }
  }

  // 2. Decompression state machine: process raw buffer and append to uncompressed buffer
  BZIP2DataFilter bz2;

  while (myRawReadPos < myRawBuffer.size()) {
    size_t avail     = myRawBuffer.size() - myRawReadPos;
    const char * cur = myRawBuffer.data() + myRawReadPos;

    // Check for mid-stream AR2V volume header
    if ((avail >= 4) && ((std::strncmp(cur, "AR2V", 4) == 0) || (std::strncmp(cur, "ARCH", 4) == 0))) {
      if (avail < NEXRADVolumeHeader::size()) {
        break; // Wait for full 24-byte volume header
      }
      if (NEXRADVolumeHeader::read(cur, avail, myVolHeader)) {
        parseUncompressedMessages(true);
        if (myAccumulator) {
          myAccumulator->flush();
        }
        myHeaderParsed = true;
        if (myAccumulator) {
          if (myRadarName.empty() && (myVolHeader.radar_name[0] != '\0')) {
            myAccumulator->setRadarName(myVolHeader.radar_name);
          }
          myAccumulator->setVolumeStartTime(myVolHeader.getVolumeStartTime());
        }
        myRawReadPos += NEXRADVolumeHeader::size();
        continue;
      }
    }

    // Check for 4-byte LDM chunk length prefix preceding BZh: [4-byte rec_len][BZh]
    if ((avail >= 7) && (cur[4] == 'B') && (cur[5] == 'Z') && (cur[6] == 'h')) {
      int32_t rec_len = decodeBES32(cur);
      if ((rec_len == INT32_MIN) || (std::abs(rec_len) > 5000000)) {
        fLogSevere("NEXRADDecoder: Corrupt chunk length {}", rec_len);
        break;
      }
      uint32_t chunk_len =
        (rec_len < 0) ? static_cast<uint32_t>(-static_cast<int64_t>(rec_len)) : static_cast<uint32_t>(rec_len);
      if (chunk_len > 0) {
        size_t required = chunk_len + 4;
        if (avail < required) {
          break; // Wait for complete chunk
        }
        std::vector<char> decompressed;
        try {
          if (bz2.apply(myRawBuffer, decompressed, myRawReadPos + 4, chunk_len)) {
            myUncompressedBuffer.insert(myUncompressedBuffer.end(), decompressed.begin(), decompressed.end());
          } else {
            fLogSevere("NEXRADDecoder: Bzip2 decompression failed for chunk of size {}", chunk_len);
          }
        } catch (const std::exception& e) {
          fLogSevere("NEXRADDecoder: Bzip2 decompression exception for chunk of size {}: {}", chunk_len, e.what());
        } catch (...) {
          fLogSevere("NEXRADDecoder: Unknown bzip2 decompression exception for chunk of size {}", chunk_len);
        }
        myRawReadPos += required;
        continue;
      }
    }

    // Check if chunk starts directly with BZh (un-framed chunk from rLDMStreamReader or direct BZIP2 stream)
    if ((avail >= 4) && isValidBzip2Header(cur, avail)) {
      size_t bzLen      = avail;
      bool foundNextBZh = false;
      for (size_t i = 3; i < avail; ++i) {
        if (isValidBzip2Header(cur + i, avail - i)) {
          size_t nextStreamStart = i;
          if (i >= 7) {
            int32_t next_len = decodeBES32(cur + i - 4);
            if ((next_len != INT32_MIN) && (std::abs(next_len) > 0) && (std::abs(next_len) <= 5000000)) {
              size_t chunkLen = static_cast<size_t>(std::abs(next_len));
              bool isFramed   = false;
              if (i + chunkLen == avail) {
                isFramed = true;
              } else if ((i + chunkLen < avail) &&
                isValidBzip2Header(cur + i + chunkLen, avail - (i + chunkLen)))
              {
                isFramed = true;
              } else if ((i + chunkLen + 4 < avail) &&
                isValidBzip2Header(cur + i + chunkLen + 4, avail - (i + chunkLen + 4)))
              {
                int32_t next_next_len = decodeBES32(cur + i + chunkLen);
                if ((next_next_len != INT32_MIN) && (std::abs(next_next_len) > 0) &&
                  (std::abs(next_next_len) <= 5000000) )
                {
                  isFramed = true;
                }
              }
              if (isFramed) {
                nextStreamStart = i - 4;
              }
            }
          }
          bzLen        = nextStreamStart;
          foundNextBZh = true;
          break;
        }
      }

      std::vector<char> decompressed;
      try {
        if (bz2.apply(myRawBuffer, decompressed, myRawReadPos, bzLen) && !decompressed.empty()) {
          myUncompressedBuffer.insert(myUncompressedBuffer.end(), decompressed.begin(), decompressed.end());
          myRawReadPos += bzLen;
          continue;
        } else {
          if (foundNextBZh) {
            fLogSevere("NEXRADDecoder: Unframed bzip2 decompression failed up to next header at {}", bzLen);
            myRawReadPos += bzLen;
            continue;
          }
          // Incomplete stream block, wait for more bytes
          break;
        }
      } catch (const std::exception& e) {
        if (foundNextBZh) {
          fLogSevere("NEXRADDecoder: Bzip2 decompression exception on stream: {}", e.what());
          myRawReadPos += bzLen;
          continue;
        }
        break;
      } catch (...) {
        if (foundNextBZh) {
          fLogSevere("NEXRADDecoder: Unknown bzip2 decompression exception on stream");
          myRawReadPos += bzLen;
          continue;
        }
        break;
      }
    }

    // Skip zero-padding words (when 4-byte decode is 0)
    if ((avail >= 4) && (decodeBES32(cur) == 0)) {
      size_t nulls = 0;
      while (nulls < avail && cur[nulls] == 0) {
        nulls++;
      }
      myRawReadPos += nulls;
      continue;
    }

    // Otherwise raw uncompressed bytes directly (e.g. synthetic test or legacy stream)
    myUncompressedBuffer.insert(myUncompressedBuffer.end(), cur, cur + avail);
    myRawReadPos += avail;
    break;
  }
  compactRawBuffer();

  // 3. Loop through messages in uncompressed buffer
  parseUncompressedMessages(isFlushing);
} // NEXRADDecoder::parseMessageBuffer

void
NEXRADDecoder::parseUncompressedMessages(bool isFlushing)
{
  while ((myUncompressedBuffer.size() - myUncompressedReadPos) >= 28) {
    auto advanceToNextSync = [&]() {
        const size_t bufLen    = myUncompressedBuffer.size();
        const char * dataPtr   = myUncompressedBuffer.data();
        const size_t scanStart = myUncompressedReadPos + 16;

        if (scanStart >= bufLen) {
          myUncompressedReadPos = bufLen;
          return;
        }

        const char * cur = dataPtr + scanStart;
        const char * end = dataPtr + bufLen;
        bool found       = false;

        while (cur < end) {
          size_t rem       = end - cur;
          const char * p31 = static_cast<const char *>(std::memchr(cur, 0x1f, rem));
          const char * p1  = static_cast<const char *>(std::memchr(cur, 0x01, rem));
          const char * p5  = static_cast<const char *>(std::memchr(cur, 0x05, rem));
          const char * p2  = static_cast<const char *>(std::memchr(cur, 0x02, rem));
          const char * p3  = static_cast<const char *>(std::memchr(cur, 0x03, rem));

          const char * pNext = nullptr;
          auto updateNext = [&](const char * p) {
              if (p && (!pNext || (p < pNext) )) {
                pNext = p;
              }
            };
          updateNext(p31);
          updateNext(p1);
          updateNext(p5);
          updateNext(p2);
          updateNext(p3);

          if (!pNext) {
            break;
          }

          size_t sigIdx    = pNext - dataPtr;
          size_t candStart = sigIdx - 15;

          uint8_t chan  = static_cast<uint8_t>(dataPtr[sigIdx - 1]);
          uint8_t mtype = static_cast<uint8_t>(dataPtr[sigIdx]);

          if (((chan == 0) || (chan == 1) || (chan == 2) || (chan == 8)) &&
            ((mtype == 31) || (mtype == 1) || (mtype == 5) || (mtype == 2) || (mtype == 3)))
          {
            if (candStart + 28 <= bufLen) {
              int16_t num_segments = decodeBES16(dataPtr + candStart + 24);
              int16_t segment_no   = decodeBES16(dataPtr + candStart + 26);
              if ((num_segments >= 1) && (segment_no >= 1) && (segment_no <= num_segments)) {
                NEXRADMessageHeader testHdr;
                if (NEXRADMessageHeader::read(dataPtr + candStart, bufLen - candStart, testHdr)) {
                  size_t tBytes = testHdr.getTotalBytes();
                  if ((tBytes >= 28) && (tBytes <= 2000000) &&
                    ((testHdr.message_type != 1) || ((testHdr.msg_size >= 1200) && (testHdr.msg_size <= 1220) )) &&
                    ((testHdr.message_type == 31) || (testHdr.message_type == 1) || (testHdr.message_type == 5) ||
                    (testHdr.message_type == 2) || (testHdr.message_type == 3)) &&
                    (testHdr.num_segments >= 1) && (testHdr.segment_no >= 1) &&
                    (testHdr.segment_no <= testHdr.num_segments) &&
                    ((testHdr.channel_id == 0) || (testHdr.channel_id == 1) ||
                    (testHdr.channel_id == 2) || (testHdr.channel_id == 8)))
                  {
                    myUncompressedReadPos = candStart;
                    found = true;
                    break;
                  }
                }
              }
            } else {
              if ((candStart + 26 <= bufLen) && (decodeBES16(dataPtr + candStart + 24) < 1)) {
                // Invalid num_segments
              } else {
                myUncompressedReadPos = candStart;
                found = true;
                break;
              }
            }
          }
          cur = pNext + 1;
        }

        if (!found) {
          if (bufLen - myUncompressedReadPos > 14) {
            myUncompressedReadPos = bufLen - 14;
          } else {
            myUncompressedReadPos = bufLen;
          }
        }
      };

    NEXRADMessageHeader msgHeader;
    if (!NEXRADMessageHeader::read(myUncompressedBuffer.data() + myUncompressedReadPos,
      myUncompressedBuffer.size() - myUncompressedReadPos, msgHeader))
    {
      advanceToNextSync();
      continue;
    }

    if ((msgHeader.channel_id != 0) && (msgHeader.channel_id != 1) &&
      (msgHeader.channel_id != 2) && (msgHeader.channel_id != 8))
    {
      advanceToNextSync();
      continue;
    }

    if (msgHeader.num_segments < 1) {
      advanceToNextSync();
      continue;
    }

    if ((msgHeader.message_type == 0) || (msgHeader.message_type > 31)) {
      advanceToNextSync();
      continue;
    }

    size_t totalBytes = msgHeader.getTotalBytes();
    if ((totalBytes < 28) || (totalBytes > 2000000)) {
      advanceToNextSync();
      continue;
    }

    if ((myUncompressedBuffer.size() - myUncompressedReadPos) < totalBytes) {
      if (isFlushing &&
        ((msgHeader.message_type == 2) || (msgHeader.message_type == 3) || (msgHeader.message_type == 5)))
      {
        size_t minBytes = 12 + ((msgHeader.msg_size > 0) ? (static_cast<size_t>(msgHeader.msg_size) * 2) : 28);
        if ((myUncompressedBuffer.size() - myUncompressedReadPos) >= minBytes) {
          totalBytes = myUncompressedBuffer.size() - myUncompressedReadPos;
        } else {
          break;
        }
      } else {
        break; // Partial message
      }
    }

    bool handled = false;
    if (msgHeader.message_type == 31) {
      NEXRADMessage31Radial radial;
      if (NEXRADMessage31Radial::read(myUncompressedBuffer.data() + myUncompressedReadPos, totalBytes, radial)) {
        if (myAccumulator) {
          myAccumulator->processRadial(radial);
        }
        handled = true;
      }
    } else if (msgHeader.message_type == 1) {
      NEXRADMessage1Radial msg1;
      if (NEXRADMessage1Radial::read(myUncompressedBuffer.data() + myUncompressedReadPos, totalBytes, msg1)) {
        if (myAccumulator) {
          NEXRADMessage31Radial radial = msg1.toMessage31Radial(myRadarName);
          myAccumulator->processRadial(radial);
        }
        handled = true;
      }
    } else if (msgHeader.message_type == 5) {
      NEXRADMessage5VCP vcp;
      if (NEXRADMessage5VCP::read(myUncompressedBuffer.data() + myUncompressedReadPos, totalBytes, vcp)) {
        if (myAccumulator) {
          myAccumulator->setVCP(vcp);
        }
        handled = true;
      }
    } else if (msgHeader.message_type == 2) {
      if (myWriteMSG2) {
        NEXRADMessage2RDAStatus msg2;
        if (NEXRADMessage2RDAStatus::read(myUncompressedBuffer.data() + myUncompressedReadPos, totalBytes, msg2)) {
          if (myAccumulator) {
            myAccumulator->processMessage2(msg2);
          }
        }
      }
      handled = true;
    } else if (msgHeader.message_type == 3) {
      if (myWriteMSG3) {
        NEXRADMessage3Data msg3;
        if (NEXRADMessage3Data::read(myUncompressedBuffer.data() + myUncompressedReadPos, totalBytes, msg3)) {
          if (myAccumulator) {
            myAccumulator->processMessage3(msg3);
          }
        }
      }
      handled = true;
    } else {
      handled = true;
    }

    if (handled) {
      myUncompressedReadPos += totalBytes;
    } else {
      advanceToNextSync();
    }
  }

  compactUncompressedBuffer();
} // NEXRADDecoder::parseUncompressedMessages

void
NEXRADDecoder::flush()
{
  parseMessageBuffer(true);
  if (myAccumulator) {
    myAccumulator->flush();
  }
}

void
NEXRADDecoder::resetStreamState()
{
  parseMessageBuffer(true);
  myRawBuffer.clear();
  myRawReadPos = 0;
  myUncompressedBuffer.clear();
  myUncompressedReadPos = 0;
  myHeaderParsed        = myHeaderMissing;
}

void
NEXRADDecoder::reset()
{
  myRawBuffer.clear();
  myRawReadPos = 0;
  myUncompressedBuffer.clear();
  myUncompressedReadPos = 0;
  myHeaderParsed        = myHeaderMissing;
  if (myAccumulator) {
    myAccumulator->reset();
    if (!myRadarName.empty()) {
      myAccumulator->setRadarName(myRadarName);
    }
    if (myHeaderMissing) {
      myAccumulator->setVolumeStartTime(Time::CurrentTime());
    }
    myAccumulator->setUserVCP(myUserVCP);
    myAccumulator->setUseVolumeTime(myUseVolumeTime);
    myAccumulator->setSeparateSAILS(mySeparateSAILS);
    bool effectiveAddLower = (myUserVCP > 0) ? myAddLower : true;
    myAccumulator->setAddLower(effectiveAddLower);
    myAccumulator->setWriteHVAN(myWriteHVAN);
    myAccumulator->setAlignmentResolution(myAlignmentResolution);
    myAccumulator->setKeepCompletedRadialSets(false);
  }
}

NEXRADAccumulator&
NEXRADDecoder::getAccumulator()
{
  return *myAccumulator;
}

const NEXRADAccumulator&
NEXRADDecoder::getAccumulator() const
{
  return *myAccumulator;
}

const std::vector<std::shared_ptr<RadialSet> >&
NEXRADDecoder::getRadialSets() const
{
  return myAccumulator->getRadialSets();
}

bool
NEXRADDecoder::writeSweepToNetcdf(
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
    std::string sovStr;
    if ((firstRs->getString("StartOfVolume", sovStr) && (sovStr == "true")) ||
      (firstRs->getString("volElevIndex", volIdxStr) && ((volIdxStr == "1") || (volIdxStr == "0") )))
    {
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
    if (elevStr.empty()) {
      dt->getString("SubType", elevStr);
    }
    std::string dateStr;
    if (!dt->getString(Constants::FilenameDateTime, dateStr) || dateStr.empty()) {
      dateStr = dt->getTime().getString("%Y%m%d-%H%M%S");
    }

    std::string factory = "netcdf";
    std::string ext     = ".netcdf";
    if (std::dynamic_pointer_cast<DataTable>(dt)) {
      factory = "text";
      ext     = ".txt";
    }

    fs::path targetPath;
    if (flatDir) {
      std::string fname;
      if (elevStr.empty()) {
        fname = typeName + "_" + dateStr + ext;
      } else {
        fname = typeName + "_" + elevStr + "_" + dateStr + ext;
      }
      targetPath = fs::path(outputDir) / fname;
    } else {
      std::string fname = dateStr + ext;
      if (elevStr.empty()) {
        targetPath = fs::path(outputDir) / typeName / fname;
      } else {
        targetPath = fs::path(outputDir) / typeName / elevStr / fname;
      }
    }

    try {
      fs::create_directories(targetPath.parent_path());
    } catch (const std::exception& e) {
      fLogSevere("NEXRADDecoder: Failed to create directories for {}: {}", targetPath.string(), e.what());
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
    bool ok = IODataType::write(dt, targetPath.string(), records, factory, cfg);
    if (!ok) {
      fLogSevere("NEXRADDecoder: Failed writing {} to {}", typeName, targetPath.string());
      allSuccess = false;
    } else {
      fLogInfo("NEXRADDecoder: Successfully wrote sweep NetCDF {}", targetPath.string());
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

        if (typeName == "Velocity") {
          notifyAs(rec, "SRVel");
          notifyAs(rec, "StormRelVel");
        }
      }
    }
  }

  return allSuccess;
} // NEXRADDecoder::writeSweepToNetcdf
} // namespace rapio
