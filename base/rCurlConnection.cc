#include "rCurlConnection.h"

#include "rError.h"
#include "rStrings.h"

extern "C" {
#include <curl/curl.h>
}

#include <boost/asio.hpp>

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
    LogSevere("Unable to initialize libcurl properly \n");
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
    LogSevere("Opening " << urls << " failed with err=" << ret << "\n");
    return (-1);
  }
  curl_easy_setopt(myCurl, CURLOPT_WRITEDATA, &(buf));
  ret = curl_easy_perform(myCurl);

  if (ret != 0) {
    LogSevere("Reading " << urls << " failed with err=" << ret << "\n");
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

#if 0
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

#endif // if 0

int
BoostConnection::read(const std::string& url, std::vector<char>& buf)
{
  // ALPHA ---
  // This code is not working correctly in all cases...

  // Convert back which is kinda silly. IOURL should send the URL I think...
  // or we create two methods
  URL aURL(url);

  // Should these be done 'once' I think so?
  boost::asio::io_context myIOContext;
  boost::asio::ip::tcp::resolver myResolver(myIOContext);
  // How is the socket reused?  Is this ok?
  boost::asio::ip::tcp::socket mySocket(myIOContext);
  boost::asio::streambuf myRequest;

  // Resolve query to endpoints
  std::cout << "URL: " << aURL.getHost() << "\n";

  boost::asio::ip::tcp::resolver::query query(aURL.getHost(), "80");
  boost::system::error_code err;
  auto endpoints = myResolver.resolve(query, err);
  boost::asio::ip::tcp::resolver::iterator end; // lists of ip:ports

  #if 0
  // NOTE: iterating moves this so connect fails
  while (endpoints != end) {
    boost::asio::ip::tcp::endpoint endpoint = *endpoints++;
    std::cout << "ENDPOINT: " << endpoint << "\n";
  }
  #endif

  if (err) {
    std::cout << "ERROR resolving host: " << err.message() << "\n";
    return -1;
  }

  // Connect to the remote data server(s)
  // boost::asio::connect(mySocket, endpoints);
  boost::asio::connect(mySocket, endpoints, err);

  // Our URL getPath can return empty, but get requires at least a /
  // FIXME: Should our URL getPath always returns at least a "/"?
  // So currently we have a double // which seems to work
  // std::string request = "GET /"+aURL.getPath()+" HTTP/1.1\r\n"
  std::string request = "GET /" + aURL.toGetString() + " HTTP/1.1\r\n"
    + "Host: " + aURL.getHost() + "\r\n"
    + "Connection: close" + "\r\n"
    + "\r\n";

  std::cout << "REQUEST IS:'" << request << "'\n";

  boost::asio::write(mySocket, boost::asio::buffer(request), err);

  // Read headers to determine content size
  boost::asio::read_until(mySocket, myRequest, "\r\n\r\n", err);

  // Extract the response headers
  std::istream response_stream(&myRequest);

  // We use the content length in the header to size our buffer,
  // avoiding having to copy the boost one
  std::string header;
  bool firstHeader = true;
  // Read each line this consumes from the buffer (myRequest.size() drops)
  while (std::getline(response_stream, header) && header != "\r") {
    // std::cout << "HEADERBACK:"<<header<<" Leftover is" << myRequest.size() << "\n";

    // FIXME: If we get a Content-Encoding things will break upstream probably...

    // Check for the Content-Length header
    if (header.compare(0, 16, "Content-Length: ") == 0) {
      // Resize the buffer to the expected size
      size_t contentLength = std::stoull(header.substr(16));
      buf.resize(contentLength);
      // Tempted to skip out, the problem is we read more than the header, so we
      // need to consume all the header items to avoid copying anything into our
      // body.
      // break;
    }
  }

  // Full copy.  Can be bad for large stuff.  Leaving here for debugging later if needed
  // Note, since using same myRequest as headers, the leftover is auto handled
  // boost::asio::read(mySocket, myRequest, err);
  // buf.resize(boost::asio::buffer_size(myRequest.data()));
  // std::copy(boost::asio::buffers_begin(myRequest.data()), boost::asio::buffers_end(myRequest.data()), buf.begin());
  // return buf.size();

  // Copy the extra bytes-transferred of the boost buffer stream into our vector (like 128) or so
  // it's typically a small block
  size_t leftOver = myRequest.size();
  std::copy(boost::asio::buffers_begin(myRequest.data()), boost::asio::buffers_end(myRequest.data()), buf.begin());

  // Just a wrapper to our vector that doesn't copy, offset by bytes left to read
  boost::asio::mutable_buffer start = boost::asio::mutable_buffer(&buf[leftOver], buf.size() - leftOver);
  // Now read the rest, placing further into the vector
  boost::asio::read(mySocket, start, err);
  return buf.size();
} // BoostConnection::read

int
BoostConnection::readH(const std::string& url, const std::vector<std::string>& headers,
  std::vector<char>& buf)
{
  LogSevere("Not implemented boost HTTP headers.\n");
  return read(url, buf); // ignore extra headers for moment
}
