#pragma once

#include <rUtility.h>

namespace rapio {
/**
 * A utility wrapping for some simple aws calls.
 * Mostly wrapping around aws cli properly installed
 * on a system with either role or development/user
 * credentials.
 * Note: At some point could see building/linking
 * the AWS C++ sdk, but our current operational
 * setup doesn't have the cmake version requirements
 *
 * So for now I'll keep this as I experiment with
 * using various AWS services.
 *
 * @author Robert Toomey
 */
class AWS : public Utility {
public:
  // -----------------------------------------------
  // Stupid simple we'll wrap aws cli calls.  The good
  // thing is it will handle roles and STS for us,
  // bad is that this way is slower than snails I'm sure.

  // -----------------------------------------------
  // REST experimental interface stuff
  // Basically just translating the AWS python example given.

  /** Do signing using SHA256 */
  static std::string
  sign(const std::string& key, const std::string& message);

  /** Do AWS4 REST signature */
  static std::string
  getSignatureKey(
    const std::string& key,
    const std::string& dateStamp,
    const std::string& regionName,
    const std::string& serviceName);

  /** Sha256 of a string */
  static std::string
  sha256(const std::string str);

  /** Hex digest of bytes */
  static std::string
  hexdigest(const std::string bytes);

  /** Test REST get method */
  static void
  test_REST_API_Get(const std::string& access_key,
    const std::string                & secret_key);
};
}
