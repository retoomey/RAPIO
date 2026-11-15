/**
 * @file rNEXRADStreamReader.cc
 * @brief NEXRAD Level II streaming reader implementation delegating to rLDMStreamReader and NEXRADDecoder.
 */

#include "rNEXRADStreamReader.h"
#include <rError.h>

namespace rapio {
NEXRADStreamReader::NEXRADStreamReader()
{
  setOptions(NEXRADStreamOptions());
}

NEXRADStreamReader::NEXRADStreamReader(const NEXRADStreamOptions& opts)
{
  setOptions(opts);
}

NEXRADStreamReader::~NEXRADStreamReader()
{ }

void
NEXRADStreamReader::setOptions(const NEXRADStreamOptions& opts)
{
  myOptions = opts;

  // Configure decoder
  myDecoder.setRadarName(myOptions.radarName);
  myDecoder.setUserVCP(myOptions.userVCP);
  myDecoder.setUseVolumeTime(myOptions.useVolumeTime);
  myDecoder.setSeparateSAILS(myOptions.separateSAILS);
  bool effectiveAddLower = (myOptions.userVCP > 0) ? myOptions.addLower : true;

  myDecoder.setAddLower(effectiveAddLower);
  myDecoder.setWriteMSG2(myOptions.writeMSG2);
  myDecoder.setWriteMSG3(myOptions.writeMSG3);
  myDecoder.setLdmBzipped(myOptions.ldmBzipped);
  myDecoder.setHeaderMissing(myOptions.headerMissing);
  myDecoder.setWriteHVAN(myOptions.writeHVAN);
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
  myStreamReader.setFileCompleteCallback([this](const std::string& /*filename*/) {
      myDecoder.resetStreamState();
    });
} // NEXRADStreamReader::setOptions

void
NEXRADStreamReader::handleSweepCompleted(const std::vector<std::shared_ptr<DataType> >& sweepOutputs)
{
  if (sweepOutputs.empty()) { return; }

  // 1. If an output directory is configured, immediately write out NetCDF files
  if (!myOptions.outputDir.empty()) {
    NEXRADDecoder::writeSweepToNetcdf(sweepOutputs, myOptions.outputDir, myOptions.flatDir,
      myOptions.uncompressedNetcdf, myOptions.alignmentResolution);
  }

  // 2. Deliver sweep to user callback if registered
  if (mySweepCallback) {
    mySweepCallback(sweepOutputs);
  }
}

void
NEXRADStreamReader::reset()
{
  myStreamReader.reset();
  myDecoder.reset();
  myDecoder.setRadarName(myOptions.radarName);
  myDecoder.setUserVCP(myOptions.userVCP);
  myDecoder.setUseVolumeTime(myOptions.useVolumeTime);
  myDecoder.setSeparateSAILS(myOptions.separateSAILS);
  bool effectiveAddLower = (myOptions.userVCP > 0) ? myOptions.addLower : true;

  myDecoder.setAddLower(effectiveAddLower);
  myDecoder.setWriteMSG2(myOptions.writeMSG2);
  myDecoder.setWriteMSG3(myOptions.writeMSG3);
  myDecoder.setLdmBzipped(myOptions.ldmBzipped);
  myDecoder.setHeaderMissing(myOptions.headerMissing);
  myDecoder.setWriteHVAN(myOptions.writeHVAN);
  myDecoder.setAlignmentResolution(myOptions.alignmentResolution);
  myDecoder.setKeepCompletedRadialSets(false);
}

void
NEXRADStreamReader::pushBytes(const char * data, size_t len)
{
  myStreamReader.pushBytes(data, len);
}

void
NEXRADStreamReader::flush()
{
  myStreamReader.flush();
  myDecoder.flush();
}

void
NEXRADStreamReader::readStream(std::istream& is, size_t chunkSize)
{
  myStreamReader.readStream(is, chunkSize);
  myDecoder.flush();
}

void
NEXRADStreamReader::readFd(int fd, size_t chunkSize)
{
  myStreamReader.readFd(fd, chunkSize);
  myDecoder.flush();
}

bool
NEXRADStreamReader::readFile(const std::string& filename)
{
  bool ok = myStreamReader.readFile(filename);

  myDecoder.flush();
  return ok;
}

size_t
NEXRADStreamReader::listenDirectory(std::atomic<bool> * stopRequested)
{
  size_t count = myStreamReader.listenDirectory(stopRequested);

  myDecoder.flush();
  return count;
}
} // namespace rapio
