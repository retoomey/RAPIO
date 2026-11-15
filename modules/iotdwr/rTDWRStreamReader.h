/**
 * @file rTDWRStreamReader.h
 * @brief Streaming reader for TDWR binary records from streams, pipes, or directories.
 *
 * Implements real-time stream ingestion and NetCDF sweep output generation for TDWR data,
 * wrapping the core rLDMStreamReader transport reader and TDWR pure data decoder.
 */

#pragma once

#include "rTDWRDecoder.h"
#include "rTDWRAccumulator.h"
#include <rLDMStreamReader.h>
#include <rIOConfig.h>
#include <rRadialSet.h>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <set>

namespace rapio {
/**
 * @struct TDWRStreamOptions
 * @brief Configuration options for TDWR streaming ingest.
 */
struct TDWRStreamOptions {
  std::string inputDir;                    ///< -i: Input LDM directory to monitor
  std::string outputDir;                   ///< -o: Output NetCDF directory
  std::string radarName;                   ///< -s: Radar identifier (e.g. TOKC, TDFW)
  std::string pattern;                     ///< -p: Filter incoming files by pattern/prefix
  bool        killFiles           = false; ///< -K: Delete LDM files after processing
  bool        readAll             = false; ///< -a: Process existing historical files on startup
  bool        onceOnly            = false; ///< -1: Process existing files once and exit (archive mode)
  bool        pipedInput          = false; ///< -P: Stream from stdin / pipe
  int         userVCP             = 0;     ///< -v: Override VCP number
  bool        useVolumeTime       = false; ///< -V: Mark elevations with volume time
  float       noiseThreshold      = 1.0f;  ///< -N: Noise threshold in dB for Low PRF
  std::string magneticOffsetStr   = "";    ///< -mag: Magnetic declination (e.g. "10W", "7E")
  int         frequencyMHz        = 5603;  ///< -freq: Radar frequency in MHz
  bool        flatDir             = false; ///< -f: Flat output directory structure
  bool        uncompressedNetcdf  = false; ///< -u: Write uncompressed NetCDF files
  float       alignmentResolution = -1.0f; ///< -A: Alignment azimuthal resolution in degrees
  int         pollIntervalMs      = 500;   ///< Directory polling interval in milliseconds
};

/**
 * @class TDWRStreamReader
 * @brief Streaming reader for TDWR records from byte streams, pipes, or directories.
 * Acts as a composition bridge between rLDMStreamReader and TDWRDecoder.
 */
class TDWRStreamReader {
public:
  using SweepCallback = TDWRDecoder::SweepCallback;

  /** @brief Default constructor. */
  TDWRStreamReader();

  /**
   * @brief Constructs stream reader with given options.
   * @param opts Stream configuration options.
   */
  explicit
  TDWRStreamReader(const TDWRStreamOptions& opts);

  /** @brief Destructor. */
  ~TDWRStreamReader();

  /**
   * @brief Sets stream configuration options.
   * @param opts Configuration options.
   */
  void
  setOptions(const TDWRStreamOptions& opts);

  /**
   * @brief Gets current stream configuration options.
   * @return Active options reference.
   */
  const TDWRStreamOptions&
  getOptions() const { return myOptions; }

  /**
   * @brief Sets callback for completed sweep delivery.
   * @param cb Callback function.
   */
  void setSweepCallback(SweepCallback cb){ mySweepCallback = cb; }

  /**
   * @brief Ingests arbitrary raw byte stream chunk.
   * @param data Buffer pointer.
   * @param len Buffer length in bytes.
   */
  void
  pushBytes(const char * data, size_t len);

  /**
   * @brief Reads streaming input from an istream.
   * @param is Input stream reference.
   * @param chunkSize Read buffer size.
   */
  void
  readStream(std::istream& is, size_t chunkSize = 65536);

  /**
   * @brief Reads streaming input from a file descriptor (e.g. STDIN_FILENO).
   * @param fd File descriptor.
   * @param chunkSize Read buffer size.
   */
  void
  readFd(int fd, size_t chunkSize = 65536);

  /**
   * @brief Reads a complete file from disk.
   * @param filename Path to file.
   * @return true if successful, false otherwise.
   */
  bool
  readFile(const std::string& filename);

  /**
   * @brief Monitors directory for incoming TDWR files.
   * @param stopRequested Optional atomic stop flag.
   * @return Number of files processed.
   */
  size_t
  listenDirectory(std::atomic<bool> * stopRequested = nullptr);

  /**
   * @brief Flushes any pending accumulated sweeps.
   */
  void
  flush();

  /**
   * @brief Clears stream buffers and accumulator state.
   */
  void
  reset();

  /**
   * @brief Writes completed sweep outputs to NetCDF directory.
   */
  static bool
  writeSweepToNetcdf(
    const std::vector<std::shared_ptr<DataType> >& sweepOutputs,
    const std::string                            & outputDir,
    bool                                         flatDir,
    bool                                         uncompressed,
    float                                        alignmentResolution = -1.0f)
  {
    return TDWRDecoder::writeSweepToNetcdf(sweepOutputs, outputDir, flatDir, uncompressed, alignmentResolution);
  }

  /**
   * @brief Access internal accumulator.
   */
  TDWRAccumulator& getAccumulator(){ return myDecoder.getAccumulator(); }

  /**
   * @brief Access internal decoder.
   */
  TDWRDecoder& getDecoder(){ return myDecoder; }

private:
  void
  handleSweepCompleted(const std::vector<std::shared_ptr<DataType> >& sweepOutputs);

  TDWRStreamOptions myOptions;
  TDWRDecoder myDecoder;
  rLDMStreamReader myStreamReader;
  SweepCallback mySweepCallback;
};
} // namespace rapio
