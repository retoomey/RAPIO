/**
 * @file rLDMStreamReader.h
 * @brief Format-agnostic LDM transport stream reader for files, directories, and streams.
 *
 * Handles LDM chunk framing, negative chunk lengths, null-byte padding, BZh synchronization,
 * and BZIP2 decompression. Completely decoupled from radar data formats.
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <istream>
#include <set>
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace rapio {
/**
 * @brief Helper to validate BZIP2 stream header and Pi block header magic.
 * Checks for "BZh[1-9]" followed by "\x31\x41\x59\x26\x53\x59" when avail >= 10,
 * or "BZh[1-9]" when 4 <= avail < 10.
 */
inline bool
isValidBzip2Header(const char * ptr, size_t avail)
{
  if (!ptr) {
    return false;
  }
  if (avail >= 10) {
    return ptr[0] == 'B' && ptr[1] == 'Z' && ptr[2] == 'h' &&
           (ptr[3] >= '1' && ptr[3] <= '9') &&
           std::memcmp(ptr + 4, "\x31\x41\x59\x26\x53\x59", 6) == 0;
  }
  if (avail >= 4) {
    return ptr[0] == 'B' && ptr[1] == 'Z' && ptr[2] == 'h' &&
           (ptr[3] >= '1' && ptr[3] <= '9');
  }
  return false;
}

/**
 * @struct LDMStreamOptions
 * @brief Configuration options for format-agnostic LDM transport streaming.
 */
struct LDMStreamOptions {
  std::string inputDir;               ///< -i: Input LDM directory to monitor
  std::string pattern;                ///< -p: Filter incoming files by pattern/prefix
  bool        killFiles      = false; ///< -K: Delete/kill LDM files after processing
  bool        readAll        = false; ///< -a: Process existing files on startup
  bool        onceOnly       = false; ///< -1: Process existing files once and exit (archive mode)
  bool        pipedInput     = false; ///< -P: Stream from stdin/pipe
  int         pollIntervalMs = 500;   ///< Polling interval in milliseconds for directory monitoring
};

/**
 * @class rLDMStreamReader
 * @brief Reusable core transport stream reader handling LDM chunking, sliding window
 * buffering, directory polling, and BZIP2 decompression.
 *
 * Pure transport layer: triggers PayloadCallback whenever a clean, decompressed
 * or raw payload buffer is successfully assembled.
 */
class rLDMStreamReader {
public:
  /** Callback signature invoked when a clean, decompressed payload buffer is assembled */
  using PayloadCallback = std::function<void (const std::vector<char>&)>;

  /** Callback signature invoked when an individual file has been completely processed */
  using FileCompleteCallback = std::function<void (const std::string&)>;

  /** @brief Default constructor. */
  rLDMStreamReader();

  /**
   * @brief Constructs stream reader with given options.
   * @param opts Stream configuration options.
   */
  explicit
  rLDMStreamReader(const LDMStreamOptions& opts);

  /** @brief Destructor. */
  ~rLDMStreamReader();

  /**
   * @brief Sets stream configuration options.
   * @param opts Stream configuration options.
   */
  void
  setOptions(const LDMStreamOptions& opts);

  /**
   * @brief Gets current stream configuration options.
   * @return Active options reference.
   */
  const LDMStreamOptions&
  getOptions() const { return myOptions; }

  /** @brief Returns true if stream reader is currently in raw BZIP2 payload mode. */
  bool
  isRawBzip2Mode() const { return myRawBzip2Mode; }

  /**
   * @brief Sets callback triggered whenever a clean payload buffer is assembled.
   * @param cb Callback function receiving decompressed payload vector.
   */
  void setPayloadCallback(PayloadCallback cb){ myPayloadCallback = std::move(cb); }

  /**
   * @brief Sets callback triggered whenever an individual file has been completely processed.
   * @param cb Callback function receiving the path of the completed file.
   */
  void setFileCompleteCallback(FileCompleteCallback cb){ myFileCompleteCallback = std::move(cb); }

  /**
   * @brief Feeds raw bytes into the transport stream reader.
   * Handles arbitrary chunk boundaries, bzip2 decompression, and invokes payload callback.
   * @param data Raw byte array.
   * @param len Number of bytes.
   */
  void
  pushBytes(const char * data, size_t len);

  /**
   * @brief Flushes any buffered bytes and processes complete chunks.
   */
  void
  flush();

  /**
   * @brief Resets stream buffers and internal state.
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
   * Monitors directory, filters files by pattern, handles -K, -a, -1, and stopRequested.
   * @param stopRequested Optional atomic bool to signal clean shutdown.
   * @return Number of files processed.
   */
  size_t
  listenDirectory(std::atomic<bool> * stopRequested = nullptr);

private:
  void
  processStreamBuffer(bool isFlushing = false);
  void
  compactStreamBuffer();
  void
  appendStreamBytes(const char * data, size_t len);

  LDMStreamOptions myOptions;
  PayloadCallback myPayloadCallback;
  FileCompleteCallback myFileCompleteCallback;

  // Stream state & buffers (sliding window)
  std::vector<char> myStreamBuffer;
  size_t myStreamReadPos = 0;
  bool myRawBzip2Mode    = false;
  std::set<std::string> myProcessedFiles;
};
} // namespace rapio
