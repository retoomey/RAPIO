/**
 * @file rLDMStreamReader.cc
 * @brief Format-agnostic LDM transport stream reader implementation.
 */

#include "rLDMStreamReader.h"
#include <rStrings.h>
#include <rError.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <endian.h>

namespace fs = std::filesystem;

namespace rapio {
namespace {
inline uint16_t
decodeBE16(const void * ptr)
{
  uint16_t val;

  std::memcpy(&val, ptr, sizeof(val));
  return be16toh(val);
}

inline int32_t
decodeBES32(const void * ptr)
{
  uint32_t val;

  std::memcpy(&val, ptr, sizeof(val));
  return static_cast<int32_t>(be32toh(val));
}
} // namespace

rLDMStreamReader::rLDMStreamReader()
{
  setOptions(LDMStreamOptions());
}

rLDMStreamReader::rLDMStreamReader(const LDMStreamOptions& opts)
{
  setOptions(opts);
}

rLDMStreamReader::~rLDMStreamReader()
{ }

void
rLDMStreamReader::setOptions(const LDMStreamOptions& opts)
{
  myOptions = opts;
}

void
rLDMStreamReader::reset()
{
  myStreamBuffer.clear();
  myStreamReadPos = 0;
  myRawBzip2Mode  = false;
}

void
rLDMStreamReader::compactStreamBuffer()
{
  if (myStreamReadPos > 0) {
    if (myStreamReadPos >= myStreamBuffer.size()) {
      myStreamBuffer.clear();
    } else {
      size_t unread = myStreamBuffer.size() - myStreamReadPos;
      std::memmove(myStreamBuffer.data(), myStreamBuffer.data() + myStreamReadPos, unread);
      myStreamBuffer.resize(unread);
    }
    myStreamReadPos = 0;
  }
}

void
rLDMStreamReader::appendStreamBytes(const char * data, size_t len)
{
  if ((len == 0) || (data == nullptr)) { return; }

  if (myStreamReadPos > 0) {
    if (myStreamReadPos >= myStreamBuffer.size()) {
      myStreamBuffer.clear();
      myStreamReadPos = 0;
    } else {
      size_t remainingCap = (myStreamBuffer.capacity() > myStreamBuffer.size()) ?
        (myStreamBuffer.capacity() - myStreamBuffer.size()) :
        0;
      if ((remainingCap < len) || (remainingCap < 65536)) {
        compactStreamBuffer();
      }
    }
  }

  myStreamBuffer.insert(myStreamBuffer.end(), data, data + len);
}

void
rLDMStreamReader::pushBytes(const char * data, size_t len)
{
  if ((len == 0) || (data == nullptr)) { return; }

  appendStreamBytes(data, len);
  processStreamBuffer();
}

void
rLDMStreamReader::flush()
{
  processStreamBuffer(true);
}

void
rLDMStreamReader::processStreamBuffer(bool isFlushing)
{
  if (myRawBzip2Mode) {
    size_t streamAvail = myStreamBuffer.size() - myStreamReadPos;
    if (streamAvail > 0) {
      const char * curPtr      = myStreamBuffer.data() + myStreamReadPos;
      size_t foundPos          = std::string::npos;
      bool foundConfirmed      = false;
      bool foundPotentialAtEnd = false;

      auto isPotentialHeader = [](const char * p, size_t rem) -> bool {
          if (rem < 2) { return false; }
          if ((p[0] != 'A') || (p[1] != 'R') ) { return false; }
          if ((rem <= 8) && (std::strncmp(p, "ARCHIVE2", rem) == 0)) {
            return true;
          }
          if ((rem <= 6) && (std::strncmp(p, "AR2V00", rem) == 0)) {
            return true;
          }
          if ((rem > 6) && (std::strncmp(p, "AR2V00", 6) == 0)) {
            return true;
          }
          return false;
        };

      for (size_t pos = 0; pos < streamAvail; ++pos) {
        size_t rem = streamAvail - pos;
        if ((rem >= 4) &&
          ((std::strncmp(curPtr + pos, "AR2V", 4) == 0) || (std::strncmp(curPtr + pos, "ARCH", 4) == 0)))
        {
          if (rem >= 8) {
            if ((std::strncmp(curPtr + pos, "ARCHIVE2", 8) == 0) || (std::strncmp(curPtr + pos, "AR2V00", 6) == 0)) {
              foundPos       = pos;
              foundConfirmed = true;
              break;
            }
          } else {
            // < 8 bytes remaining from match at very end of buffer
            if (isPotentialHeader(curPtr + pos, rem)) {
              foundPos = pos;
              foundPotentialAtEnd = true;
              break;
            }
          }
        } else if ((rem < 4) && isPotentialHeader(curPtr + pos, rem)) {
          foundPos = pos;
          foundPotentialAtEnd = true;
          break;
        }
      }

      if (foundConfirmed) {
        if (foundPos > 0) {
          if (myPayloadCallback) {
            std::vector<char> rawChunk(curPtr, curPtr + foundPos);
            myPayloadCallback(rawChunk);
          }
          myStreamReadPos += foundPos;
        }
        myRawBzip2Mode = false;
        // Fall through to main loop so 24-byte volume header and subsequent records are processed cleanly
      } else if (foundPotentialAtEnd) {
        if (foundPos > 0) {
          if (myPayloadCallback) {
            std::vector<char> rawChunk(curPtr, curPtr + foundPos);
            myPayloadCallback(rawChunk);
          }
          myStreamReadPos += foundPos;
        }
        return; // wait for more bytes before deciding
      } else {
        if (myPayloadCallback) {
          std::vector<char> rawChunk(curPtr, curPtr + streamAvail);
          myPayloadCallback(rawChunk);
        }
        myStreamReadPos += streamAvail;
        return;
      }
    } else {
      return;
    }
  }

  // 1. Chunked LDM stream parsing loop
  while ((myStreamBuffer.size() - myStreamReadPos) >= 4) {
    const char * curPtr = myStreamBuffer.data() + myStreamReadPos;
    size_t streamAvail  = myStreamBuffer.size() - myStreamReadPos;

    // Check for 24-byte Level II Volume Header (AR2V / ARCHIVE2)
    if ((streamAvail >= 4) &&
      ((std::strncmp(curPtr, "AR2V", 4) == 0) || (std::strncmp(curPtr, "ARCH", 4) == 0)))
    {
      if (streamAvail < 24) {
        break; // Wait for full 24-byte header
      }
      if (myPayloadCallback) {
        std::vector<char> headerChunk(curPtr, curPtr + 24);
        myPayloadCallback(headerChunk);
      }
      myStreamReadPos += 24;
      continue;
    }

    // Raw BZIP2 stream without chunk framing
    if ((streamAvail >= 4) &&
      (curPtr[0] == 'B') && (curPtr[1] == 'Z') && (curPtr[2] == 'h') &&
      ((curPtr[3] >= '1') && (curPtr[3] <= '9')))
    {
      if ((streamAvail < 10) && !isFlushing) {
        break; // Wait for 10 bytes unless flushing so the 6-byte pi magic \x31\x41\x59\x26\x53\x59 is confirmed
      }
      if (isValidBzip2Header(curPtr, streamAvail)) {
        myRawBzip2Mode = true;
        if (myPayloadCallback) {
          std::vector<char> rawChunk(curPtr, curPtr + streamAvail);
          myPayloadCallback(rawChunk);
        }
        myStreamReadPos += streamAvail;
        continue;
      }
    }

    // Unframed TDWR packet check (0x2b00 or 0x2b01)
    const auto * ucur = reinterpret_cast<const uint8_t *>(curPtr);
    if ((streamAvail >= 2) && (ucur[0] == 0x2b) && ((ucur[1] == 0x00) || (ucur[1] == 0x01))) {
      if (streamAvail < 4) {
        break;
      }
      uint16_t len16  = decodeBE16(curPtr + 2);
      bool isBZhAhead = (streamAvail >= 7 && curPtr[4] == 'B' && curPtr[5] == 'Z' && curPtr[6] == 'h');
      if (!isBZhAhead && (len16 <= 6144)) {
        size_t minExpected = (ucur[1] == 0x00) ? 4852 : 6004;
        size_t frameSize   = (len16 >= minExpected) ? static_cast<size_t>(len16) : minExpected;
        if (streamAvail < frameSize) {
          break; // wait for that many bytes
        }
        if (myPayloadCallback) {
          std::vector<char> tdwrChunk(curPtr, curPtr + frameSize);
          myPayloadCallback(tdwrChunk);
        }
        myStreamReadPos += frameSize;
        continue;
      }
    }

    int32_t rec_len = decodeBES32(curPtr);

    if ((rec_len == INT32_MIN) || (std::abs(rec_len) > 5000000)) {
      // Corrupt record size or leading header bytes: perform byte-by-byte scan for BZh
      bool syncFound   = false;
      auto searchStart = myStreamBuffer.begin() + myStreamReadPos;

      while (searchStart < myStreamBuffer.end()) {
        auto it = std::search(
          searchStart, myStreamBuffer.end(),
          "\x42\x5A\x68", "\x42\x5A\x68" + 3);

        if (it == myStreamBuffer.end()) {
          break;
        }

        // Validate chunk boundary: read the 4 bytes immediately preceding BZh
        // and verify they decode to a sensible chunk_len (> 0 and < 5000000)
        size_t distFromReadPos = static_cast<size_t>(std::distance(myStreamBuffer.begin() + myStreamReadPos, it));
        if (distFromReadPos >= 4) {
          size_t byteIdx     = static_cast<size_t>(std::distance(myStreamBuffer.begin(), it));
          size_t availFromIt = myStreamBuffer.size() - byteIdx;
          if (isValidBzip2Header(myStreamBuffer.data() + byteIdx, availFromIt)) {
            int32_t potentialLen = decodeBES32(myStreamBuffer.data() + byteIdx - 4);
            if (potentialLen != INT32_MIN) {
              int32_t absLen = std::abs(potentialLen);
              if ((absLen > 0) && (absLen < 5000000)) {
                // Valid LDM chunk boundary confirmed at byteIdx - 4!
                // If there are leading bytes before this first chunk boundary (e.g. 24-byte volume header),
                // dispatch them as an initial payload buffer to the callback:
                if (byteIdx - 4 > myStreamReadPos) {
                  if (myPayloadCallback) {
                    std::vector<char> leading(myStreamBuffer.data() + myStreamReadPos,
                      myStreamBuffer.data() + byteIdx - 4);
                    myPayloadCallback(leading);
                  }
                }
                myStreamReadPos = byteIdx - 4;
                syncFound       = true;
                break;
              }
            }
          }
        }

        // False positive BZh in payload or corrupt length prefix: advance past this BZh and keep searching
        searchStart = it + 1;
      }

      if (syncFound) {
        continue;
      } else {
        // No valid BZh sync found in available buffer: retain up to last 6 bytes
        // (4 bytes length prefix + 2 bytes partial BZh) in case chunk marker is split across packets
        size_t currentAvail = myStreamBuffer.size() - myStreamReadPos;
        if (currentAvail > 6) {
          myStreamReadPos = myStreamBuffer.size() - 6;
        }
        break; // Wait for more stream bytes
      }
    }

    // Negative record length signifies final record in volume; safe unsigned cast
    uint32_t chunk_len =
      (rec_len < 0) ? static_cast<uint32_t>(-static_cast<int64_t>(rec_len)) : static_cast<uint32_t>(rec_len);

    if (chunk_len == 0) {
      // Advance past zero-padding byte-by-byte until hitting a non-zero byte
      size_t nullBytes = 0;
      while (nullBytes < streamAvail) {
        if ((streamAvail - nullBytes >= 3) &&
          (curPtr[nullBytes] == 'B') &&
          (curPtr[nullBytes + 1] == 'Z') &&
          (curPtr[nullBytes + 2] == 'h'))
        {
          break;
        }
        if ((streamAvail - nullBytes >= 7) &&
          (curPtr[nullBytes + 4] == 'B') &&
          (curPtr[nullBytes + 5] == 'Z') &&
          (curPtr[nullBytes + 6] == 'h'))
        {
          break;
        }
        if (curPtr[nullBytes] != 0) {
          break;
        }
        nullBytes++;
      }
      if (nullBytes > 0) {
        fLogInfo("rLDMStreamReader: Spool of {} null bytes encountered (LDM padding), advancing buffer", nullBytes);
        myStreamReadPos += nullBytes;
        streamAvail     -= nullBytes;
        curPtr += nullBytes;
      }
      if ((streamAvail < 4) && std::all_of(curPtr, curPtr + streamAvail, [](char c) {
          return c == 0;
        }))
      {
        myStreamBuffer.clear();
        myStreamReadPos = 0;
      }
      continue;
    }

    size_t required = static_cast<size_t>(chunk_len) + 4;
    if (streamAvail < required) {
      // Record incomplete in stream; wait for more bytes
      break;
    }

    const char * chunk_ptr = curPtr + 4;
    // Pass the extracted raw chunk immediately to callback without decompression
    if (myPayloadCallback) {
      std::vector<char> rawChunk(chunk_ptr, chunk_ptr + chunk_len);
      myPayloadCallback(rawChunk);
    }

    myStreamReadPos += required;
  }

  // Trailing null padding: if fewer than 4 bytes remain and all are 0x00, clear rather than stall
  size_t remaining = myStreamBuffer.size() - myStreamReadPos;

  if ((remaining > 0) && (remaining < 4)) {
    const char * curPtr = myStreamBuffer.data() + myStreamReadPos;
    if (std::all_of(curPtr, curPtr + remaining, [](char c) {
        return c == 0;
      }))
    {
      myStreamBuffer.clear();
      myStreamReadPos = 0;
    }
  }

  if (myStreamReadPos >= myStreamBuffer.size()) {
    myStreamBuffer.clear();
    myStreamReadPos = 0;
  }
} // rLDMStreamReader::processStreamBuffer

void
rLDMStreamReader::readStream(std::istream& is, size_t chunkSize)
{
  std::vector<char> buf(chunkSize);

  while (is.good() && !is.eof()) {
    is.read(buf.data(), chunkSize);
    std::streamsize bytesRead = is.gcount();
    if (bytesRead > 0) {
      pushBytes(buf.data(), static_cast<size_t>(bytesRead));
    }
  }
  flush();
}

void
rLDMStreamReader::readFd(int fd, size_t chunkSize)
{
  std::vector<char> buf(chunkSize);

  while (true) {
    ssize_t bytesRead = ::read(fd, buf.data(), chunkSize);
    if (bytesRead <= 0) {
      break;
    }
    pushBytes(buf.data(), static_cast<size_t>(bytesRead));
  }
  flush();
  if (myFileCompleteCallback) {
    myFileCompleteCallback("<stdin>");
  }
}

bool
rLDMStreamReader::readFile(const std::string& filename)
{
  std::ifstream in(filename, std::ios::binary);

  if (!in.is_open()) {
    fLogSevere("rLDMStreamReader: Failed to open file '{}'", filename);
    return false;
  }
  reset();
  readStream(in);
  in.close();
  if (myFileCompleteCallback) {
    myFileCompleteCallback(filename);
  }
  return true;
}

size_t
rLDMStreamReader::listenDirectory(std::atomic<bool> * stopRequested)
{
  if (myOptions.inputDir.empty()) {
    fLogSevere("rLDMStreamReader: input directory is empty");
    return 0;
  }

  if (!fs::exists(myOptions.inputDir)) {
    fLogSevere("rLDMStreamReader: input directory '{}' does not exist", myOptions.inputDir);
    return 0;
  }

  // Handle -P piped input
  if (myOptions.pipedInput) {
    fLogInfo("rLDMStreamReader: Streaming input from stdin");
    readFd(STDIN_FILENO);
    return 1;
  }

  bool firstScan        = true;
  size_t filesProcessed = 0;

  fLogInfo(
    "rLDMStreamReader: Listening to directory '{}' (pattern: '{}', readAll: {}, onceOnly: {}, kill: {})",
    myOptions.inputDir, myOptions.pattern,
    myOptions.readAll, myOptions.onceOnly, myOptions.killFiles);

  while (true) {
    if (stopRequested && stopRequested->load()) {
      break;
    }

    std::vector<std::string> candidateFiles;

    try {
      for (const auto& entry : fs::directory_iterator(myOptions.inputDir)) {
        if (!entry.is_regular_file()) { continue; }

        std::string filename = entry.path().filename().string();
        if (filename.empty() || (filename[0] == '.')) { continue; }
        if (filename.find(".tmp") != std::string::npos) { continue; }

        if (!myOptions.pattern.empty()) {
          if (!rapio::Strings::beginsWith(filename, myOptions.pattern)) {
            continue;
          }
        }

        std::string fullPath = entry.path().string();
        if (myProcessedFiles.find(fullPath) == myProcessedFiles.end()) {
          candidateFiles.push_back(fullPath);
        }
      }
    } catch (const std::exception& e) {
      fLogSevere("rLDMStreamReader: Error scanning directory {}: {}", myOptions.inputDir, e.what());
    }

    std::sort(candidateFiles.begin(), candidateFiles.end());

    // If not readAll (-a) on first scan, ignore historical files except optionally the latest
    if (firstScan && !myOptions.readAll) {
      firstScan = false;
      if (!candidateFiles.empty()) {
        std::string latestFile = candidateFiles.back();
        for (const auto& f : candidateFiles) {
          if (f != latestFile) {
            myProcessedFiles.insert(f);
          }
        }
        candidateFiles.clear();
        candidateFiles.push_back(latestFile);
      }
    } else {
      firstScan = false;
    }

    for (const auto& filePath : candidateFiles) {
      if (stopRequested && stopRequested->load()) {
        break;
      }

      fLogInfo("rLDMStreamReader: Processing file '{}'", filePath);

      std::ifstream in(filePath, std::ios::binary);
      if (in.is_open()) {
        reset();
        readStream(in);
        in.close();

        myProcessedFiles.insert(filePath);
        filesProcessed++;

        if (myFileCompleteCallback) {
          myFileCompleteCallback(filePath);
        }

        if (myOptions.killFiles) {
          std::error_code ec;
          fs::remove(filePath, ec);
          if (ec) {
            fLogInfo("rLDMStreamReader: Could not remove file {}: {}", filePath, ec.message());
          } else {
            fLogInfo("rLDMStreamReader: Deleted processed file '{}'", filePath);
            myProcessedFiles.erase(filePath);
          }
        }
      } else {
        fLogSevere("rLDMStreamReader: Failed to open file '{}'", filePath);
      }
    }

    if (myProcessedFiles.size() > 10000) {
      for (auto it = myProcessedFiles.begin(); it != myProcessedFiles.end();) {
        std::error_code ec;
        if (!fs::exists(*it, ec)) {
          it = myProcessedFiles.erase(it);
        } else {
          ++it;
        }
      }
    }

    if (myOptions.onceOnly) {
      break;
    }

    if (stopRequested && stopRequested->load()) {
      break;
    }

    constexpr int sleepStepMs = 50;
    int remainingMs = myOptions.pollIntervalMs;
    while (remainingMs > 0) {
      if (stopRequested && stopRequested->load()) {
        break;
      }
      int sleepMs = std::min(remainingMs, sleepStepMs);
      std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
      remainingMs -= sleepMs;
    }
  }

  return filesProcessed;
} // rLDMStreamReader::listenDirectory
} // namespace rapio
