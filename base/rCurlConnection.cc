#include "rCurlConnection.h"

#include "rError.h"
#include "rStrings.h"

// --------------------------------------------------------------
// Optional class for networking if CURL installed.
//
#ifdef HAVE_CURL

extern "C" {
# include <curl/curl.h>
}

using namespace rapio;

namespace {
/** Callback for CURL, write to a std::vector<char> */
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

CurlConnection::CurlConnection()
{
  if (!init()) {
    throw std::runtime_error("CURL couldn't initialize!");
  }
}

bool
CurlConnection::init()
{
  // Global init of curl and a curl connection
  myCurlCode = curl_global_init(CURL_GLOBAL_ALL);
  myCurl     = 0;

  bool ok = (myCurlCode == 0);

  if (!ok) {
    fLogSevere("Unable to initialize libcurl properly");
  } else {
    // Now our instance for requesting
    myCurl = curl_easy_init();

    ok = (myCurl != 0);
    if (ok) {
      // Settings used
      // ok = ok && curl_easy_setopt(curl, CURLOPT_VERBOSE, 10) == 0;
      ok = ok && curl_easy_setopt(myCurl, CURLOPT_WRITEFUNCTION, processData) == 0;
      ok = ok && curl_easy_setopt(myCurl, CURLOPT_NOSIGNAL, 1) == 0;
      ok = ok && curl_easy_setopt(myCurl, CURLOPT_TIMEOUT, 30) == 0; // 30 seconds
      ok = ok && curl_easy_setopt(myCurl, CURLOPT_CONNECTTIMEOUT, 30) == 0;
    }
  }
  return ok;
}

CurlConnection::~CurlConnection()
{
  if (myCurl != 0) {
    curl_easy_cleanup(myCurl);
  }

  if (myCurlCode == 0) {
    curl_global_cleanup();
  }
}

int
CurlConnection::read(const std::string& url, std::vector<char>& buf)
{
  URL urlb      = url; // recreation from URL.  FIXME:
  std::string p = urlb.getPath();

  Strings::replace(p, "//", "/");
  urlb.setPath(p);

  std::string urls = urlb.toString();
  CURLcode ret     = curl_easy_setopt(
    myCurl, CURLOPT_URL, urls.c_str());

  if (ret != 0) {
    fLogSevere("Opening {} failed with err={}", urls, (int) ret);
    return (-1);
  }
  curl_easy_setopt(myCurl, CURLOPT_WRITEDATA, &(buf));
  ret = curl_easy_perform(myCurl);

  if (ret != 0) {
    fLogSevere("Reading {} failed with err={}", urls, (int) ret);
    return (-1);
  }
  return (buf.size());
}

int
CurlConnection::readH(const std::string& url, const std::vector<std::string>& headers,
  std::vector<char>& buf)
{
  if (myCurl) {
    // Initialize headers
    curl_slist * httpHeaders = NULL;

    for (auto h:headers) {
      httpHeaders = curl_slist_append(httpHeaders, h.c_str());
    }

    // Set header options
    if (httpHeaders != NULL) {
      curl_easy_setopt(myCurl, CURLOPT_HEADER, true);            // Set header true
      curl_easy_setopt(myCurl, CURLOPT_HTTPHEADER, httpHeaders); // Set headers
    }

    curl_easy_setopt(myCurl, CURLOPT_FOLLOWLOCATION, 1);                   // Set automatic redirection
    curl_easy_setopt(myCurl, CURLOPT_MAXREDIRS, 1);                        // Set max redirection times
    curl_easy_setopt(myCurl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1); // Set http version 1.1fer
    curl_easy_setopt(myCurl, CURLOPT_URL, url.c_str());                    // Set URL

    curl_easy_setopt(myCurl, CURLOPT_WRITEFUNCTION, processData);
    curl_easy_setopt(myCurl, CURLOPT_WRITEDATA, &buf);

    // Run CURL request
    curl_easy_perform(myCurl); // Perform

    if (httpHeaders != NULL) {
      curl_slist_free_all(httpHeaders);
    }
    return (buf.size());
  }
  return -1;
} // CurlConnection::read1

# if 0
NOTE: Might work on this again sometime, so holding onto it
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

# endif // if 0
#endif  // ifdef HAVE_CURL
