#pragma once

#include <rIO.h>

#include <string>
#include <vector>

#include <curl/curl.h>

namespace rapio {
/**
 * Handle the CURL library setup and shutdown
 *
 * @author Robert Toomey
 */
class CurlConnection : public IO {
public:

  /** Create a curl connection */
  CurlConnection();

  /** Destroy curl connection */
  ~CurlConnection();

  /** Is this curl connection valid and ready to use? */
  bool
  isValid();

  /** Is this curl connection been initialized? */
  bool
  isInit();

  /** Get the curl connection for use */
  CURL *
  connection();

  /** Lazy initialize curl on first use */
  bool
  lazyInit();

  /** Static read once if able, return -1 on fail or size of buffer */
  static int
  read1(const std::string& url, std::vector<char>& buf);

  /** Static get once if able, return -1 on fail or size of buffer */
  static int
  get1(const std::string& url, const std::vector<std::string>& headers,
    std::vector<char>& buf);

  /** Static put once if able, return -1 on fail or size of buffer */
  static int
  put1(const std::string& url, const std::vector<std::string>& headers,
    const std::vector<std::string>& body);


private:

  /** Global curl initialize code */
  CURLcode myCurlCode;

  /** Used to access a curll connection */
  CURL * myCurl;

  /** Are we initialized? */
  bool myCurlInited;
};
}
