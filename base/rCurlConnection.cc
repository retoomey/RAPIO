#include "rCurlConnection.h"

#include "rError.h"
#include "rStrings.h"

#include <curl/curl.h>

using namespace rapio;

bool CurlConnection::TRY_CURL  = true;
bool CurlConnection::GOOD_CURL = false;
std::shared_ptr<CurlConnection> CurlConnection::myCurlConnection;

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
// Write to a std::vector<char>
size_t
processData(void * ptr, size_t sz, size_t n, void * obj)
{
  std::vector<char> * data = (std::vector<char> *) obj;
  char * start = (char *) ptr;
  char * end   = start + sz * n;

  data->insert(data->end(), start, end);
  return (end - start);
}

/* Write to a std:;string
 * size_t write(void *contents, size_t size, size_t nmemb, void *userp)
 * {
 *  ((std::string*)userp)->append((char*)contents, size * nmemb);
 *  return size * nmemb;
 * }
 */
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

int
CurlConnection::read(const std::string& url, std::vector<char>& buf)
{
  // Lazy init of global curl

  if (!GOOD_CURL) { // 99.999% skip
    // Try to set up curl if we haven't tried
    if (TRY_CURL) {
      if (myCurlConnection == nullptr) {
        myCurlConnection = std::make_shared<CurlConnection>();
      }
      if (!(myCurlConnection->isValid() && myCurlConnection->isInit())) {
        LogInfo("Initializing curl for remote request ability...\n");

        if (myCurlConnection->lazyInit()) {
          LogInfo("...curl initialized.\n");
          GOOD_CURL = true;
        }
      } else {
        LogInfo("...curl failed to initialize.\n");
        return (-1); // Curl failed :(
      }
      TRY_CURL = false; // Don't try again
    } else {
      LogSevere("Can't read remote URL without curl\n");
      return (-1);
    }
  }

  URL urlb      = url;
  std::string p = urlb.getPath();

  Strings::replace(p, "//", "/");
  urlb.setPath(p);

  std::string urls = urlb.toString();
  CURLcode ret     = curl_easy_setopt(
    CurlConnection::myCurlConnection->connection(), CURLOPT_URL, urls.c_str());

  if (ret != 0) {
    LogSevere("Opening " << urls << " failed with err=" << ret << "\n");
    return (-1);
  }
  curl_easy_setopt(CurlConnection::myCurlConnection->connection(), CURLOPT_WRITEDATA, &(buf));
  ret = curl_easy_perform(CurlConnection::myCurlConnection->connection());

  if (ret != 0) {
    LogSevere("Reading " << url << " failed with err=" << ret << "\n");
    return (-1);
  }
  return (buf.size());
} // CurlConnection::read

int
CurlConnection::read1(const std::string& url, std::vector<char>& buf)
{
  // Initialize curl...
  curl_global_init(CURL_GLOBAL_ALL);
  CURL * curlHandle = curl_easy_init();

  // Option set up
  // ok = ok && curl_easy_setopt(curl, CURLOPT_VERBOSE, 10) == 0;
  // ok = ok &&
  //  curl_easy_setopt(connection(), CURLOPT_WRITEFUNCTION, processData) == 0;
  // ok = ok && curl_easy_setopt(connection(), CURLOPT_NOSIGNAL, 1) == 0;
  // ok = ok && curl_easy_setopt(connection(), CURLOPT_TIMEOUT, 30) == 0; // 30
  //                                                                     // seconds
  // ok = ok && curl_easy_setopt(connection(), CURLOPT_CONNECTTIMEOUT, 30) == 0;
  CURLcode ret = curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());

  if (ret != 0) {
    LogSevere("Opening " << url << " failed with err=" << ret << "\n");
    return (-1);
  }

  // Read
  curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &(buf));
  ret = curl_easy_perform(curlHandle);
  if (ret != 0) {
    LogSevere("Reading " << url << " failed with err=" << ret << "\n");
    return (-1);
  }

  // Cleanup
  curl_easy_cleanup(curlHandle);
  curl_global_cleanup();

  return (buf.size());
} // CurlConnection::read1

int
CurlConnection::get1(const std::string& url, const std::vector<std::string>& headers,
  std::vector<char>& buf)
{
  // Initialize curl...
  curl_global_init(CURL_GLOBAL_ALL);
  CURL * curlHandle = curl_easy_init();

  // Initialize headers
  curl_slist * httpHeaders = NULL;

  for (auto h:headers) {
    httpHeaders = curl_slist_append(httpHeaders, h.c_str());
    if (httpHeaders == NULL) {
      // error, return...
    }
  }

  // Execute
  // std::string output = "";
  if (curlHandle) {
    // Can reuse handle and settings I think, but how to do cleanly for different purposes
    curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1);                   // Set automaticallly redirection
    curl_easy_setopt(curlHandle, CURLOPT_MAXREDIRS, 1);                        // Set max redirection times
    curl_easy_setopt(curlHandle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1); // Set http version 1.1fer
    curl_easy_setopt(curlHandle, CURLOPT_HEADER, true);                        // Set header true
    curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, httpHeaders);             // Set headers

    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str()); // Set URL

    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, processData);
    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &buf);

    curl_easy_perform(curlHandle); // Perform

    if (httpHeaders != NULL) {
      curl_slist_free_all(httpHeaders);
    }
    curl_easy_cleanup(curlHandle);
    curl_global_cleanup();
    return (buf.size());
  }

  return -1;
} // CurlConnection::get1

int
CurlConnection::put1(const std::string& url, const std::vector<std::string>& headers,
  const std::vector<std::string>& body)
{
  // Initialize curl...

  /*  curl_global_init(CURL_GLOBAL_ALL);
   * CURL* curlHandle = curl_easy_init();
   *
   * // Initialize headers
   * curl_slist* httpHeaders = NULL;
   * for(auto h:headers)
   * {
   *   httpHeaders = curl_slist_append(httpHeaders, h.c_str());
   * }
   */

  /*
   *  // Intialize body
   *  string file = body[0];
   *  FILE * fd = fopen(file.c_str(), "rb"); // open file to upload
   *  struct stat file_info;
   *  fstat(fileno(fd), &file_info);
   *
   *  // Execute
   *  std::string output = "";
   *  if(curlHandle)
   *  {
   *      curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION,1);//Set automaticallly redirection
   *      curl_easy_setopt(curlHandle, CURLOPT_MAXREDIRS,1);//Set max redirection times
   *      curl_easy_setopt(curlHandle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);//Set http version 1.1fer
   *      curl_easy_setopt(curlHandle, CURLOPT_HEADER, true);//Set header true
   *      curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, httpHeaders);//Set headers
   *      curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());//Set URL
   *      curl_easy_setopt(curlHandle, CURLOPT_TRANSFER_ENCODING, 1L);
   *      curl_easy_setopt(curlHandle, CURLOPT_CUSTOMREQUEST, "PUT");
   *      curl_easy_setopt(curlHandle, CURLOPT_UPLOAD, 1L);
   *      curl_easy_setopt(curlHandle, CURLOPT_READDATA, fd);
   *      curl_easy_setopt(curlHandle, CURLOPT_READFUNCTION, reader);
   *      curl_easy_setopt(curlHandle, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_info.st_size);
   *      curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, write);
   *      curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &output);
   *
   *      result = curl_easy_perform(curlHandle);//Perform
   *      curl_global_cleanup();
   *
   *      return true;
   *      return (result == CURLE_OK);
   *  }
   */
  return -1;
}
