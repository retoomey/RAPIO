/**
 * @file rNEXRADStreamReader.h
 * @brief Streaming reader for NEXRAD Level II (.ar2v) data wrapping rLDMStreamReader and NEXRADDecoder.
 *
 * Enforces strict separation of concerns:
 * - rLDMStreamReader handles LDM chunk framing, buffering, directory monitoring, and decompression.
 * - NEXRADDecoder handles ICD message decoding, moment unpacking, and sweep accumulation.
 */

#pragma once

#include "rNEXRADDecoder.h"
#include "rNEXRADAccumulator.h"
#include <rLDMStreamReader.h>
#include <rIOConfig.h>
#include <rRadialSet.h>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <istream>
#include <set>

namespace rapio {
/**
 * @struct NEXRADStreamOptions
 * @brief Configuration options for Level II / LDM streaming.
 * Options closely map to legacy ldm2netcdf command-line switches.
 */
struct NEXRADStreamOptions {
  std::string inputDir;                    ///< -i: Input LDM directory to monitor
  std::string outputDir;                   ///< -o: Output NetCDF directory
  std::string radarName;                   ///< -s: Source radar name (e.g. KTLX, KATX)
  std::string pattern;                     ///< -p: Filter incoming files by pattern/prefix
  bool        killFiles           = false; ///< -K: Delete/kill LDM files after processing
  bool        readAll             = false; ///< -a: Process existing files on startup
  bool        onceOnly            = false; ///< -1: Process existing files once and exit (archive mode)
  bool        pipedInput          = false; ///< -P: Stream from stdin/pipe
  bool        ldmBzipped          = false; ///< -L: LDM bzipped radial packets
  bool        headerMissing       = false; ///< -H: NEXRAD 24-byte volume header is missing
  bool        writeHVAN           = false; ///< -HVAN: Output HVAzNoise information
  int         userVCP             = 0;     ///< -v: Override VCP in data (0 means agnostic)
  bool        useVolumeTime       = false; ///< -V: Mark elevations with volume time
  bool        separateSAILS       = false; ///< -e: Separate out extra SAILS cuts with _EXTRA suffix
  bool        addLower            = false; ///< -addLower: Output tilts lower than 0.5 deg without _EXTRA suffix
  bool        flatDir             = false; ///< -f: Flat output directory structure
  bool        uncompressedNetcdf  = false; ///< -u: Write uncompressed NetCDF files
  bool        writeMSG2           = false; ///< -MSG2: Output Message2 RDA Status information
  bool        writeMSG3           = false; ///< -MSG3: Output Message3 information
  float       alignmentResolution = -1.0f; ///< -A: Alignment azimuthal resolution in degrees
  int         pollIntervalMs      = 500;   ///< Polling interval in milliseconds for directory monitoring
};

/**
 * @class NEXRADStreamReader
 * @brief Handles streaming I/O of NEXRAD Level II (.ar2v) data from an LDM stream,
 * piped input (stdin), or a monitored LDM directory.
 *
 * Composes core transport reader rLDMStreamReader with pure data decoder NEXRADDecoder.
 */
class NEXRADStreamReader {
public:
  /** Callback signature invoked when a full sweep completes */
  using SweepCallback = NEXRADDecoder::SweepCallback;

  /** @brief Default constructor. */
  NEXRADStreamReader();

  /**
   * @brief Constructs stream reader with given options.
   * @param opts Stream configuration options.
   */
  explicit
  NEXRADStreamReader(const NEXRADStreamOptions& opts);

  /** @brief Destructor. */
  ~NEXRADStreamReader();

  /**
   * @brief Sets stream configuration options.
   * @param opts Stream configuration options.
   */
  void
  setOptions(const NEXRADStreamOptions& opts);

  /**
   * @brief Gets current stream configuration options.
   * @return Active options reference.
   */
  const NEXRADStreamOptions&
  getOptions() const { return myOptions; }

  /**
   * @brief Sets custom callback for immediate sweep completion notification.
   * @param cb Callback function receiving a vector of completed RadialSets.
   */
  void setSweepCallback(SweepCallback cb){ mySweepCallback = cb; }

  /**
   * @brief Accesses the internal accumulator.
   * @return Reference to internal NEXRADAccumulator.
   */
  NEXRADAccumulator& getAccumulator(){ return myDecoder.getAccumulator(); }

  /**
   * @brief Accesses the internal accumulator (const).
   * @return Const reference to internal NEXRADAccumulator.
   */
  const NEXRADAccumulator&
  getAccumulator() const { return myDecoder.getAccumulator(); }

  /**
   * @brief Accesses the internal decoder.
   */
  NEXRADDecoder& getDecoder(){ return myDecoder; }

  void setLdmBzipped(bool flag){ myOptions.ldmBzipped = flag; myDecoder.setLdmBzipped(flag); }

  bool
  getLdmBzipped() const { return myOptions.ldmBzipped; }

  void setHeaderMissing(bool flag){ myOptions.headerMissing = flag; myDecoder.setHeaderMissing(flag); }

  bool
  getHeaderMissing() const { return myOptions.headerMissing; }

  void setWriteHVAN(bool flag){ myOptions.writeHVAN = flag; myDecoder.setWriteHVAN(flag); }

  bool
  getWriteHVAN() const { return myOptions.writeHVAN; }

  /**
   * @brief Feeds raw bytes into the stream reader.
   * @param data Raw byte array.
   * @param len Number of bytes.
   */
  void
  pushBytes(const char * data, size_t len);

  /**
   * @brief Flushes any accumulated radials in the current sweep.
   */
  void
  flush();

  /**
   * @brief Resets internal stream buffers and accumulator.
   */
  void
  reset();

  /**
   * @brief Streams data from an input stream (e.g. std::cin, pipe, socket, or file).
   * @param is Input stream reference.
   * @param chunkSize Read chunk buffer size in bytes (default 64KB).
   */
  void
  readStream(std::istream& is, size_t chunkSize = 65536);

  /**
   * @brief Streams data from a file descriptor (e.g. STDIN_FILENO, pipe).
   * @param fd POSIX file descriptor.
   * @param chunkSize Read chunk buffer size in bytes (default 64KB).
   */
  void
  readFd(int fd, size_t chunkSize = 65536);

  /**
   * @brief Reads a single file from disk.
   * @param filename Path to file.
   * @return true if opened and read successfully, false otherwise.
   */
  bool
  readFile(const std::string& filename);

  /**
   * @brief Listens to an LDM directory in real-time or archive mode.
   * @param stopRequested Optional atomic bool to signal clean shutdown.
   * @return Number of files processed.
   */
  size_t
  listenDirectory(std::atomic<bool> * stopRequested = nullptr);

  /**
   * @brief Helper utility to write a completed sweep of RadialSets to NetCDF files.
   */
  static bool
  writeSweepToNetcdf(
    const std::vector<std::shared_ptr<DataType> >& sweepOutputs,
    const std::string                            & outputDir,
    bool                                         flatDir             = false,
    bool                                         uncompressed        = false,
    float                                        alignmentResolution = -1.0f)
  {
    return NEXRADDecoder::writeSweepToNetcdf(sweepOutputs, outputDir, flatDir, uncompressed, alignmentResolution);
  }

private:
  void
  handleSweepCompleted(const std::vector<std::shared_ptr<DataType> >& sweepOutputs);

  NEXRADStreamOptions myOptions;
  NEXRADDecoder myDecoder;
  rLDMStreamReader myStreamReader;
  SweepCallback mySweepCallback;
};


// Backward compatibility aliases
using AR2VStreamOptions = NEXRADStreamOptions;
using AR2VStreamReader  = NEXRADStreamReader;
} // namespace rapio
