#pragma once

#include "rIODataType.h"

#include "rHmrgProductInfo.h"
#include "rIO.h"
#include "rOS.h"

#include <zlib.h>

namespace rapio {
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
  fromHmrgValue(short int v, const int dataUnavailable, const int dataMissing,
    const float dataScale)
  {
    float out;

    ON_BIG_ENDIAN(OS::byteswap(v));
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
  toHmrgValue(float v, const int dataUnavailable, const int dataMissing,
    const float dataScale)
  {
    short int out;

    if (v == Constants::DataUnavailable) {
      out = dataUnavailable;
    } else if (v == Constants::MissingData) {
      out = dataMissing;
    } else {
      out = v * dataScale;
      ON_BIG_ENDIAN(OS::byteswap(out));
    }
    return out;
  }

  /** What we consider a valid year in the MRMS binary file,
   * used for validation of datatype */
  static bool
  isMRMSValidYear(int year);

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
