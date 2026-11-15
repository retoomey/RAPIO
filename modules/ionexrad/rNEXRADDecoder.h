/**
 * @file rNEXRADDecoder.h
 * @brief Low-level binary decoding primitives and structures for NEXRAD Level II (.ar2v).
 *
 * Implements ICD 2620002 / 2620010 compliant binary structures for:
 * - 24-byte Level II Volume Header
 * - 16-byte Message Header (CTM + Message header)
 * - Message 31 (Digital Radar Data Generic Format: VOL, ELV, RAD, and Moments)
 * - Message 5 (Volume Coverage Pattern / VCP)
 * - Endian-safe byte swapping and bitmask processing for SAILS / MRLE / MPDA.
 */

#pragma once

#include <rTime.h>
#include <rConstants.h>
#include <rLLH.h>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <cstring>
#include <cstdint>
#include <functional>

#include <endian.h>

namespace rapio {
/**
 * @enum NEXRADBitMask
 * @brief Bitmasks for VCP / SAILS processing (ICD 2620002 Table XI).
 */
enum NEXRADBitMask : uint16_t {
  HW09_TRUNCATED_VCP   = 1 << 14, ///< Truncated VCP flag in halfword 9
  HW10_SAILS_VCP       = 1 << 0,  ///< SAILS active in VCP halfword 10
  HW10_MRLE_VCP        = 1 << 4,  ///< MRLE active in VCP halfword 10
  HW10_MPDA_CUTS_ADDED = 1 << 10, ///< MPDA cuts added flag
  HW10_MPDA_VCP        = 1 << 11, ///< MPDA active in VCP
  HWE15_SAILS_CUT      = 1 << 0,  ///< Individual elevation cut is SAILS cut
  HWE15_MRLE_CUT       = 1 << 4,  ///< Individual elevation cut is MRLE cut
  HWE15_MPDA_CUT       = 1 << 9,  ///< Individual elevation cut is MPDA cut
  HWE15_BASE_TILT_CUT  = 1 << 10  ///< Individual elevation cut is base tilt cut
};

/**
 * @brief Checks if a specific bit flag or bitmask is set.
 * @param value The 16-bit word value.
 * @param flagMask The mask to check.
 * @return true if the mask bits are all set, false otherwise.
 */
inline bool
isBitSet(uint16_t value, uint16_t flagMask)
{
  return (value & flagMask) == flagMask;
}

#ifndef RAPIO_ENDIAN_DECODE_HELPERS
# define RAPIO_ENDIAN_DECODE_HELPERS
// Endian-safe decoding helpers
/** @brief Decodes big-endian unsigned 16-bit integer. */
inline uint16_t
decodeBE16(const void * ptr)
{
  uint16_t val;

  std::memcpy(&val, ptr, sizeof(val));
  return be16toh(val);
}

/** @brief Decodes big-endian signed 16-bit integer. */
inline int16_t
decodeBES16(const void * ptr)
{
  return static_cast<int16_t>(decodeBE16(ptr));
}

/** @brief Decodes big-endian unsigned 32-bit integer. */
inline uint32_t
decodeBE32(const void * ptr)
{
  uint32_t val;

  std::memcpy(&val, ptr, sizeof(val));
  return be32toh(val);
}

/** @brief Decodes big-endian signed 32-bit integer. */
inline int32_t
decodeBES32(const void * ptr)
{
  return static_cast<int32_t>(decodeBE32(ptr));
}

/** @brief Decodes big-endian IEEE 754 32-bit single-precision float. */
inline float
decodeBEFloat(const void * ptr)
{
  uint32_t u = decodeBE32(ptr);
  float f;

  std::memcpy(&f, &u, sizeof(f));
  return f;
}

#endif // RAPIO_ENDIAN_DECODE_HELPERS

/**
 * @brief Converts NEXRAD Julian days (since 1970-01-01) and milliseconds past midnight to rapio::Time.
 * @param days Julian days since 1970-01-01 (1 = 1970-01-01).
 * @param msecs Milliseconds past midnight UTC.
 * @return Corresponding rapio::Time timestamp.
 */
inline Time
nexradTimeToRapioTime(uint16_t days, int32_t msecs)
{
  // days: 1 corresponds to 1970-01-01
  int64_t epoch_seconds = (static_cast<int64_t>(days) - 1) * 86400LL + (msecs / 1000);
  double fractional     = (msecs % 1000) / 1000.0;

  return Time(static_cast<time_t>(epoch_seconds), fractional);
}

/**
 * @struct NEXRADVolumeHeader
 * @brief 24-byte Level II Volume Header (ICD 2620010 Section 7.3.2).
 */
struct NEXRADVolumeHeader {
  char    archive[9];    ///< Tape filename / signature ("AR2V0001." or "ARCHIVE2.")
  char    extension[3];  ///< Extension / volume number e.g. "001"
  int32_t create_days;   ///< Julian days since 1/1/1970
  int32_t create_msecs;  ///< Milliseconds past midnight UTC
  char    radar_name[5]; ///< 4-character ICAO radar identifier (e.g. "KTLX") + null

  /** @brief Validates magic archive signature. */
  bool
  isValid() const;

  /** @brief Gets the volume creation/start time. */
  Time
  getVolumeStartTime() const;

  /** @brief Header size in bytes (24). */
  static size_t size(){ return 24; }

  /**
   * @brief Deserializes a volume header from buffer.
   * @param buf Pointer to at least 24 bytes of data.
   * @param buflen Available length in buffer.
   * @param out Deserialized NEXRADVolumeHeader structure.
   * @return true if valid volume header, false otherwise.
   */
  static bool
  read(const char * buf, size_t buflen, NEXRADVolumeHeader& out);
};

/**
 * @struct NEXRADMessageHeader
 * @brief 16-byte NEXRAD Message Header (preceded in Level II stream by 12 bytes of CTM header).
 */
struct NEXRADMessageHeader {
  uint16_t msg_size;       ///< Length in halfwords (16-bit words) from this point to end of record
  uint8_t  channel_id;     ///< Channel number
  uint8_t  message_type;   ///< 1=legacy data, 2=RDA status, 3=perf, 5=VCP, 31=digital radar data
  uint16_t seq_id;         ///< Message sequence ID
  uint16_t gen_time_days;  ///< Generation date (Julian days since 1970-01-01)
  int32_t  gen_time_msecs; ///< Generation time (ms past midnight UTC)
  int16_t  num_segments;   ///< Total segments
  int16_t  segment_no;     ///< Segment number

  /** @brief Calculates total byte size of this message (including 12-byte CTM header). */
  size_t
  getTotalBytes() const;

  /**
   * @brief Deserializes a message header from buffer starting at the 12-byte CTM header.
   * @param buf Input buffer.
   * @param buflen Buffer length.
   * @param out Deserialized NEXRADMessageHeader.
   * @return true if successfully read, false otherwise.
   */
  static bool
  read(const char * buf, size_t buflen, NEXRADMessageHeader& out);
};

/**
 * @struct NEXRADMessage31VolumeBlock
 * @brief Message 31 Volume Data Block ("VOL").
 */
struct NEXRADMessage31VolumeBlock {
  uint16_t data_block_size = 0;                 ///< Block size in bytes
  uint8_t  major_version   = 0;                 ///< Major ICD version
  uint8_t  minor_version   = 0;                 ///< Minor ICD version
  float    site_lat        = 0.0f;              ///< Latitude in degrees (-90 to +90)
  float    site_lon        = 0.0f;              ///< Longitude in degrees (-180 to +180)
  int16_t  site_ht         = 0;                 ///< Antenna height above sea level in meters
  int16_t  feedhorn_ht_above_ground  = 0;       ///< Feedhorn height above ground in meters
  float    dbz0_calibration_constant = 0.0f;    ///< System calibration constant dBZ0
  float    hor_shv_tx_power = 0.0f;             ///< Horizontal transmit power (kW)
  float    ver_shv_tx_power = 0.0f;             ///< Vertical transmit power (kW)
  float    zdr_calibration  = 0.0f;             ///< System differential reflectivity calibration (dB)
  float    phidp_initial    = 0.0f;             ///< Initial system differential phase (deg)
  int16_t  vcp = 0;                             ///< Volume Coverage Pattern number
  int16_t  processing_status = 0;               ///< Processing status flags
  int16_t  zdr_bias_estimate_weighted_mean = 0; ///< ZDR bias estimate
};

/**
 * @struct NEXRADMessage31ElevationBlock
 * @brief Message 31 Elevation Data Block ("ELV").
 */
struct NEXRADMessage31ElevationBlock {
  int16_t  data_block_size = 0;              ///< Block size in bytes
  int16_t  atmos_attenuation_factor  = 0;    ///< Atmospheric attenuation factor (dB/km * 1000)
  float    dbz0_calibration_constant = 0.0f; ///< System calibration constant dBZ0
  uint8_t  waveform_type = 0;                ///< Waveform type (1 = Contiguous Surveillance)
  uint16_t sector1_spare = 0;                ///< Sector 1 spare (SAILS / MRLE / MPDA cut flags)
};

/**
 * @struct NEXRADMessage31RadialBlock
 * @brief Message 31 Radial Data Block ("RAD").
 */
struct NEXRADMessage31RadialBlock {
  int16_t data_block_size = 0;    ///< Block size in bytes
  int16_t unamb_range     = 0;    ///< Unambiguous range in units of 0.1 km
  float   hor_noise_level = 0.0f; ///< Horizontal noise level (dBm)
  float   ver_noise_level = 0.0f; ///< Vertical noise level (dBm)
  int16_t nyquist_vel     = 0;    ///< Nyquist velocity in units of 0.01 m/s
  float   cal_const_hor   = 0.0f; ///< Horizontal calibration constant
  float   cal_const_ver   = 0.0f; ///< Vertical calibration constant

  /** @brief Returns Nyquist velocity in meters/second. */
  float
  getNyquistVelocity() const { return nyquist_vel / 100.0f; }

  /** @brief Returns unambiguous range in kilometers. */
  float
  getUnambiguousRangeKm() const { return unamb_range / 10.0f; }
};

/**
 * @struct NEXRADMessage31MomentBlock
 * @brief Message 31 Moment Data Block ("REF", "VEL", "SW", "ZDR", "PHI", "RHO", "SNR", "SND").
 */
struct NEXRADMessage31MomentBlock {
  std::string        data_name;     ///< 3-character moment identifier (e.g. "REF", "VEL")
  uint32_t           reserved  = 0; ///< Reserved field
  uint16_t           num_gates = 0; ///< Number of range gates along this radial
  int16_t            dist_to_center_of_first_gate = 0;///< Distance to center of first gate in meters
  int16_t            range_sample_interval        = 0; ///< Gate spacing / sample interval in meters
  int16_t            threshold_for_overlayed      = 0; ///< SNR threshold for overlayed data
  int16_t            snr_threshold  = 0;               ///< SNR threshold
  uint8_t            ctrl_flags     = 0;               ///< Control flags
  uint8_t            data_word_size = 0;               ///< Data word size (8 or 16 bits)
  float              scale  = 1.0f;                    ///< Scale factor: val = (raw - offset) / scale
  float              offset = 0.0f;                    ///< Offset: val = (raw - offset) / scale
  std::vector<float> data;                             ///< Decoded engineering units (e.g. dBZ, m/s)

  /** @brief Returns range gate spacing in meters. */
  float
  getGateWidth() const { return static_cast<float>(range_sample_interval); }

  /** @brief Returns range to the center of the first gate in meters. */
  float
  getRangeToFirstGate() const { return static_cast<float>(dist_to_center_of_first_gate); }
};

/**
 * @struct NEXRADMessage31Radial
 * @brief Complete decoded Message 31 Radial Packet containing radial metadata, blocks, and moments.
 */
struct NEXRADMessage31Radial {
  char                                              radar[5];                           ///< 4-character radar identifier + null
  int32_t                                           radial_time_msecs = 0;              ///< Collection time (ms past midnight UTC)
  uint16_t                                          radial_time_days  = 0;              ///< Collection date (Julian days since 1970-01-01)
  uint16_t                                          az_no                       = 0;    ///< Radial sequence number within sweep (1..N)
  float                                             azimuth                     = 0.0f; ///< Azimuth angle in degrees (0..360)
  uint8_t                                           compress_flag               = 0;    ///< Compression flag (0=uncompressed, 1=BZIP2)
  uint8_t                                           spare                       = 0;    ///< Spare
  int16_t                                           uncompressed_length         = 0;    ///< Uncompressed radial size in bytes
  uint8_t                                           az_spacing                  = 0;    ///< Azimuth spacing (1 = 0.5 deg, 2 = 1.0 deg)
  uint8_t                                           radial_status               = 0;    ///< Radial status (0=start elev, 1=interm, 2=end elev, 3=start vol, 4=end vol)
  uint8_t                                           elevation_no                = 0;    ///< Elevation cut number within volume (1..25)
  uint8_t                                           sector_no                   = 0;    ///< RDA cut sector number
  float                                             elevation                   = 0.0f; ///< Target elevation angle in degrees
  uint8_t                                           radial_spot_blanking_status = 0;    ///< Spot blanking status
  uint8_t                                           azimuth_indexing_mode       = 0;    ///< Azimuth indexing mode (0=none, 1=0.5 deg, 2=1.0 deg)
  int16_t                                           num_data_blocks             = 0;    ///< Number of data blocks in this radial

  NEXRADMessage31VolumeBlock                        vol_block;    ///< Parsed VOL data block
  NEXRADMessage31ElevationBlock                     elev_block;   ///< Parsed ELV data block
  NEXRADMessage31RadialBlock                        radial_block; ///< Parsed RAD data block
  std::map<std::string, NEXRADMessage31MomentBlock> moments;      ///< Map of moment name to decoded moment block

  bool                                              has_vol_block  = false; ///< Indicates if VOL block was present
  bool                                              has_elev_block = false; ///< Indicates if ELV block was present
  bool                                              has_rad_block  = false; ///< Indicates if RAD block was present

  /** @brief Returns true if this radial marks the beginning of an elevation cut. */
  bool
  startOfElev() const { return radial_status == 0 || radial_status == 5; }

  /** @brief Returns true if this radial marks the end of an elevation cut. */
  bool
  endOfElev() const { return radial_status == 2; }

  /** @brief Returns true if this radial marks the beginning of a volume scan. */
  bool
  startOfVolume() const { return radial_status == 3; }

  /** @brief Returns true if this radial marks the end of a volume scan. */
  bool
  endOfVolume() const { return radial_status == 4; }

  /** @brief Returns azimuth spacing in degrees (0.5 or 1.0). */
  float
  getAzimuthSpacing() const { return (az_spacing == 1) ? 0.5f : 1.0f; }

  /** @brief Returns timestamp of this radial. */
  Time
  getTime() const { return nexradTimeToRapioTime(radial_time_days, radial_time_msecs); }

  /**
   * @brief Deserializes a Message 31 radial from buffer.
   * @param buf Pointer to message bytes starting at the Message Header.
   * @param buflen Available bytes in buffer.
   * @param out Deserialized NEXRADMessage31Radial structure.
   * @return true if successfully decoded, false otherwise.
   */
  static bool
  read(const char * buf, size_t buflen, NEXRADMessage31Radial& out);
};

/**
 * @struct NEXRADMessage1Radial
 * @brief Legacy Message 1 Radial Packet (2432-byte legacy ICD format).
 */
struct NEXRADMessage1Radial {
  int32_t            radial_time_msecs = 0;       ///< Collection time (ms past midnight UTC)
  uint16_t           radial_time_days  = 0;       ///< Collection date (Julian days since 1970-01-01)
  int16_t            unamb_range       = 0;       ///< Unambiguous range in units of 0.1 km
  uint16_t           azimuth       = 0;           ///< Azimuth code
  uint16_t           azimuth_no    = 0;           ///< Radial sequence number within sweep
  uint16_t           radial_status = 0;           ///< Radial status (0=start elev, 1=interm, 2=end elev, 3=start vol, 4=end vol)
  uint16_t           elevation     = 0;           ///< Elevation code
  int16_t            elevation_no  = 0;           ///< Sweep cut number
  int16_t            range_to_first_gate_ref = 0; ///< Range to first gate REF in meters
  int16_t            range_to_first_gate_vel = 0; ///< Range to first gate VEL/SPW in meters
  int16_t            gate_width_ref = 0;          ///< Gate width REF in meters
  int16_t            gate_width_vel = 0;          ///< Gate width VEL/SPW in meters
  uint16_t           num_bins_ref   = 0;          ///< Number of gates for REF
  uint16_t           num_bins_vel   = 0;          ///< Number of gates for VEL/SPW
  uint16_t           sector_no      = 0;          ///< Sector number
  float              calib_const    = 0.0f;       ///< Calibration constant
  uint16_t           ref_offset     = 0;          ///< Offset to REF data
  uint16_t           vel_offset     = 0;          ///< Offset to VEL data
  uint16_t           spw_offset     = 0;          ///< Offset to SPW data
  uint16_t           vel_resolution = 2;          ///< 2 = 0.5 m/s, 4 = 1.0 m/s
  int16_t            vcp         = 0;             ///< VCP pattern number
  int16_t            nyquist_vel = 0;             ///< Nyquist velocity in units of 0.01 m/s

  std::vector<float> ref_data;
  std::vector<float> vel_data;
  std::vector<float> spw_data;

  /** @brief Returns azimuth angle in degrees (0..360). */
  float
  getAzimuth() const
  {
    float az = (azimuth / 8.0f) * (180.0f / 4096.0f);

    if (az < 0.0f) { az += 360.0f; }
    if (az >= 360.0f) { az -= 360.0f; }
    return az;
  }

  /** @brief Returns elevation angle in degrees. */
  float
  getElevation() const
  {
    int16_t el_signed = static_cast<int16_t>(elevation);

    return (el_signed / 8.0f) * (180.0f / 4096.0f);
  }

  /** @brief Returns true if this radial marks the beginning of an elevation cut. */
  bool
  startOfElev() const { return radial_status == 0 || radial_status == 3; }

  /** @brief Returns true if this radial marks the end of an elevation cut. */
  bool
  endOfElev() const { return radial_status == 2; }

  /** @brief Returns true if this radial marks the beginning of a volume scan. */
  bool
  startOfVolume() const { return radial_status == 3; }

  /** @brief Returns true if this radial marks the end of a volume scan. */
  bool
  endOfVolume() const { return radial_status == 4; }

  /** @brief Returns timestamp of this radial. */
  Time
  getTime() const { return nexradTimeToRapioTime(radial_time_days, radial_time_msecs); }

  static bool
  read(const char * buf, size_t buflen, NEXRADMessage1Radial& out);

  NEXRADMessage31Radial
  toMessage31Radial(const std::string& radarName = "") const;
};

/**
 * @struct NEXRADMessage5VCP
 * @brief Message 5 Volume Coverage Pattern (VCP) definition.
 */
struct NEXRADMessage5VCP {
  int16_t  msg_size              = 0; ///< Message size in halfwords
  int16_t  pattern_type          = 0; ///< Pattern type
  int16_t  pattern_number        = 0; ///< VCP number (e.g. 12, 35, 212, 215)
  int16_t  num_elevation_cuts    = 0; ///< Number of elevation cuts in this VCP
  int16_t  clutter_map_group_num = 0; ///< Clutter map group number
  uint8_t  doppler_vel_res       = 0; ///< Doppler velocity resolution (2=0.5 m/s, 4=1.0 m/s)
  uint8_t  pulse_width           = 0; ///< Pulse width (2=short, 4=long)
  uint16_t vcp_sequencing        = 0; ///< VCP sequencing word
  uint16_t vcp_supplemental_data = 0; ///< Supplemental data (SAILS / MRLE bits)

  /**
   * @struct VCPElevation
   * @brief Elevation angle configuration within a VCP.
   */
  struct VCPElevation {
    int16_t  elevation_angle          = 0; ///< Scaled elevation angle
    uint8_t  channel_config           = 0; ///< Channel configuration
    uint8_t  waveform_type            = 0; ///< Waveform type
    uint8_t  super_res                = 0; ///< Super-resolution flags
    uint8_t  surveillance_prf_num     = 0; ///< Surveillance PRF number
    int16_t  surveillance_pulse_count = 0;///< Surveillance pulse count
    uint16_t sector1_spare            = 0; ///< Sector 1 spare (E15): SAILS / MRLE / MPDA cut flags

    /** @brief Returns elevation angle in degrees. */
    float
    getElevationDegs(int numDec = 1) const
    {
      double angle_deg = static_cast<double>(elevation_angle & 0xfff8) * (180.0 / 32768.0);

      if (angle_deg > 180.0) { angle_deg -= 360.0; }
      double prec       = (numDec == 1) ? 10.0 : std::pow(10.0, numDec);
      int angle_deg_int = (angle_deg > 0.0) ? static_cast<int>((angle_deg * prec) + 0.5) :
        static_cast<int>((angle_deg * prec) - 0.5);

      return static_cast<float>(angle_deg_int / prec);
    }

    /** @brief Returns true if this specific elevation cut is a SAILS cut. */
    bool
    isSAILSCut() const { return isBitSet(sector1_spare, HWE15_SAILS_CUT); }

    /** @brief Returns true if this specific elevation cut is an MRLE cut. */
    bool
    isMRLECut() const { return isBitSet(sector1_spare, HWE15_MRLE_CUT); }
  };
  std::vector<VCPElevation> elevations; ///< Cut definitions in this VCP

  /**
   * @brief Matches elevation cut using cut sequence number (1-based), falling back to epsilon comparison.
   * @param elevation_no Cut sequence number (1..N).
   * @param targetElevDegs Target elevation angle in degrees for fallback matching.
   * @param epsilon Tolerance for floating point comparison (default 0.05 deg).
   * @param preferSAILS Whether to prefer a cut marked as SAILS when multiple cuts match.
   * @return Cut index (0-based) or -1 if no matching elevation found.
   */
  int
  findCutIndex(uint8_t elevation_no, float targetElevDegs, float epsilon = 0.05f, bool preferSAILS = false) const
  {
    int cutIdx = static_cast<int>(elevation_no) - 1;

    if ((cutIdx >= 0) && (cutIdx < static_cast<int>(elevations.size()))) {
      return cutIdx;
    }
    return findCutIndex(targetElevDegs, epsilon, preferSAILS);
  }

  /**
   * @brief Matches target elevation against elevations in VCP using epsilon comparison.
   * @param targetElevDegs Target elevation angle in degrees.
   * @param epsilon Tolerance for floating point comparison (default 0.05 deg).
   * @param preferSAILS Whether to prefer a cut marked as SAILS when multiple cuts match.
   * @return Cut index (0-based) or -1 if no matching elevation found.
   */
  int
  findCutIndex(float targetElevDegs, float epsilon = 0.05f, bool preferSAILS = false) const
  {
    int matched = -1;

    for (size_t i = 0; i < elevations.size(); ++i) {
      if (std::abs(elevations[i].getElevationDegs() - targetElevDegs) < epsilon) {
        if (preferSAILS && elevations[i].isSAILSCut()) {
          return static_cast<int>(i);
        }
        if (matched < 0) {
          matched = static_cast<int>(i);
        }
      }
    }
    return matched;
  }

  /**
   * @brief Deserializes a Message 5 VCP from buffer.
   * @param buf Pointer to message bytes starting at Message Header.
   * @param buflen Available bytes in buffer.
   * @param out Deserialized NEXRADMessage5VCP structure.
   * @return true if successfully decoded, false otherwise.
   */
  static bool
  read(const char * buf, size_t buflen, NEXRADMessage5VCP& out);
};

/**
 * @struct NEXRADMessage2RDAStatus
 * @brief Message 2 (RDA Status) decoding structure mirroring legacy LevelII::Message2RDADataStatus.
 */
struct NEXRADMessage2RDAStatus {
  Time     time;
  int16_t  rda_status                = 0;
  int16_t  operability_status        = 0;
  int16_t  control_status            = 0;
  int16_t  aux_power_generator_state = 0;
  uint16_t avg_transmiter_power      = 0;
  int16_t  horz_reflecitivty_calibration_correction = 0;
  int16_t  data_transmission_enabled      = 0;
  short    volume_coverage_pattern_number = 0;
  int16_t  rda_control_auth        = 0;
  uint16_t rda_build_number        = 0;
  int16_t  operational_mode        = 0;
  int16_t  super_resolution_status = 0;
  int16_t  clutter_mitigation_decision_status = 0;
  int16_t  avset_ebc_log_status        = 0;
  int16_t  rda_alarm_summary           = 0;
  int16_t  command_acknowledgment      = 0;
  int16_t  channel_control_status      = 0;
  int16_t  spot_blanking_status        = 0;
  uint16_t bypass_map_gen_date         = 0;
  uint16_t bypass_map_gen_time         = 0;
  uint16_t clutter_filter_map_gen_date = 0;
  uint16_t clutter_filter_map_gen_time = 0;
  int16_t  vertical_reflectivity_calibration_correction = 0;
  uint16_t transition_power_source_status = 0;
  int16_t  rms_control_status       = 0;
  int16_t  performance_check_status = 0;
  uint16_t alarm_codes[14] = { 0 };
  int16_t  signal_processing_options = 0;
  uint16_t msg2_spares[18] = { 0 };
  uint16_t status_version  = 0;

  static bool
  read(const char * buf, size_t buflen, NEXRADMessage2RDAStatus& out);
};

struct NEXRADMessage3Communications {
  uint16_t spare = 0;
  uint16_t loop_back_test_status     = 0;
  uint32_t t1_output_frames          = 0;
  uint32_t t1_input_frames           = 0;
  uint32_t router_memory_used        = 0;
  uint32_t router_memory_free        = 0;
  uint16_t router_memory_utilization = 0;
  uint16_t route_to_rpg             = 0;
  uint32_t csu_loss_of_signal       = 0;
  uint32_t csu_loss_of_frames       = 0;
  uint32_t csu_yellow_alarms        = 0;
  uint32_t csu_blue_alarms          = 0;
  uint32_t csu_24hr_errored_seconds = 0;
  uint32_t csu_24hr_severely_errored_seconds         = 0;
  uint32_t csu_24hr_severely_errored_framing_seconds = 0;
  uint32_t csu_24hr_unavailable_seconds     = 0;
  uint32_t csu_24hr_controlled_slip_seconds = 0;
  uint32_t csu_24hr_path_coding_violations  = 0;
  uint32_t csu_24hr_line_errored_seconds    = 0;
  uint32_t csu_24hr_bursty_errored_seconds  = 0;
  uint32_t csu_24hr_degraded_minutes        = 0;
  uint32_t lan_switch_cpu_utilization       = 0;
  uint16_t lan_switch_memory_utilization    = 0;
  uint16_t ifdr_chasis_temperature = 0;
  uint16_t ifdr_fpga_temperature   = 0;
  uint32_t gps_satellites = 0;
  uint16_t ipc_status     = 0;
  uint16_t commanded_channel_control = 0;
};

struct NEXRADMessage3AME {
  uint16_t polarization = 0;
  float    ame_internal_temperature           = 0.0f;
  float    ame_receiver_module_temperature    = 0.0f;
  float    ame_bite_cal_module_temperature    = 0.0f;
  uint16_t ame_peltier_pulse_width_modulation = 0;
  uint16_t ame_peltier_status       = 0;
  uint16_t ame_a_d_converter_status = 0;
  uint16_t ame_state = 0;
  float    ame_pos3dot3v_ps_voltage = 0.0f;
  float    ame_pos5v_ps_voltage     = 0.0f;
  float    ame_pos6dot5v_ps_voltage = 0.0f;
  float    ame_pos15v_ps_voltage    = 0.0f;
  float    ame_pos48v_ps_voltage    = 0.0f;
  float    ame_stalo_power = 0.0f;
  float    peltier_current = 0.0f;
  float    adc_calibration_reference_voltage = 0.0f;
  uint16_t ame_mode         = 0;
  uint16_t ame_peltier_mode = 0;
  float    ame_peltier_inside_fan_current  = 0.0f;
  float    ame_peltier_outside_fan_current = 0.0f;
  float    horizontal_tr_limiter_voltage   = 0.0f;
  float    vertical_tr_limiter_voltage     = 0.0f;
  float    adc_calibration_offset_voltage  = 0.0f;
  float    adc_calibration_gain_correction = 0.0f;
};

struct NEXRADMessage3RCP_SPIP_PowerButtonStatus {
  uint16_t rcp_status         = 0;
  char     rcp_string[16]     = { 0 };
  uint16_t spip_power_buttons = 0;
  uint16_t spare = 0; // *2
};

struct NEXRADMessage3Power {
  float    master_power_administrator_load    = 0.0f;
  float    expansion_power_administrator_load = 0.0f;
  uint16_t spare = 0; // *22
};

struct NEXRADMessage3Transmitter {
  uint16_t pos5_vdc_ps                       = 0;
  uint16_t pos15_vdc_ps                      = 0;
  uint16_t pos28_vdc_ps                      = 0;
  uint16_t neg15_vdc_ps                      = 0;
  uint16_t pos45_vdc_ps                      = 0;
  uint16_t filament_ps_voltage               = 0;
  uint16_t vacuum_pump_ps_voltage            = 0;
  uint16_t focus_coil_ps_voltage             = 0;
  uint16_t filament_ps                       = 0;
  uint16_t klystron_warmup                   = 0;
  uint16_t transmitter_available             = 0;
  uint16_t wg_switch_position                = 0;
  uint16_t wg_pfn_transfer_interlock         = 0;
  uint16_t maintenance_mode                  = 0;
  uint16_t maintenance_required              = 0;
  uint16_t pfn_switch_position               = 0;
  uint16_t modulator_overload                = 0;
  uint16_t modulator_inv_current             = 0;
  uint16_t modulator_switch_fail             = 0;
  uint16_t main_power_voltage                = 0;
  uint16_t charging_system_fail              = 0;
  uint16_t inverse_diode_current             = 0;
  uint16_t trigger_amplifier                 = 0;
  uint16_t circulator_temperature            = 0;
  uint16_t spectrum_filter_pressure          = 0;
  uint16_t wg_arc_vswr                       = 0;
  uint16_t cabinet_interlock                 = 0;
  uint16_t cabinet_air_temperature           = 0;
  uint16_t cabinet_airflow                   = 0;
  uint16_t klystron_current                  = 0;
  uint16_t klystron_filament_current         = 0;
  uint16_t klystron_vacion_current           = 0;
  uint16_t klystron_air_temperature          = 0;
  uint16_t klystron_airflow                  = 0;
  uint16_t modulator_switch_maintenance      = 0;
  uint16_t post_charge_regulator_maintenance = 0;
  uint16_t wg_pressure_humidity              = 0;
  uint16_t transmitter_overvoltage           = 0;
  uint16_t transmitter_overcurrent           = 0;
  uint16_t focus_coil_current                = 0;
  uint16_t focus_coil_airflow                = 0;
  uint16_t oil_temperature                   = 0;
  uint16_t prf_limit = 0;
  uint16_t transmitter_oil_level         = 0;
  uint16_t transmitter_battery_charging  = 0;
  uint16_t high_voltage_hv_status        = 0;
  uint16_t transmitter_recycling_summary = 0;
  uint16_t transmitter_inoperable        = 0;
  uint16_t transmitter_air_filter        = 0;
  uint16_t zero_test_bit_0            = 0;
  uint16_t zero_test_bit_1            = 0;
  uint16_t zero_test_bit_2            = 0;
  uint16_t zero_test_bit_3            = 0;
  uint16_t zero_test_bit_4            = 0;
  uint16_t zero_test_bit_5            = 0;
  uint16_t zero_test_bit_6            = 0;
  uint16_t zero_test_bit_7            = 0;
  uint16_t one_test_bit_0             = 0;
  uint16_t one_test_bit_1             = 0;
  uint16_t one_test_bit_2             = 0;
  uint16_t one_test_bit_3             = 0;
  uint16_t one_test_bit_4             = 0;
  uint16_t one_test_bit_5             = 0;
  uint16_t one_test_bit_6             = 0;
  uint16_t one_test_bit_7             = 0;
  uint16_t xmtr_spip_interface        = 0;
  uint16_t transmitter_summary_status = 0;
  uint16_t spare = 0; // *1
  float    transmitter_rf_power_sensor = 0.0f;
  float    horizontal_xmtr_peak_power  = 0.0f;
  float    xmtr_peak_power = 0.0f;
  float    vertical_xmtr_peak_power = 0.0f;
  float    xmtr_rf_avg_power        = 0.0f;
  // spare * 2
  uint32_t xmtr_recycle_count        = 0;
  float    receiver_bias_measurement = 0.0f;
  float    transmit_imbalance        = 0.0f;
  float    xmtr_power_meter_zero     = 0.0f;
  // spare * 4
};

struct NEXRADMessage3TowerUtilities {
  uint16_t ac_unit_1_compressor_shut_off  = 0;
  uint16_t ac_unit_2_compressor_shut_off  = 0;
  uint16_t generator_maintenance_required = 0;
  uint16_t generator_battery_voltage      = 0;
  uint16_t generator_engine         = 0;
  uint16_t generator_volt_frequency = 0;
  uint16_t power_source = 0;
  uint16_t transitional_power_source_tps = 0;
  uint16_t generator_auto_run_off_switch = 0;
  uint16_t aircraft_hazard_lighting      = 0;
  uint16_t spare = 0; // *11
};

struct NEXRADMessage3EquipmentShelter {
  uint16_t equipment_shelter_fire_detection_system = 0;
  uint16_t equipment_shelter_fire_smoke = 0;
  uint16_t generator_shelter_fire_smoke = 0;
  uint16_t utility_voltage_frequency    = 0;
  uint16_t site_security_alarm = 0;
  uint16_t security_equipment  = 0;
  uint16_t security_system     = 0;
  uint16_t receiver_connected_to_antenna = 0;
  uint16_t radome_hatch = 0;
  uint16_t ac_unit_1_filter_dirty        = 0;
  uint16_t ac_unit_2_filter_dirty        = 0;
  float    equipment_shelter_temperature = 0.0f;
  float    outside_ambient_temperature   = 0.0f;
  float    transmitter_leaving_air_temp  = 0.0f;
  float    ac_unit_1_discharge_air_temp  = 0.0f;
  float    generator_shelter_temperature = 0.0f;
  float    radome_air_temperature        = 0.0f;
  float    ac_unit_2_discharge_air_temp  = 0.0f;
  float    spip_pos15v_ps        = 0.0f;
  float    spip_neg15v_ps        = 0.0f;
  uint16_t spip_pos28v_ps_status = 0;
  uint16_t spare         = 0; // *1
  float    spip_pos5v_ps = 0.0f;
  uint16_t converted_generator_fuel_level = 0;
  // spare * 16
};

struct NEXRADMessage3AntennaPedestal {
  uint16_t elevation_pos_dead_limit          = 0;
  uint16_t pos150v_overvoltage               = 0;
  uint16_t pos150v_undervoltage              = 0;
  uint16_t elevation_servo_amp_inhibit       = 0;
  uint16_t elevation_servo_amp_short_circuit = 0;
  uint16_t elevation_servo_amp_overtemp      = 0;
  uint16_t elevation_motor_overtemp          = 0;
  uint16_t elevation_stow_pin         = 0;
  uint16_t elevation_housing_5v_ps    = 0;
  uint16_t elevation_negdead_limit    = 0;
  uint16_t elevation_posnormal_limit  = 0;
  uint16_t elevation_neg_normal_limit = 0;
  uint16_t elevation_encoder_light    = 0;
  uint16_t elevation_gearbox_oil      = 0;
  uint16_t elevation_handwheel        = 0;
  uint16_t elevation_amp_ps = 0;
  uint16_t azimuth_servo_amp_inhibit       = 0;
  uint16_t azimuth_servo_amp_short_circuit = 0;
  uint16_t azimuth_servo_amp_overtemp      = 0;
  uint16_t azimuth_motor_overtemp = 0;
  uint16_t azimuth_stow_pin       = 0;
  uint16_t azimuth_housing_5v_ps  = 0;
  uint16_t azimuth_encoder_light  = 0;
  uint16_t azimuth_gearbox_oil    = 0;
  uint16_t azimuth_bull_gear_oil  = 0;
  uint16_t azimuth_handwheel      = 0;
  uint16_t azimuth_servo_amp_ps   = 0;
  uint16_t servo = 0;
  uint16_t pedestal_interlock_switch = 0;
  uint16_t spare = 0; // *12
};

struct NEXRADMessage3RF_GeneratorReceiver {
  uint16_t coho_clock = 0;
  uint16_t rf_generator_frequency_select_oscillator = 0;
  uint16_t rf_generator_rf_stalo = 0;
  uint16_t rf_generator_phase_shifted_coho = 0;
  uint16_t pos9v_receiver_ps             = 0;
  uint16_t pos5v_receiver_ps             = 0;
  uint16_t pos_or_neg18v_receiver_ps     = 0;
  uint16_t neg9v_receiver_ps             = 0;
  uint16_t pos5v_single_channel_rdaiu_ps = 0;
  uint16_t spare = 0;
  float    horizontal_short_pulse_noise = 0.0f;
  float    horizontal_long_pulse_noise  = 0.0f;
  float    horizontal_noise_temperature = 0.0f;
  float    vertical_short_pulse_noise   = 0.0f;
  float    vertical_long_pulse_noise    = 0.0f;
  float    vertical_noise_temperature   = 0.0f;
};

struct NEXRADMessage3Calibration {
  float    horizontal_linearity     = 0.0f;
  float    horizontal_dynamic_range = 0.0f;
  float    horizontal_delta_dbz0    = 0.0f;
  float    vertical_delta_dbz0      = 0.0f;
  float    kd_peak_measured         = 0.0f;
  uint16_t spare = 0; // *2
  float    short_pulse_horizontal_dbz0 = 0.0f;
  float    long_pulse_horizontal_dbz0  = 0.0f;
  uint16_t velocity_processed        = 0;
  uint16_t width_processed           = 0;
  uint16_t velocity_rf_gen           = 0;
  uint16_t width_rf_gen              = 0;
  float    horizontal_i0             = 0.0f;
  float    vertical_i0               = 0.0f;
  float    vertical_dynamic_range    = 0.0f;
  float    short_pulse_vertical_dbz0 = 0.0f;
  float    long_pulse_vertical_dbz0  = 0.0f;
  // spare * 4
  float    horizontal_power_sense = 0.0f;
  float    vertical_power_sense   = 0.0f;
  float    zdr_bias = 0.0f;
  // spare * 6
  float    clutter_suppression_delta = 0.0f;
  float    clutter_suppression_unfiltered_power = 0.0f;
  float    clutter_suppression_filtered_power   = 0.0f;
  // spare * 10
  float    vertical_linearity = 0.0f;
  // spare * 4
};

struct NEXRADMessage3FileStatus {
  uint16_t state_file_read_status       = 0;
  uint16_t state_file_write_status      = 0;
  uint16_t bypass_map_file_read_status  = 0;
  uint16_t bypass_map_file_write_status = 0;
  uint16_t spare = 0; // *2
  uint16_t current_adaptation_file_read_status  = 0;
  uint16_t current_adaptation_file_write_status = 0;
  uint16_t censor_zone_file_read_status         = 0;
  uint16_t censor_zone_file_write_status        = 0;
  uint16_t remote_vcp_file_read_status          = 0;
  uint16_t remote_vcp_file_write_status         = 0;
  uint16_t baseline_adaptation_file_read_status = 0;
  uint16_t read_status_of_prf_sets = 0;
  uint16_t clutter_filter_map_file_read_status  = 0;
  uint16_t clutter_filter_map_file_write_status = 0;
  uint16_t general_disk_io_error = 0;
  // These share a halfword
  uint8_t  rsp_status = 0;
  uint8_t  motherboard_temperature = 0;
  // These share a halfword
  uint8_t  cpu_1_temperature = 0;
  uint8_t  cpu_2_temperature = 0;
  uint16_t cpu_1_fan_speed   = 0;
  uint16_t cpu_2_fan_speed   = 0;
  uint16_t rsp_fan_1_speed   = 0;
  uint16_t rsp_fan_2_speed   = 0;
  uint16_t rsp_fan_3_speed   = 0;
  // spare * 6
};

struct NEXRADMessage3DeviceStatus {
  uint16_t spip_comm_status = 0;
  uint16_t hci_comm_status  = 0;
  uint16_t spare = 0;
  uint16_t signal_processor_command_status = 0;
  uint16_t ame_communication_status        = 0;
  uint16_t rms_link_status        = 0;
  uint16_t rpg_link_status        = 0;
  uint16_t interpanel_link_status = 0;
  uint32_t performance_check_time = 0;
  // spare * 9
  uint16_t version = 0;
};

/**
 * @struct NEXRADMessage3Data
 * @brief Message 3 (System/Comm Status) decoding structure mirroring legacy LevelII::Message3Data.
 */
struct NEXRADMessage3Data {
  Time                                     time;
  NEXRADMessage3Communications             communications;
  NEXRADMessage3AME                        ame;
  NEXRADMessage3RCP_SPIP_PowerButtonStatus power_button_status;
  NEXRADMessage3Power                      power;
  NEXRADMessage3Transmitter                transmitter;
  NEXRADMessage3TowerUtilities             tower_utilities;
  NEXRADMessage3EquipmentShelter           equipment_shelter;
  NEXRADMessage3AntennaPedestal            antenna_pedestal;
  NEXRADMessage3RF_GeneratorReceiver       rf_generator_receiver;
  NEXRADMessage3Calibration                calibration;
  NEXRADMessage3FileStatus                 file_status;
  NEXRADMessage3DeviceStatus               device_status;

  static bool
  read(const char * buf, size_t buflen, NEXRADMessage3Data& out);
};

class NEXRADAccumulator;

class DataType;
class RadialSet;

/**
 * @class NEXRADDecoder
 * @brief Pure, stateless binary data decoder for NEXRAD Level II (.ar2v).
 *
 * Implements ICD 2620002 / 2620010 message parsing (Message 31, 5, 2, 3),
 * accumulates elevation sweeps, and produces standard rapio::RadialSet objects.
 * Decoupled from transport mechanism: ingests clean payloads via processBuffer().
 */
class NEXRADDecoder {
public:
  using SweepCallback = std::function<void (const std::vector<std::shared_ptr<DataType> >&)>;

  NEXRADDecoder();
  ~NEXRADDecoder();

  NEXRADDecoder(const NEXRADDecoder&) = delete;
  NEXRADDecoder&
  operator = (const NEXRADDecoder&) = delete;
  NEXRADDecoder(NEXRADDecoder&&)    = delete;
  NEXRADDecoder&
  operator = (NEXRADDecoder&&) = delete;

  void
  setRadarName(const std::string& name);
  void
  setUserVCP(int vcp);
  void
  setUseVolumeTime(bool flag);
  void
  setSeparateSAILS(bool flag);
  void
  setAddLower(bool flag);
  void
  setWriteMSG2(bool flag);
  void
  setWriteMSG3(bool flag);
  void
  setLdmBzipped(bool flag);
  bool
  getLdmBzipped() const { return myLdmBzipped; }

  void
  setHeaderMissing(bool flag);
  bool
  getHeaderMissing() const { return myHeaderMissing; }

  void
  setWriteHVAN(bool flag);
  bool
  getWriteHVAN() const { return myWriteHVAN; }

  void
  setAlignmentResolution(float res);
  float
  getAlignmentResolution() const;
  void
  setKeepCompletedRadialSets(bool keep);

  void
  setSweepCallback(SweepCallback cb);

  /**
   * @brief Ingests clean payload bytes from rLDMStreamReader or any stream.
   * Parses ICD structures, unpacks moments, accumulates sweeps, and triggers sweep callback.
   */
  void
  processBuffer(const std::vector<char>& data);
  void
  processBuffer(const char * data, size_t len);

  void
  flush();
  void
  reset();
  void
  resetStreamState();

  NEXRADAccumulator&
  getAccumulator();
  const NEXRADAccumulator&
  getAccumulator() const;

  const std::vector<std::shared_ptr<RadialSet> >&
  getRadialSets() const;

  static bool
  writeSweepToNetcdf(
    const std::vector<std::shared_ptr<DataType> >& sweepOutputs,
    const std::string                            & outputDir,
    bool                                         flatDir             = false,
    bool                                         uncompressed        = false,
    float                                        alignmentResolution = -1.0f);

private:
  void
  parseMessageBuffer(bool isFlushing = false);
  void
  parseUncompressedMessages(bool isFlushing = false);
  void
  compactRawBuffer();
  void
  compactUncompressedBuffer();
  void
  appendRawBytes(const char * data, size_t len);

  std::string myRadarName;
  int myUserVCP               = 0;
  bool myUseVolumeTime        = false;
  bool mySeparateSAILS        = false;
  bool myAddLower             = true;
  bool myWriteMSG2            = false;
  bool myWriteMSG3            = false;
  bool myLdmBzipped           = false;
  bool myHeaderMissing        = false;
  bool myWriteHVAN            = false;
  float myAlignmentResolution = -1.0f;

  SweepCallback mySweepCallback;
  std::unique_ptr<NEXRADAccumulator> myAccumulator;

  std::vector<char> myRawBuffer;
  size_t myRawReadPos = 0;
  std::vector<char> myUncompressedBuffer;
  size_t myUncompressedReadPos = 0;
  bool myHeaderParsed = false;
  NEXRADVolumeHeader myVolHeader;
};


// Backward compatibility aliases
using AR2VBitMask                 = NEXRADBitMask;
using AR2VVolumeHeader            = NEXRADVolumeHeader;
using AR2VMessageHeader           = NEXRADMessageHeader;
using AR2VMessage31VolumeBlock    = NEXRADMessage31VolumeBlock;
using AR2VMessage31ElevationBlock = NEXRADMessage31ElevationBlock;
using AR2VMessage31RadialBlock    = NEXRADMessage31RadialBlock;
using AR2VMessage31MomentBlock    = NEXRADMessage31MomentBlock;
using AR2VMessage31Radial         = NEXRADMessage31Radial;
using AR2VMessage5VCP             = NEXRADMessage5VCP;
using AR2VMessage2RDAStatus       = NEXRADMessage2RDAStatus;
using AR2VMessage3Communications  = NEXRADMessage3Communications;
using AR2VMessage3AME             = NEXRADMessage3AME;
using AR2VMessage3RCP_SPIP_PowerButtonStatus = NEXRADMessage3RCP_SPIP_PowerButtonStatus;
using AR2VMessage3Power                = NEXRADMessage3Power;
using AR2VMessage3Transmitter          = NEXRADMessage3Transmitter;
using AR2VMessage3TowerUtilities       = NEXRADMessage3TowerUtilities;
using AR2VMessage3EquipmentShelter     = NEXRADMessage3EquipmentShelter;
using AR2VMessage3AntennaPedestal      = NEXRADMessage3AntennaPedestal;
using AR2VMessage3RF_GeneratorReceiver = NEXRADMessage3RF_GeneratorReceiver;
using AR2VMessage3Calibration          = NEXRADMessage3Calibration;
using AR2VMessage3FileStatus           = NEXRADMessage3FileStatus;
using AR2VMessage3DeviceStatus         = NEXRADMessage3DeviceStatus;
using AR2VMessage3Data = NEXRADMessage3Data;
using AR2VDecoder      = NEXRADDecoder;
inline rapio::Time ar2vTimeToRapioTime(uint16_t days, int32_t msecs){ return nexradTimeToRapioTime(days, msecs); }
} // namespace rapio
