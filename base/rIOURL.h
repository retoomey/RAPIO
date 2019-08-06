#pragma once

#include <rIO.h>
#include <rURL.h>
// #include <rIOXML.h>

#include <string>
#include <rBuffer.h>
#include <memory>

namespace rapio {
/**
 * A framework to simplify URL reading.
 */
class IOURL : public IO {
public:

  /**
   * Convenience function to read `url' into `buf'.
   * Handles remote, local, compressed, and uncompressed files transparently.
   *
   * @param url the data source.
   * @param buffer where source's contents will be stored.
   * @return the number of bytes read, or a negative number on error.
   */
  static int
  read(const URL& url,
    Buffer      & buffer);

  /**
   * Identical to read() except except it doesn't uncompress files.
   * This is useful when mirroring compressed files to avoid
   * decompress / recompress steps.
   */
  static int
  readRaw(const URL& url,
    Buffer         & b);

  /**
   * Identical to read(const URL&,Buffer&,long,size_t) except it takes a string
   * instead of a buffer.
   */
  static int
  read(const URL & url,
    std::string  & s);


  /** Destroy a reader */
  virtual ~IOURL(){ }

protected:

  /** Try to initialize curl on first call to a remote data file */
  static bool TRY_CURL;

  /** Set to true when curl initialization is successfull */
  static bool GOOD_CURL;
};
}
