/**
 * @file rTDWRDecoder.h
 * @brief Modern C++17 binary decoder for Terminal Doppler Weather Radar (TDWR).
 *
 * Implements bounds-checked, endian-safe binary parsing for:
 * - TDWR Message Header (msg_id, msg_len)
 * - TDWR BaseDataHeader (48 bytes / 24 16-bit words)
 * - Message 0x2b00 (Compressed Base Data: Z, SNR, VEL, dealVEL, W)
 * - Message 0x2b01 (Low PRF Base Data: Z, SNR with split-resolution geometry)
 */

#pragma once

#include <rTime.h>
#include <rConstants.h>
#include <rLLH.h>
#include <vector>
#include <string>
#include <memory>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <functional>
#include <endian.h>

namespace rapio {
/** Message type identifiers for TDWR base data */
enum TDWRMessageType : uint16_t {
  TDWR_MSG_COMPRESSED_BASEDATA = 0x2b00, ///< Compressed Base Data (Reflectivity, Velocity, Dealiased Velocity, Width, SNR)
  TDWR_MSG_LOWPRF_BASEDATA     = 0x2b01  ///< Low PRF Base Data (Reflectivity, SNR with split-resolution geometry)
};

/** Geometry and size constants */
namespace TDWRConstants {
constexpr size_t MESSAGE_HEADER_SIZE    = 4;    ///< 4 bytes (msg_id + msg_len)
constexpr size_t BASE_DATA_HEADER_SIZE  = 48;   ///< 48 bytes (24 16-bit halfwords)
constexpr size_t HEADER_TOTAL_SIZE      = 52;   ///< MessageHeader + BaseDataHeader
constexpr size_t MAX_COMPRESSED_GATES   = 600;  ///< Uniform 150m gates for 0x2b00
constexpr size_t COMPRESSED_GATE_SIZE   = 8;    ///< 8 bytes per gate in 0x2b00
constexpr size_t COMPRESSED_RADIAL_SIZE = 4852; ///< 52 + 600 * 8 bytes

constexpr size_t MAX_LOWPRF_GATES      = 1984; ///< 900 inner (150m) + 1084 outer (300m)
constexpr size_t LOWPRF_INNER_GATES    = 900;  ///< Inner 150m resolution segment
constexpr size_t LOWPRF_OUTER_GATES    = 1084; ///< Outer 300m resolution segment
constexpr size_t LOWPRF_EXPANDED_GATES = 3068; ///< 900 + 1084 * 2 (uniform 150m spacing)
constexpr size_t LOWPRF_RADIAL_SIZE    = 6004; ///< 52 + 1984 * 3 bytes

constexpr float GATE_SPACING_150M = 150.0f; ///< 150 meters
constexpr float GATE_SPACING_300M = 300.0f; ///< 300 meters
} // namespace TDWRConstants

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
 * @struct TDWRMessageHeader
 * @brief 4-byte message header prefixing TDWR network packets.
 */
struct TDWRMessageHeader {
  uint16_t msg_id  = 0; ///< 0x2b00 (Compressed) or 0x2b01 (Low PRF)
  uint16_t msg_len = 0; ///< Total message length in bytes (normally 4852, 6004, or 6144)

  bool
  isCompressedBaseData() const { return msg_id == TDWR_MSG_COMPRESSED_BASEDATA; }

  bool
  isLowPRFBaseData() const { return msg_id == TDWR_MSG_LOWPRF_BASEDATA; }
};

/**
 * @struct TDWRBaseDataHeader
 * @brief 48-byte Base Data Header (24 16-bit halfwords) matching TDWR ICD.
 */
struct TDWRBaseDataHeader {
  uint16_t volume_scan_count = 0;
  bool     EOV           = false; ///< End of Volume scan
  bool     SOV           = false; ///< Start of Volume scan
  uint16_t nullScan      = 0;
  uint16_t scan_strategy = 0; ///< 1=MONITOR, 2=HAZARDOUS, 3=CLEAR AIR, etc.
  uint16_t peak_transmitter_power = 0;
  uint16_t site_id                 = 0;
  uint16_t MD                      = 0; ///< Magnetic declination flag/field
  uint16_t null_flags              = 0;
  bool     IDLE                    = false; ///< Idle mode
  bool     SLD                     = false; ///< Start of Live Data
  bool     SPD                     = false; ///< Start of Playback Data
  uint16_t scan_number             = 0;     ///< Elevation scan number within volume
  bool     EOE                     = false; ///< End of Elevation
  bool     SOE                     = false; ///< Start of Elevation
  uint16_t CRMN                    = 0;     ///< Clutter residue map number
  uint16_t scan_id                 = 0;
  bool     OF                      = false; ///< Obscuration flagging indicator
  bool     SR                      = false; ///< Spike removal indicator
  uint16_t DA                      = 0;     ///< Velocity dealiasing scan (0=none, 1=single, 2=dual PRF1, 3=dual PRF2)
  bool     SS                      = false; ///< Sector scan
  bool     MA                      = false; ///< Microburst aloft scan
  uint16_t PR                      = 0;     ///< Precipitation scan and resolution
  bool     WS                      = false; ///< Wind shift scan
  bool     LE                      = false; ///< Low elevation scan
  bool     MB                      = false; ///< Microburst surface scan
  bool     GF                      = false; ///< Gust front scan
  bool     LP                      = false; ///< Low PRF scan
  float    current_elevation       = 0.0f;  ///< Elevation angle in degrees
  float    angular_scan_rate       = 0.0f;  ///< Angular rate in deg/s
  bool     NP                      = false; ///< New PRI flag
  bool     MP                      = false; ///< Multiple PRI flag
  uint16_t PRI                     = 0;     ///< Pulse repetition interval in microseconds
  uint16_t dwell_id                = 0;     ///< 0=noise, 1=signal, 2=sector blanking, 3=initialization
  bool     SI                      = false; ///< Solar indicator
  bool     TST                     = false; ///< Test/maintenance mode flag
  uint16_t pulses_per_dwell        = 0;     ///< Number of radar pulses in current dwell
  uint16_t final_range_sample      = 0;     ///< Number of range gates in current dwell
  uint16_t range_samples_per_dwell = 0;
  float    current_azimuth         = 0.0f; ///< Azimuth angle in degrees (magnetic referenced)
  float    total_noise_power       = 0.0f;
  uint32_t timestamp               = 0; ///< UTC seconds since Unix epoch
  Time     radial_time;                 ///< Converted rapio::Time timestamp
  uint16_t base_data_type = 0;          ///< Bit 0: unconditioned, Bit 1: edited, Bit 2: fully conditioned
  uint8_t  CH  = 0;                     ///< Channel number
  uint16_t VN  = 0;                     ///< Version number
  bool     WI  = false;                 ///< Wind field model initializer
  bool     IVS = false;                 ///< Incomplete volume scan
  bool     IES = false;                 ///< Incomplete elevation scan
  bool     VSR = false;                 ///< Volume scan restart
  bool     ESR = false;                 ///< Elevation scan restart
  uint16_t integer_azimuth        = 0;
  uint16_t load_shed_final_sample = 0;

  /**
   * @brief Calculates Nyquist velocity based on radar frequency and PRI.
   * Formula: (lambda / PRI / 4.0) * 1000000.0, where lambda = 300000000.0 / frequency / 1000000.0.
   * @param frequencyMHz Radar frequency in MHz (e.g. 5603 for OKC TDWR).
   * @return Nyquist velocity in m/s.
   */
  float
  getNyquistVelocity(int frequencyMHz) const
  {
    if ((frequencyMHz <= 0) || (PRI == 0)) {
      return 0.0f;
    }
    const double c = 300000000.0; // speed of light (m/s)
    double lambda  = c / (static_cast<double>(frequencyMHz) * 1000000.0);

    return static_cast<float>((lambda / (static_cast<double>(PRI) * 4.0)) * 1000000.0);
  }
};

/**
 * @struct TDWRCompressedGate
 * @brief Unpacked 8-byte gate data for Message 0x2b00.
 */
struct TDWRCompressedGate {
  uint8_t Z       = 0;     ///< Raw reflectivity byte
  uint8_t SNR     = 0;     ///< Raw SNR byte
  int16_t VEL     = 0;     ///< Raw velocity 16-bit word
  uint8_t W       = 0;     ///< Raw spectrum width byte
  uint8_t GCF     = 0;     ///< Ground Clutter Filter flag (bits 6-7)
  uint8_t CAF     = 0;     ///< Dealias algorithm failure flag (bits 3-5, 0=success)
  bool    CTF     = false; ///< Point target filter flag (bit 2)
  bool    CVF     = false; ///< Conditioned valid velocity flag (bit 1)
  bool    CCV     = false; ///< Conditioned valid reflectivity flag (bit 0)
  int16_t dealVEL = 0;     ///< Dealiased velocity 16-bit word
};

/**
 * @struct TDWRCompressedRadial
 * @brief Decoded Message 0x2b00 radial containing header and up to 600 gates.
 */
struct TDWRCompressedRadial {
  TDWRMessageHeader               msg_hdr;
  TDWRBaseDataHeader              hdr;
  std::vector<TDWRCompressedGate> gates;
};

/**
 * @struct TDWRLowPRFFlag
 * @brief Flag byte for Low PRF Base Data (Message 0x2b01).
 */
struct TDWRLowPRFFlag {
  bool    GCF     = false; ///< Ground Clutter Filter flag (bit 7)
  bool    CV      = false; ///< Conditioned valid flag (bit 6)
  uint8_t bd_nul2 = 0;     ///< Bits 3-5
  bool    CTF     = false; ///< Point target filter flag (bit 2)
  uint8_t bd_nul3 = 0;     ///< Bits 0-1
};

/**
 * @struct TDWRLowPRFRadial
 * @brief Decoded Message 0x2b01 radial with split-resolution gate arrays.
 */
struct TDWRLowPRFRadial {
  TDWRMessageHeader           msg_hdr;
  TDWRBaseDataHeader          hdr;
  std::vector<uint8_t>        Z;     ///< Raw reflectivity bytes (up to 1984)
  std::vector<uint8_t>        SNR;   ///< Raw SNR bytes (up to 1984)
  std::vector<TDWRLowPRFFlag> flags; ///< Gate flags (up to 1984)
};

class TDWRAccumulator;
class DataType;
class RadialSet;

/**
 * @class TDWRDecoder
 * @brief Bounds-checked binary parser and pure data decoder for TDWR records.
 * Accepts clean payload buffers, parses TDWR radials, accumulates sweeps,
 * and outputs standard rapio::RadialSet objects.
 */
class TDWRDecoder {
public:
  using SweepCallback = std::function<void (const std::vector<std::shared_ptr<DataType> >&)>;

  TDWRDecoder();
  ~TDWRDecoder();

  // Non-copyable and non-movable due to this capture in accumulator callback
  TDWRDecoder(const TDWRDecoder&) = delete;
  TDWRDecoder&
  operator = (const TDWRDecoder&) = delete;
  TDWRDecoder(TDWRDecoder&&)      = delete;
  TDWRDecoder&
  operator = (TDWRDecoder&&) = delete;

  void
  setRadarName(const std::string& name);
  void
  setUserVCP(int vcp);
  void
  setUseVolumeTime(bool flag);
  void
  setNoiseThreshold(float db);
  void
  setMagneticOffset(const std::string& offsetStr);
  void
  setFrequency(int freqMHz);
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
   * Parses TDWR 0x2b00 / 0x2b01 messages, unpacks moments, accumulates sweeps, and triggers callback.
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

  TDWRAccumulator&
  getAccumulator();
  const TDWRAccumulator&
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

  /**
   * @brief Decodes 4-byte MessageHeader from buffer.
   * @param ptr Buffer pointer.
   * @param len Available buffer length.
   * @param out Deserialized message header.
   * @return true if successful, false if buffer too small.
   */
  static bool
  decodeMessageHeader(const uint8_t * ptr, size_t len, TDWRMessageHeader& out);

  /**
   * @brief Decodes 48-byte BaseDataHeader from buffer.
   * @param ptr Buffer pointer.
   * @param len Available buffer length.
   * @param out Deserialized base data header.
   * @return true if successful, false if buffer too small.
   */
  static bool
  decodeBaseDataHeader(const uint8_t * ptr, size_t len, TDWRBaseDataHeader& out);

  /**
   * @brief Decodes a full Message 0x2b00 (Compressed Base Data) radial.
   * @param ptr Buffer pointer starting at the MessageHeader.
   * @param len Available buffer length.
   * @param out Deserialized compressed radial.
   * @return true if successful, false if corrupted or buffer truncated.
   */
  static bool
  decodeCompressedRadial(const uint8_t * ptr, size_t len, TDWRCompressedRadial& out);

  /**
   * @brief Decodes a full Message 0x2b01 (Low PRF Base Data) radial.
   * @param ptr Buffer pointer starting at the MessageHeader.
   * @param len Available buffer length.
   * @param out Deserialized Low PRF radial.
   * @return true if successful, false if corrupted or buffer truncated.
   */
  static bool
  decodeLowPRFRadial(const uint8_t * ptr, size_t len, TDWRLowPRFRadial& out);

private:
  void
  parseMessageBuffer();
  void
  compactMessageBuffer();

  std::string myRadarName;
  int myUserVCP          = 0;
  bool myUseVolumeTime   = false;
  float myNoiseThreshold = 1.0f;
  std::string myMagneticOffsetStr;
  int myFrequencyMHz = 5603;
  float myAlignmentResolution = -1.0f;

  SweepCallback mySweepCallback;
  std::unique_ptr<TDWRAccumulator> myAccumulator;

  std::vector<uint8_t> myMessageBuffer;
  size_t myMessageReadPos = 0;
};
} // namespace rapio
