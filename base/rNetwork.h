#pragma once

#include <rIO.h>
#include <rUtility.h>

#include <string>
#include <vector>
#include <memory>

namespace rapio {
/** Network connection interface implementing different libraries.
 *
 * @author Robert Toomey
 */
class NetworkConnection : public IO {
public:
  /** Read a url */
  virtual int
  read(const std::string& url, std::vector<char>& buf) = 0;

  /** Read a url and pass extra HTTP headers */
  virtual int
  readH(const std::string& url, const std::vector<std::string>& headers,
    std::vector<char>& buf) = 0;
};

/**
 * Utils to handle network related things
 *
 * Creating framework to switch to ASIO over CURL
 * but this will require lots of testing so maintaining
 * the ability to swap out network handlers.
 *
 * @author Robert Toomey
 */
class Network : public Utility {
public:

  /** Set the network engine used such as CURL or BOOST,
   * on failure the current engine will remain unchanged. */
  static bool
  setNetworkEngine(const std::string& key);

  /** Static read URL ability into a buffer */
  inline static int
  read(const std::string& url, std::vector<char>& buf)
  {
    // This is quickest, but you must call setNetworkEngine
    // before calling to make sure myConnection is initialized
    return myConnection->read(url, buf);
  }

  /** Static read URL ability into a buffer with http headers */
  inline static int
  readH(const std::string& url, const std::vector<std::string>& headers,
    std::vector<char>& buf)
  {
    // This is quickest, but you must call setNetworkEngine
    // before calling to make sure myConnection is initialized
    return myConnection->readH(url, headers, buf);
  }

private:

  /** Current network handler */
  static std::shared_ptr<NetworkConnection> myConnection;
};
}
