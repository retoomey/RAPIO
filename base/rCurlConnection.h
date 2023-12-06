#pragma once

#include <rNetwork.h>

extern "C" {
#include <curl/curl.h>
}

namespace rapio {
/** CURL based network reader
 * Sometimes the simple old C libraries just work.
 *
 * @author Robert Toomey
 */
class CurlConnection : public NetworkConnection {
public:

  /** Create a curl connection */
  CurlConnection();

  /** Destroy curl connection */
  ~CurlConnection();

  /** Read a url */
  virtual int
  read(const std::string& url, std::vector<char>& buf) override;

  /** Read a url and pass extra HTTP headers */
  virtual int
  readH(const std::string& url, const std::vector<std::string>& headers,
    std::vector<char>& buf);

  /** Put a url and pass extra HTTP headers */
  // virtual int
  // put1(const std::string& url, const std::vector<std::string>& headers,
  //   const std::vector<std::string>& body);

private:

  /** Initialize CURL */
  bool
  init();

  /** Global CURL initialize code */
  CURLcode myCurlCode;

  /** Used to access a CURL connection */
  CURL * myCurl;
};

/** BOOST::asio network reader
 * FIXME: Currently alpha, has issues but have to start somewhere
 * The advantage to getting this to work will be not having a 100%
 * CURL requirement which isn't on every distro.
 *
 * @author Robert Toomey
 */
class BoostConnection : public NetworkConnection {
public:

  /** Read a url */
  virtual int
  read(const std::string& url, std::vector<char>& buf) override;

  /** Read a url and pass extra HTTP headers */
  virtual int
  readH(const std::string& url, const std::vector<std::string>& headers,
    std::vector<char>& buf);
};
}
