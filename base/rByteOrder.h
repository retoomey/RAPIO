#pragma once

#include <rIO.h>
#include <algorithm>

namespace rapio {
/**
 * ByteOrder allows you to have a portable character representation
 * of your class across architechtures. The BOStream class provides
 * a better interface to this class.
 *
 * @see BOStream
 */
class ByteOrder : public IO {
  bool mySwapNeeded;

public:

  /**
   * Instantiate a ByteOrder object.
   * @param endian true if the file's bytes are little endian
   */
  ByteOrder(bool littleEndianFile = true)
  {
    const int x = 1;
    const bool littleEndianMachine = !(*(char *) &x == 1);

    mySwapNeeded = (littleEndianMachine != littleEndianFile);
  }

  /**
   * Swap (in place) the variable passed in
   * iff the file's endianness differs from this machine's.
   */
  void
  swapIfNeeded(char * startVal, size_t numChars) const
  {
    if (mySwapNeeded) { swapAlways(startVal, numChars); }
  }

  /**
   * Unconditionally swaps (in place) the variable passed in.
   * Useful when you have to swap regardless of the byteorder.
   */
  static void
  swapAlways(char * startVal, size_t numChars)
  {
    std::reverse(startVal, startVal + numChars);
  }
};
}
