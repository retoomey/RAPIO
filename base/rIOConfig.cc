#include "rIOConfig.h"
#include "rError.h"
#include "rStrings.h"

using namespace rapio;

IOConfig::SparseMode IOConfig::ourDefaultSparseMode = IOConfig::SparseMode::Guess;
float IOConfig::ourDefaultSparseThreshold = 0.75f;
bool IOConfig::ourDoUnsparse = true; // You want this on

void
IOConfig::setGlobalSparseDefaults(const std::string& modeStr, float threshold)
{
  std::string lowerMode = modeStr;

  Strings::toLower(lowerMode);

  if (lowerMode == SPARSE_MODE_NONE) {
    ourDefaultSparseMode = SparseMode::None;
  } else if (lowerMode == SPARSE_MODE_GUESS) {
    ourDefaultSparseMode = SparseMode::Guess;
  } else if (lowerMode == SPARSE_MODE_HARD) {
    ourDefaultSparseMode = SparseMode::Hard;
  } else if (lowerMode == SPARSE_MODE_FORCE) {
    ourDefaultSparseMode = SparseMode::Force;
  } else {
    fLogSevere("Unrecognized sparse mode '{}' in configuration. Defaulting to 'guess'.", lowerMode);
    ourDefaultSparseMode = SparseMode::Guess;
    lowerMode = "guess"; // For printing
  }

  if ((threshold >= 0.0f) && (threshold <= 1.0f)) {
    ourDefaultSparseThreshold = threshold;
  } else {
    fLogSevere("Sprase threshold out of range '{}' in configuration. Defaulting to '{}'.",
      threshold, ourDefaultSparseThreshold);
  }
  fLogInfo("Sparse settings defaulting to '{}' and '{}'", lowerMode, ourDefaultSparseThreshold);
}

IOConfig::SparseMode
IOConfig::getGlobalSparseMode()
{
  return ourDefaultSparseMode;
}

float
IOConfig::getGlobalSparseThreshold()
{
  return ourDefaultSparseThreshold;
}
