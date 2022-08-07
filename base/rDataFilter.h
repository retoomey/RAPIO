#pragma once

#include <rIO.h>

#include <memory>
#include <vector>
#include <map>

#define RAPIO_USE_SNAPPY 0

namespace rapio {
class URL;

/** General class which allows filtering of data from input to output
 * based on a key factory name, and possibly reversing that
 * action.
 *
 * Compression, decompression, base64, encryption, lots of possibilities
 * here.
 *
 * @author Robert Toomey
 */
class DataFilter : public IO {
public:
  /** Default create a DataFilter */
  DataFilter(){ }

  // Registering of classes ---------------------------------------------

  /** Introduce self into factories */
  static void
  introduceSelf();

  /** Use this to introduce new subclasses. */
  static void
  introduce(const std::string   & protocol,
    std::shared_ptr<DataFilter> factory);

  /** Returns the data filter for this key */
  static std::shared_ptr<DataFilter>
  getDataFilter(const std::string& type);

  // Filter methods -----------------------------------------------------

  /** Take input character and apply filter creating output characters,
   * a copy is made. */
  virtual bool
  apply(std::vector<char>& input, std::vector<char>& output)
  {
    return false;
  }

  /** Apply filter to a given URL, write to output location */
  virtual bool
  applyURL(const URL& infile, const URL& outfile,
    std::map<std::string, std::string> &params){ return false; }
};

// ----------------------------------------------------------------------
// These classes are pretty simple not gonna bother with own class files
// for now at least

/** Simple GZIP filter for compressing to GZIP and uncompressing from GZIP
 * data.
 */
class GZIPDataFilter : public DataFilter {
public:
  /** Construct GZIP filter */
  GZIPDataFilter(){ }

  /** Introduce self into factories */
  static void
  introduceSelf();

  /** Take input character and apply filter creating output characters,
   * a copy is made. */
  virtual bool
  apply(std::vector<char>& input, std::vector<char>& output) override;

  /** Apply filter to a given URL, write to output location */
  virtual bool
  applyURL(const URL& infile, const URL& outfile,
    std::map<std::string, std::string> &params) override;
};

/** Simple BZIP2 filter for compressing to BZIP2 and uncompressing from BZIP2
 * data.
 */
class BZIP2DataFilter : public DataFilter {
public:
  /** Construct BZIP2 filter */
  BZIP2DataFilter(){ }

  /** Introduce self into factories */
  static void
  introduceSelf();

  /** Take input character and apply filter creating output characters,
   * a copy is made. */
  virtual bool
  apply(std::vector<char>& input, std::vector<char>& output) override;

  /** Apply filter to a given URL, write to output location */
  virtual bool
  applyURL(const URL& infile, const URL& outfile,
    std::map<std::string, std::string> &params) override;
};

/** Simple Zlib filter for compressing to Zlib and uncompressing from Zlib
 * data.
 */
class ZLIBDataFilter : public DataFilter {
public:
  /** Construct ZLIB filter */
  ZLIBDataFilter(){ }

  /** Introduce self into factories */
  static void
  introduceSelf();

  /** Take input character and apply filter creating output characters,
   * a copy is made. */
  virtual bool
  apply(std::vector<char>& input, std::vector<char>& output) override;

  /** Apply filter to a given URL, write to output location */
  virtual bool
  applyURL(const URL& infile, const URL& outfile,
    std::map<std::string, std::string> &params) override;
};

#if HAVE_LZMA

/** LZMA, XZ style
 */
class LZMADataFilter : public DataFilter {
public:
  /** Construct LZMA filter */
  LZMADataFilter(){ }

  /** Introduce self into factories */
  static void
  introduceSelf();

  /** Take input character and apply filter creating output characters,
   * a copy is made. */
  virtual bool
  apply(std::vector<char>& input, std::vector<char>& output) override;

  /** Apply filter to a given URL, write to output location */
  virtual bool
  applyURL(const URL& infile, const URL& outfile,
    std::map<std::string, std::string> &params) override;
};
#endif // if HAVE_LZMA

#if RAPIO_USE_SNAPPY == 1

/** Google snappyfilter for compressing/uncompressing snappy.
 */
class SnappyDataFilter : public DataFilter {
public:
  /** Construct GZIP filter */
  SnappyDataFilter(){ }

  /** Introduce self into factories */
  static void
  introduceSelf();

  /** Take input character and apply filter creating output characters,
   * a copy is made. */
  virtual bool
  apply(std::vector<char>& input, std::vector<char>& output) override;

  /** Apply filter to a given URL, write to output location */
  virtual bool
  applyURL(const URL& infile, const URL& outfile,
    std::map<std::string, std::string> &params) override;
};
#endif // if RAPIO_USE_SNAPPY == 1
}
