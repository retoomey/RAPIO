#pragma once

#include <rIO.h>

#include <vector>

namespace rapio {
/**
 * This is an utility class that provides various compression and
 * uncompression strategies. Mostly wraps around boost for convenience.
 *
 * @author Robert Toomey
 */
class Compression : public IO {
public:

  // -----------------------------------------------------------
  // Decompression routines.

  /** Is suffix one of our supported file endings? */
  static bool
  suffixRecognized(const std::string& suffix);

  // FIXME: Could introduce a class here to 'add' compression ability, but
  // currently probably overkill

  /** Uncompress Gzip input to output, vectors should be different */
  static bool
  uncompressGzip(std::vector<char>& input, std::vector<char>& output);

  /** Uncompress Bzip2 input to output, vectors should be different */
  static bool
  uncompressBzip2(std::vector<char>& input, std::vector<char>& output);

  /** Uncompress Zlib input to output, vectors should be different */
  static bool
  uncompressZlib(std::vector<char>& input, std::vector<char>& output);

  /** Uncompress Lzma input to output, vectors should be different */
  static bool
  uncompressLzma(std::vector<char>& input, std::vector<char>& output);

  // -----------------------------------------------------------
  // Compression routines.  FIXME: make em if we want em
}
;
}
