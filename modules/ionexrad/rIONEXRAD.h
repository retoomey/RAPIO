/**
 * @file rIONEXRAD.h
 * @brief Dynamic RAPIO IODataType plugin for NEXRAD Level II (.ar2v).
 *
 * Exposes the ioar2v module to RAPIO's runtime plugin architecture, supporting
 * static file reading, buffer decoding, and streaming ingest invocation.
 */

#pragma once

#include <rIODataType.h>

namespace rapio {
/**
 * @class IONEXRAD
 * @brief Main I/O DataType module handler for NEXRAD Level II (.ar2v) files and streams.
 * @author Robert Toomey
 */
class IONEXRAD : public IODataType {
public:
  /** @brief Default constructor. Registers module identification. */
  IONEXRAD();

  /** @brief Virtual destructor. */
  virtual
  ~IONEXRAD();

  /**
   * @brief Returns dynamic help information for configuration keys.
   * @param key Option key.
   * @return Formatted documentation string.
   */
  virtual std::string
  getHelpString(const std::string& key) override;

  /**
   * @brief Initializes the module and registers standard keys.
   */
  virtual void
  initialize() override;

  /**
   * @brief Creates DataType from file, directory, or stream URL specified in IOConfig.
   * Supports both static file loading (returns MultiDataType of RadialSets) and
   * streaming directory listening when configured.
   * @param config I/O configuration settings.
   * @return Shared pointer to DataType, or nullptr on failure.
   */
  virtual std::shared_ptr<DataType>
  createDataType(IOConfig& config) override;

  /**
   * @brief Creates DataType directly from an in-memory buffer.
   * @param buffer Byte buffer containing .ar2v data.
   * @return Shared pointer to DataType, or nullptr on failure.
   */
  virtual std::shared_ptr<DataType>
  createDataTypeFromBuffer(std::vector<char>& buffer) override;
};

// Backward compatibility alias
using IOAR2V = IONEXRAD;
} // namespace rapio
