#pragma once

#include <rUtility.h>

#include <vector>

#include <cstdio>
#include <cstdint>
#include <iostream>
#include <unistd.h>
#include <iostream>
#include <fstream>


namespace rapio {
/**
 * Utilities for dealing with signatures, coding, hashes, etc.
 * with general portable security stuff.
 * Note of course with open source here this is only
 * as secure as your control over your source/binary
 * code.
 *
 * @author Robert Toomey
 */
class Secure : public Utility {
public:

  // Most of those uses for AWS REST call experiments

  /** Do signing using SHA256 */
  static std::string
  sign(const std::string& key, const std::string& message);

  /** Sha256 of a string */
  static std::string
  sha256(const std::string& str);

  /** Decode a binary base64 into ascii text */
  static std::vector<uint8_t>
  decode64(const std::string& input);

  /** Hex digest of bytes */
  static std::string
  hexdigest(const std::string& bytes);

  // Signature checking, etc.

  /** Take a publickey, a message and a signature (binary or base64),
   * and verify the message */
  static bool
  validateSigned(
    const std::string& publickey,
    const std::string& message,
    const std::string& signature,
    bool             base64);
};
}
