#include "rCurlConnection.h"

#include "rError.h"

#include <curl/curl.h>

using namespace rapio;

CurlConnection::CurlConnection()
{
  myCurl       = 0;
  myCurlInited = false;
  myCurlCode   = curl_global_init(CURL_GLOBAL_ALL);

  bool success = (myCurlCode != 0);

  if (success) {
    LogSevere("Unable to initialize libcurl properly \n");
  } else {
    myCurl = curl_easy_init();
  }
}

namespace {
size_t
processData(void * ptr, size_t sz, size_t n, void * obj)
{
  std::vector<char> * data = (std::vector<char> *)obj;
  char * start = (char *) ptr;
  char * end   = start + sz * n;
  data->insert(data->end(), start, end);
  return (end - start);
}
}

CurlConnection::~CurlConnection()
{
  if (myCurl != 0) {
    curl_easy_cleanup(myCurl);
  }

  if (isValid()) {
    curl_global_cleanup();
  }
}

bool
CurlConnection::isValid()
{
  return (myCurlCode == 0);
}

bool
CurlConnection::isInit()
{
  return (myCurlInited);
}

CURL *
CurlConnection::connection()
{
  return (myCurl);
}

bool
CurlConnection::lazyInit()
{
  bool ok = (myCurlCode == 0);

  if (!ok) {
    LogSevere("Unable to initialize libcurl properly \n");
    return (false);
  }

  // ok = ok && curl_easy_setopt(curl, CURLOPT_VERBOSE, 10) == 0;
  ok = ok &&
    curl_easy_setopt(connection(), CURLOPT_WRITEFUNCTION, processData) == 0;
  ok = ok && curl_easy_setopt(connection(), CURLOPT_NOSIGNAL, 1) == 0;
  ok = ok && curl_easy_setopt(connection(), CURLOPT_TIMEOUT, 30) == 0; // 30
                                                                       // seconds
  ok = ok && curl_easy_setopt(connection(), CURLOPT_CONNECTTIMEOUT, 30) == 0;

  if (!ok) {
    LogSevere("Unable to initialize libcurl library.\n");
    curl_easy_cleanup(connection());
    return (false);
  }

  myCurlInited = true;
  return (true);
}
