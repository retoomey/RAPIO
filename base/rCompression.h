#pragma once

#include <rIO.h>
#include <stddef.h> // size_t

namespace rapio {
class Buffer;

/**
 * This is an utility class that provides various compression and
 * uncompression strategies.
 *
 */
class Compression : public IO {
public:

  /**
   *  Does in-memory compression similar to what is done by the
   *  UNIX executable bzip2 and returns the result. The
   *  result may be the uncompressed original if insufficient
   *  compression is obtained.
   *
   *  @param uncompr The data to compress
   *  @param factor A guess at a compression ratio. 1.2 is a good
   *  pessimistic guess (20% more)
   *  @param maxfactor The compression ratio beyond which we should
   *  not try to compress it.
   */
  static Buffer
  compressBZ2(const Buffer& uncompr,
    float                 factor = 1.2,
    float                 maxfactor = 2.0);


  /**
   *  Does in-memory compression similar to what is done by the
   *  UNIX executable gzip and returns the result.
   *
   *  @param uncompr The data to compress
   *  @param factor A guess at a compression ratio. 1.2 is a good
   *  pessimistic guess (20% more)
   *  @param maxfactor The compression ratio beyond which we should
   *  not try to compress it.
   *
   *  The underlying zlib library guarantees that these guesses will work,
   *  so there won't be a case where insufficient compression is obtained
   *  as long as you use the defaults.
   */
  static Buffer
  compressGZ(const Buffer& uncompr,
    float                factor = 1.2,
    float                maxfactor = 2.0);

  /**
   *  Does in-memory compression similar to what is done by the
   *  zlib library and returns the result.
   *
   *  @param uncompr The data to compress
   *  @param factor A guess at a compression ratio. 1.2 is a good
   *  pessimistic guess (20% more)
   *  @param maxfactor The compression ratio beyond which we should
   *  not try to compress it.
   *
   *  The underlying zlib library guarantees that these guesses will work,
   *  so there won't be a case where insufficient compression is obtained
   *  as long as you use the defaults.
   */
  static Buffer
  compressZlib(const Buffer& uncompr,
    float                  factor = 1.2,
    float                  maxfactor = 2.0);


  /**
   *  Does in-memory uncompression similar to what is done by the
   *  UNIX executable bunzip2 and returns the result.
   *
   *  @param input The data to uncompress
   */
  static
  Buffer
  uncompressBZ2(const Buffer& input);

  /**
   *  Does in-memory uncompression similar to what is done by the
   *  UNIX executable gunzip and returns the result.
   *
   *  @return original data if the data is not in gzip format.
   *
   *  @param input The data to uncompress
   */
  static
  Buffer
  uncompressGZ(const Buffer& input);

  /**
   *  Does in-memory uncompression similar to what is done by the
   *  zlib library and returns the result.
   *
   *  @return original data if the data is not in zlib format.
   *
   *  @param input The data to uncompress
   */
  static
  Buffer
  uncompressZlib(const Buffer& input);

private:

  /** utility function that implements both GZ and Zlib. */
  static Buffer
  compressGZ(const Buffer& uncompr,
    float                factor,
    float                maxfactor,
    size_t               HeaderSize);

  /** The output buffer should be primed to the right size. */
  static int
  uncompressZlibNoHeader(const Buffer& input,
    Buffer                           & output,
    size_t                           headerSize);
}
;
}
