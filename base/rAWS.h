#pragma once

#include <string>

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
 * @ingroup rapio_utility
 * @brief Some AWS experiments without AWS c++ API
 */
class AWS {
public:
  // -----------------------------------------------
  // REST experimental interface stuff
  // Basically just translating the AWS python example given.

  /** Do AWS4 REST signature */
  static std::string
  getSignatureKey(
    const std::string& key,
    const std::string& dateStamp,
    const std::string& regionName,
    const std::string& serviceName);

  /** Test REST get method */
  static void
  test_REST_API_Get(const std::string& access_key,
    const std::string                & secret_key);
};
}
