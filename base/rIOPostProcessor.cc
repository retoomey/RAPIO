#include "rIOPostProcessor.h"
#include "rError.h"
#include "rOS.h"
#include "rDataFilter.h"
#include "rFactory.h"
#include "rStrings.h"

// Needed for safe boost::process execution
#include "rBOOST.h"
BOOST_WRAP_PUSH
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
BOOST_WRAP_POP

namespace rapio {
std::string
CompressionStep::execute(const std::string& currentFilePath,
  std::map<std::string, std::string>      & keys)
{
  std::shared_ptr<DataFilter> f = Factory<DataFilter>::get(myCompressType, "IO writer");

  if (!f) {
    fLogSevere("Compression requested but module '{}' not found.", myCompressType);
    return ""; // Fail fast
  }

  std::string tmpgz = OS::getUniqueTemporaryFile(myCompressType + "-");

  if (f->applyURL(URL(currentFilePath), URL(tmpgz), keys)) {
    fLogDebug("Compressed {} with '{}' to {}", currentFilePath, myCompressType, tmpgz);
    OS::deleteFile(currentFilePath); // Clean up the uncompressed temp file

    // Append the compression suffix to the intended final target name
    keys["filename"] = keys["filename"] + "." + myCompressType;
    return tmpgz;
  }

  fLogSevere("Unable to compress {} with '{}' to {}", currentFilePath, myCompressType, tmpgz);
  return "";
}

std::string
AtomicRenameStep::execute(const std::string& currentFilePath,
  std::map<std::string, std::string>       & keys)
{
  std::string targetFile = keys["filename"];

  fLogDebug("Migrating {} to {}", currentFilePath, targetFile);

  if (OS::moveFile(currentFilePath, targetFile)) {
    return targetFile;
  }

  fLogSevere("Unable to move temp file {} to final location {}", currentFilePath, targetFile);
  return "";
}

std::string
SafeCommandStep::execute(const std::string& currentFilePath,
  std::map<std::string, std::string>      & keys)
{
  if (OS::runCommandOnFile(myCommandTemplate, currentFilePath, true)) {
    return currentFilePath;
  }

  fLogSevere("Pipeline aborted: Post-command '{}' failed.", myCommandTemplate);
  return "";
}

std::vector<std::string>
LDMInsertStep::buildLdmArgs(const std::string& filepath)
{
  // If you ever need to add a flag, you only do it right here.
  return { "pqinsert", "-v", "-f", "EXP", filepath };
}

std::string
LDMInsertStep::execute(const std::string& currentFilePath,
  std::map<std::string, std::string>    & keys)
{
  fLogDebug("Executing LDM insertion for: {}", currentFilePath);

  std::vector<std::string> args = buildLdmArgs(currentFilePath);
  std::vector<std::string> output;

  // Use the safe, vector-based executor
  if (OS::runProcessArgs(args, output) != -1) {
    for (const auto& line : output) {
      fLogInfo("   [LDM] {}", line);
    }
    return currentFilePath;
  }

  fLogSevere("Pipeline aborted: LDM insertion failed for {}", currentFilePath);
  return "";
}

void
IOPostProcessor::buildPipeline(const std::map<std::string, std::string>& keys)
{
  myPipeline.clear();

  // 1. Compression (Opt-in based on keys)
  auto compIt = keys.find("compression");

  if ((compIt != keys.end()) && !compIt->second.empty()) {
    myPipeline.push_back(std::make_unique<CompressionStep>(compIt->second));
  }

  // 2. Rename (Mandatory to move from temp space to final resting place)
  myPipeline.push_back(std::make_unique<AtomicRenameStep>());

  // 3. Post-write execution / LDM Insertion
  auto cmdIt = keys.find("postwrite");

  if ((cmdIt != keys.end()) && !cmdIt->second.empty()) {
    if (cmdIt->second == "ldm") {
      // Route specifically to our LDM class
      myPipeline.push_back(std::make_unique<LDMInsertStep>());
    } else {
      // Route to the generic safe shell executor
      myPipeline.push_back(std::make_unique<SafeCommandStep>(cmdIt->second));
    }
  }
}

bool
IOPostProcessor::run(const std::string& initialTempFile, std::map<std::string, std::string>& keys)
{
  std::string currentFile = initialTempFile;

  for (auto& step : myPipeline) {
    currentFile = step->execute(currentFile, keys);
    if (currentFile.empty()) {
      fLogSevere("I/O Post-Processing pipeline aborted.");
      return false;
    }
  }

  return true;
}
} // namespace rapio
