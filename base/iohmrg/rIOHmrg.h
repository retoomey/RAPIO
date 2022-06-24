#pragma once

#include "rIODataType.h"

#include "rHmrgProductInfo.h"
#include "rIO.h"
#include "rOS.h"

#include <zlib.h>
#include <string.h> // errno strs

namespace rapio {
/* Exception wrapper for Errno gzip and  C function calls, etc. */
class ErrnoException : public IO, public std::exception {
public:
  ErrnoException(int err, const std::string& c) : std::exception(), retval(err),
    command(c)
  { }

  virtual ~ErrnoException() throw() { }

  int
  getErrnoVal() const
  {
    return (retval);
  }

  std::string
  getErrnoCommand() const
  {
    return (command);
  }

  std::string
  getErrnoStr() const
  {
    return (std::string(strerror(retval)) + " on '" + command + "'");
  }

private:

  /** The Errno value from the command (copied from errno) */
  int retval;

  /** The Errno command we wrapped */
  std::string command;
};

/**
 * Read HMRG binary files, an internal format we use in NSSL
 * Based on reader work done by others
 *
 * @author Jian Zhang
 * @author Carrie Langston
 * @author Robert Toomey
 */
class IOHmrg : public IODataType {
public:

  /** Help for ioimage module */
  virtual std::string
  getHelpString(const std::string& key) override;

  // Registering of classes ---------------------------------------------
  virtual void
  initialize() override;

  // READING ------------------------------------------------------------
  //

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(const std::string& params) override;

  /** Convert scaled compressed int to float.  Grouping the uncompression logic here inline,
   * this 'should' optimize in compiler to inline. */
  static inline float
  fromHmrgValue(short int v, const bool needSwap, const int dataUnavailable, const int dataMissing,
    const float dataScale)
  {
    float out;

    if (needSwap) { OS::byteswap(v); }
    if (v == dataUnavailable) {
      out = Constants::DataUnavailable;
    } else if (v == dataMissing) {
      out = Constants::MissingData;
    } else {
      out = (float) v / (float) dataScale;
    }
    return out;
  }

  /** Convert float to scaled compressed int. */
  static inline short int
  toHmrgValue(float v, const bool needSwap, const int dataUnavailable, const int dataMissing,
    const float dataScale)
  {
    short int out;

    if (v == Constants::DataUnavailable) {
      out = dataUnavailable;
    } else if (v == Constants::MissingData) {
      out = dataMissing;
    } else {
      out = v * dataScale;
      if (needSwap) { OS::byteswap(out); }
    }
    return out;
  }

  /** What we consider a valid year in the MRMS binary file,
   * used for validation of datatype */
  static bool
  isMRMSValidYear(int year);

  /** Read a scaled integer with correct endian and return as a float */
  static float
  readScaledInt(gzFile fp, float scale);

  /** Write a scaled integer with correct endian */
  static void
  writeScaledInt(gzFile fp, float w, float scale);

  /** Read an integer with correct endian and return as an int */
  static int
  readInt(gzFile fp);

  /** Write an integer with correct endian and return as an int */
  static void
  writeInt(gzFile fp, int w);

  /** Read a float with correct endian and return as a float */
  static int
  readFloat(gzFile fp);

  /** Write a float with correct endian and return as a float */
  static void
  writeFloat(gzFile fp, float w);

  /** Read up to length characters into a std::string */
  static std::string
  readChar(gzFile fp, size_t length);

  /** Write up to length characters from a std::string */
  static void
  writeChar(gzFile fp, std::string c, size_t length);

  /** Convenience method to read time, with optional predefined year */
  static Time
  readTime(gzFile fp, int year = -99);

  /** Convenience method to write time */
  static void
  writeTime(gzFile fp, const Time& time);

  /** Give back W2 info based on passed in HMRG */
  static bool
  HmrgToW2Name(const std::string& varName,
    std::string                 & outW2Name);

  /** Give back HMRGinfo based on passed in WG */
  static bool
  W2ToHmrgName(const std::string& varName,
    std::string                 & outHmrgName);

  /** Convert keys string to gzfile pointer in generic parameter passing */
  static gzFile
  keyToGZFile(std::map<std::string, std::string>& keys);

  /** Convert gzFile pointer to keys string in generic parameter passing */
  static void
  GZFileToKey(std::map<std::string, std::string>& keys, gzFile fp);

  // WRITING ------------------------------------------------------------

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    std::map<std::string, std::string>     & keys
  ) override;

  virtual
  ~IOHmrg();

  /** The database of product info for conversion stuff */
  static ProductInfoSet theProductInfos;
};
}

// Code readablity...
#define ERRNO(e) \
  { { e; } if ((errno != 0)) { throw ErrnoException(errno, # e); } }
