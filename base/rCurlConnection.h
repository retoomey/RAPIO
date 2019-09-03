#pragma once

#include <rIO.h>

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

private:

  /** Global curl initialize code */
  CURLcode myCurlCode;

  /** Used to access a curll connection */
  CURL * myCurl;

  /** Are we initialized? */
  bool myCurlInited;
};
}
