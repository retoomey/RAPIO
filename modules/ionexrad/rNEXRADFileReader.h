/**
 * @file rNEXRADFileReader.h
 * @brief Static file and in-memory buffer reader for NEXRAD Level II (.ar2v).
 *
 * Implements Phase 1 static I/O: loads entire .ar2v files, extracts the 24-byte
 * volume header and subsequent BZIP2-compressed records, feeds records through
 * NEXRADAccumulator, and returns a rapio::DataType (MultiDataType of RadialSets).
 */

#pragma once

#include "rNEXRADAccumulator.h"
#include <rIOConfig.h>
#include <rDataType.h>
#include <memory>
#include <vector>

namespace rapio {
/**
 * @class NEXRADFileReader
 * @brief Handles static reading of Level II .ar2v files from disk or memory buffers.
 */
class NEXRADFileReader {
public:
  /** @brief Default constructor. */
  NEXRADFileReader();

  /** @brief Destructor. */
  ~NEXRADFileReader();

  /**
   * @brief Reads an .ar2v file from disk.
   * @param filename Path to the .ar2v file.
   * @param config RAPIO IOConfig configuration.
   * @return Shared pointer to DataType (MultiDataType containing RadialSet objects).
   */
  std::shared_ptr<DataType>
  readFile(const std::string& filename, IOConfig& config);

  /**
   * @brief Reads an .ar2v payload from a memory buffer.
   * @param buffer Raw byte buffer.
   * @param config RAPIO IOConfig configuration.
   * @return Shared pointer to DataType (MultiDataType containing RadialSet objects).
   */
  std::shared_ptr<DataType>
  readFromBuffer(const std::vector<char>& buffer, IOConfig& config);

private:

  /**
   * @brief Parses an uncompressed or partially compressed .ar2v buffer into decoder using IOConfig.
   * @param data Buffer pointer.
   * @param datalen Buffer size in bytes.
   * @param decoder Configured decoder instance.
   * @param config IOConfig containing options.
   * @return true on successful parse, false on error.
   */
  bool
  parseBuffer(const char * data, size_t datalen, NEXRADDecoder& decoder, const IOConfig& config);

  /**
   * @brief Parses an uncompressed or partially compressed .ar2v buffer into accumulator.
   * @param data Buffer pointer.
   * @param datalen Buffer size in bytes.
   * @param accumulator Destination accumulator.
   * @return true on successful parse, false on error.
   */
  bool
  parseBuffer(const char * data, size_t datalen, NEXRADAccumulator& accumulator);
};

// Backward compatibility alias
using AR2VFileReader = NEXRADFileReader;
} // namespace rapio
