#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

/**
 * @file rIOPostProcessor.h
 * @brief Defines the I/O post-processing pipeline.
 * This pipeline replaces monolithic post-write execution, allowing discrete,
 * testable steps (like compression, atomic renaming, and external command execution)
 * to be chained together dynamically based on output configuration keys.
 *
 * If we want additional post-writing operations here such as pushing to a bucket,
 * etc. then we can simply add a new module and update buildPipeline.
 *
 * @author Robert Toomey
 */

namespace rapio {
/**
 * @class PostProcessStep
 * @brief Abstract base class for a single discrete step in the post-processing pipeline.
 */
class PostProcessStep {
public:
  virtual
  ~PostProcessStep() = default;

  /**
   * @brief Executes the pipeline step on the current file.
   * @param currentFilePath The path to the file as it currently exists in the pipeline.
   * @param keys A dictionary of configuration keys governing the output behavior.
   * @return std::string The new "current" filepath (e.g., if the file was compressed and renamed).
   * Returns an empty string ("") if the step fails to halt the pipeline.
   */
  virtual std::string
  execute(const std::string           & currentFilePath,
    std::map<std::string, std::string>& keys) = 0;
};

/**
 * @class CompressionStep
 * @brief Pipeline step that compresses the file using a specified algorithm.
 */
class CompressionStep : public PostProcessStep {
public:

  /**
   * @brief Constructs a CompressionStep.
   * @param compressType The string identifier for the compression type (e.g., "gz", "bz2").
   */
  explicit CompressionStep(const std::string& compressType) : myCompressType(compressType){ }

  std::string
  execute(const std::string           & currentFilePath,
    std::map<std::string, std::string>& keys) override;
private:
  std::string myCompressType;
};

/**
 * @class AtomicRenameStep
 * @brief Pipeline step that atomically moves the working file to its final configured destination.
 */
class AtomicRenameStep : public PostProcessStep {
public:
  std::string
  execute(const std::string           & currentFilePath,
    std::map<std::string, std::string>& keys) override;
};

/**
 * @class SafeCommandStep
 * @brief Pipeline step that safely executes an external shell command on the file.
 * @details Safely injects the filename into the argument vector to prevent
 * shell expansion and command injection vulnerabilities.
 */
class SafeCommandStep : public PostProcessStep {
public:

  /**
   * @brief Constructs a SafeCommandStep.
   * @param commandTemplate The command to execute, containing the macro "%filename%".
   */
  explicit SafeCommandStep(const std::string& commandTemplate) : myCommandTemplate(commandTemplate){ }

  std::string
  execute(const std::string           & currentFilePath,
    std::map<std::string, std::string>& keys) override;
private:
  std::string myCommandTemplate;
};

/**
 * @class LDMInsertStep
 * @brief Specialized pipeline step for inserting a file into the Local Data Manager (LDM).
 */
class LDMInsertStep : public PostProcessStep {
public:
  std::string
  execute(const std::string           & currentFilePath,
    std::map<std::string, std::string>& keys) override;

  /**
   * @brief The single source of truth for constructing the pqinsert command vector.
   * @param filepath The file to insert.
   * @return std::vector<std::string> The safely tokenized command arguments.
   */
  static std::vector<std::string>
  buildLdmArgs(const std::string& filepath);
};

/**
 * @class IOPostProcessor
 * @brief The Pipeline Manager: Assembles and runs the post-processing steps.
 */
class IOPostProcessor {
public:
  IOPostProcessor() = default;

  /**
   * @brief Inspects the keys map and builds the sequential pipeline dynamically.
   * @param keys The configuration keys (e.g., "compression", "postwrite") that dictate the steps.
   */
  void
  buildPipeline(const std::map<std::string, std::string>& keys);

  /**
   * @brief Pushes the initial file through the assembled chain.
   * @param initialTempFile The path to the initial temporary file written by the I/O module.
   * @param keys The configuration map, passed along to each step.
   * @return true If all steps in the pipeline completed successfully.
   * @return false If any step in the pipeline failed or aborted.
   */
  bool
  run(const std::string& initialTempFile, std::map<std::string, std::string>& keys);

private:
  std::vector<std::unique_ptr<PostProcessStep> > myPipeline;
};
} // namespace rapio
