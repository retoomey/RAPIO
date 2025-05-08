#pragma once

#include <rNetwork.h>

// --------------------------------------------------------------
// Optional class for networking if CURL installed.
//
// With oracle/redhat distros currently, libcurl-netcdf goes
// alone with netcdf-devel so it's not an issue, but some distros
// don't have libcurl by default.  Plus since we require boost
// might as well have boost networking as an option.
#ifdef HAVE_CURL

extern "C" {
# include <curl/curl.h>
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
    std::vector<char>& buf) override;

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
}
#endif // ifdef HAVE_CURL
