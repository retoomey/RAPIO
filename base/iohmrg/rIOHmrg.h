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

  /** Do a full read from a param list */
  static std::shared_ptr<DataType>
  readRawDataType(const URL& path);

  /** Convert scaled compressed int to float.  Grouping the uncompression logic here inline,
   * this 'should' optimize in compiler to inline. */
  static inline float
  convertDataValue(short int v, const bool needSwap, const int dataUnavailable, const int dataMissing,
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

  /** What we consider a valid year in the MRMS binary file,
   * used for validation of datatype */
  static bool
  isMRMSValidYear(int year);

  /** Read a scaled integer with correct endian and return as a float */
  static float
  readScaledInt(gzFile fp, float scale);

  /** Read an integer with correct endian and return as an int */
  static int
  readInt(gzFile fp);

  /** Read a float with correct endian and return as a float */
  static int
  readFloat(gzFile fp);

  /** Read up to length characters into a std::string */
  static std::string
  readChar(gzFile fp, size_t length);

  /** Give back W2 info based on passed in HMRG */
  static bool
  HmrgToW2(const std::string& varName,
    std::string             & outW2Name);

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
