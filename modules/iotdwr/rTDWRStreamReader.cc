/**
 * @file rTDWRStreamReader.cc
 * @brief TDWR streaming reader implementation delegating to rLDMStreamReader and TDWRDecoder.
 */

#include "rTDWRStreamReader.h"
#include <rError.h>
#include <rStrings.h>

namespace rapio {
TDWRStreamReader::TDWRStreamReader()
{
  setOptions(TDWRStreamOptions());
}

TDWRStreamReader::TDWRStreamReader(const TDWRStreamOptions& opts)
{
  setOptions(opts);
}

TDWRStreamReader::~TDWRStreamReader()
{ }

void
TDWRStreamReader::setOptions(const TDWRStreamOptions& opts)
{
  myOptions = opts;

  // Configure pure data decoder
  myDecoder.setRadarName(myOptions.radarName);
  myDecoder.setUserVCP(myOptions.userVCP);
  myDecoder.setUseVolumeTime(myOptions.useVolumeTime);
  myDecoder.setNoiseThreshold(myOptions.noiseThreshold);
  if (!myOptions.magneticOffsetStr.empty()) {
    myDecoder.setMagneticOffset(myOptions.magneticOffsetStr);
  }
  myDecoder.setFrequency(myOptions.frequencyMHz);
  myDecoder.setAlignmentResolution(myOptions.alignmentResolution);
  myDecoder.setKeepCompletedRadialSets(false);

  myDecoder.setSweepCallback([this](const std::vector<std::shared_ptr<DataType> >& sweepOutputs) {
      handleSweepCompleted(sweepOutputs);
    });

  // Configure transport reader
  LDMStreamOptions ldmOpts;

  ldmOpts.inputDir       = myOptions.inputDir;
  ldmOpts.pattern        = myOptions.pattern;
  ldmOpts.killFiles      = myOptions.killFiles;
  ldmOpts.readAll        = myOptions.readAll;
  ldmOpts.onceOnly       = myOptions.onceOnly;
  ldmOpts.pipedInput     = myOptions.pipedInput;
  ldmOpts.pollIntervalMs = myOptions.pollIntervalMs;

  myStreamReader.setOptions(ldmOpts);
  myStreamReader.setPayloadCallback([this](const std::vector<char>& payload) {
      myDecoder.processBuffer(payload);
    });
  myStreamReader.setFileCompleteCallback([this](const std::string& /*filePath*/) {
      myDecoder.resetStreamState();
    });
} // TDWRStreamReader::setOptions

void
TDWRStreamReader::handleSweepCompleted(const std::vector<std::shared_ptr<DataType> >& sweepOutputs)
{
  if (sweepOutputs.empty()) {
    return;
  }

  // Write out NetCDF files if output directory specified
  if (!myOptions.outputDir.empty()) {
    std::vector<std::string> odirs;
    Strings::splitWithoutEnds(myOptions.outputDir, ' ', &odirs);
    if (odirs.size() <= 1) {
      TDWRDecoder::writeSweepToNetcdf(sweepOutputs, myOptions.outputDir, myOptions.flatDir,
        myOptions.uncompressedNetcdf, myOptions.alignmentResolution);
    } else {
      for (size_t i = 0; i < odirs.size(); ++i) {
        bool uncompressed = (i != 0);
        TDWRDecoder::writeSweepToNetcdf(sweepOutputs, odirs[i], myOptions.flatDir, uncompressed,
          myOptions.alignmentResolution);
      }
    }
  }

  // Notify user callback
  if (mySweepCallback) {
    mySweepCallback(sweepOutputs);
  }
}

void
TDWRStreamReader::reset()
{
  myStreamReader.reset();
  myDecoder.reset();
  myDecoder.setRadarName(myOptions.radarName);
  myDecoder.setUserVCP(myOptions.userVCP);
  myDecoder.setUseVolumeTime(myOptions.useVolumeTime);
  myDecoder.setNoiseThreshold(myOptions.noiseThreshold);
  if (!myOptions.magneticOffsetStr.empty()) {
    myDecoder.setMagneticOffset(myOptions.magneticOffsetStr);
  }
  myDecoder.setFrequency(myOptions.frequencyMHz);
  myDecoder.setAlignmentResolution(myOptions.alignmentResolution);
  myDecoder.setKeepCompletedRadialSets(false);
}

void
TDWRStreamReader::pushBytes(const char * data, size_t len)
{
  if (!data || (len == 0)) {
    return;
  }
  myStreamReader.pushBytes(data, len);
}

void
TDWRStreamReader::readStream(std::istream& is, size_t chunkSize)
{
  myStreamReader.readStream(is, chunkSize);
  myDecoder.flush();
}

void
TDWRStreamReader::readFd(int fd, size_t chunkSize)
{
  myStreamReader.readFd(fd, chunkSize);
  myDecoder.flush();
}

bool
TDWRStreamReader::readFile(const std::string& filename)
{
  bool ok = myStreamReader.readFile(filename);

  myDecoder.flush();
  return ok;
}

size_t
TDWRStreamReader::listenDirectory(std::atomic<bool> * stopRequested)
{
  size_t count = myStreamReader.listenDirectory(stopRequested);

  myDecoder.flush();
  return count;
}

void
TDWRStreamReader::flush()
{
  myStreamReader.flush();
  myDecoder.flush();
}
} // namespace rapio
