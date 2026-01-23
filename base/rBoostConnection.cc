#include "rBoostConnection.h"

#include "rError.h"
#include "rStrings.h"

#include <iostream>

namespace asio = boost::asio;
namespace ssl  = asio::ssl;

using namespace rapio;

namespace {
// Header compare (Also strings::prefix, but we'll inline here)
inline bool
inHeader(const std::string& in, const std::string& what)
{
  return (in.compare(0, what.size(), what) == 0);
}
}

int
BoostConnection::read(const std::string& url, std::vector<char>& buf)
{
  try{
    // Pass to local which can recursively redirect
    size_t level = 0;
    return readLocal(url, buf, level);
  }catch (const std::exception& e) {
    fLogSevere("Error reading {} {}", url, e.what());
  }
  return 0;
}

// Putting code here since we're private and only called from us.
template <typename T>
int
BoostConnection::readOpenSocketT(
  T& mySocket, const std::string& request, std::vector<char>& buf, size_t& recursive)
{
  // std::cout << "Made it to read open socket...\n";
  boost::asio::streambuf myRequest;
  boost::system::error_code err;

  // std::cout << "WRITE:'"<<request<<"'\n";
  boost::asio::write(mySocket, boost::asio::buffer(request), err);

  // Read headers to determine content size
  boost::asio::read_until(mySocket, myRequest, "\r\n\r\n", err);

  // ---------------------------------------------------
  // Extract the response headers
  //

  int status_code = -1;
  std::string http_version = "1.0";
  std::string content_type = "";
  std::istream response_stream(&myRequest);
  size_t contentLength = 0;

  // Read each line this consumes from the buffer (myRequest.size() drops)
  std::string header;

  while (std::getline(response_stream, header) && header != "\r") {
    // std::cout << ">>>HEADER:'"<<header<<"'\n";
    // HTTP version and status code
    if (inHeader(header, "HTTP/")) {            // HTTP/1.1 200 OK
      std::istringstream iss(header.substr(5)); // skip HTTP/
      iss >> http_version >> status_code;
    }

    // All other http headers have form key:value
    size_t colonPos = header.find(':');

    if (colonPos != std::string::npos) {
      // Make the key lower case always (or we need case-i compare)
      std::transform(header.begin(), header.begin() + colonPos, header.begin(), ::tolower);

      // Content type
      if (inHeader(header, "content-type: ")) {
        std::istringstream iss(header.substr(13)); // skip HTTP/
        iss >> content_type;
        // Resize the buffer to the expected size
      }

      // Content length
      if (inHeader(header, "content-length: ")) {
        // Resize the buffer to the expected size
        // We use the content length in the header to size our buffer,
        // avoiding having to copy the boost one
        contentLength = std::stoull(header.substr(16));
      }

      if (inHeader(header, "location: ")) {
        std::istringstream iss(header.substr(10)); // skip HTTP/
        std::string newlocation;
        iss >> newlocation;
        // std::cout << "Redirecting to :"<<newlocation <<"\n";
        return (this->readLocal(newlocation, buf, recursive));
      }

      if (inHeader(header, "server: ")) { }
      if (inHeader(header, "date: ")) { }
    } else {
      // Not sure what we are...
    }
  }

  // ---------------------------------------------------
  // Get the data into the buffer
  //
  // std::cout << ">>>>>STATUS CODE:'"<<status_code <<"'\n";
  // std::cout << ">>>>>HTTP VERSION:'"<<http_version <<"'\n";
  // std::cout << ">>>>>Content-Type:'"<<content_type <<"'\n";
  // std::cout << ">>>>>Content length:" << contentLength << "\n";

  if (true) { // full copy read (safest at moment)
    // Full copy.  Can be bad for large stuff.  Leaving here for debugging later if needed
    // Note, since using same myRequest as headers, the leftover is auto handled
    boost::asio::read(mySocket, myRequest, err);
    buf.resize(boost::asio::buffer_size(myRequest.data()));
    std::copy(boost::asio::buffers_begin(myRequest.data()), boost::asio::buffers_end(myRequest.data()), buf.begin());
  } else {
    // Partial copy?  Need buffer class I think...
    // The issue at moment with this technique is that a lot of times we don't get a content-length header
    // so we can crash...
    if (contentLength > 0) {
      buf.resize(contentLength);
      // Copy the extra bytes-transferred of the boost buffer stream into our vector (like 128) or so
      // it's typically a small block
      size_t leftOver = myRequest.size();
      std::copy(boost::asio::buffers_begin(myRequest.data()), boost::asio::buffers_end(myRequest.data()), buf.begin());

      // Just a wrapper to our vector that doesn't copy, offset by bytes left to read
      boost::asio::mutable_buffer start = boost::asio::mutable_buffer(&buf[leftOver], buf.size() - leftOver);
      // Now read the rest, placing further into the vector
      boost::asio::read(mySocket, start, err);
    } else {
      fLogSevere("No content-length header?");
    }
  }

  return buf.size();
} // BoostConnection::readOpenSocketT

int
BoostConnection::readLocal(const std::string& url, std::vector<char>& buf, size_t& recursive)
{
  // Recursive limit for any redirects (avoid infinity)
  recursive++;
  if (recursive > 2) {
    fLogSevere("Redirect count too high for URL: {}", recursive);
    return 0;
  }

  URL aURL(url); // parse the url

  // Support http or https only for this next round
  // FIXME: Think if we build without openssl than https breaks?
  // need to check and add conditionals
  const std::string scheme = aURL.getScheme();

  if (!((scheme == "http") || (scheme == "https"))) {
    // FIXME: could we get ftp working maybe or others?
    std::cout << "Unimplemented url scheme '" << scheme << "\n";
    return 0;
  }

  // std::cout << ">>>URL IS " << aURL.toString() << "\n";
  // std::cout << ">>>SCHEME IS " << scheme << "\n";

  // ---------------------------------------------------
  // Host resolve to ip list based on scheme and host name
  //
  boost::asio::ip::tcp::resolver::query query(aURL.getHost(), scheme);
  boost::system::error_code err;
  auto endpoints = myResolver.resolve(query, err);
  boost::asio::ip::tcp::resolver::iterator end; // lists of ip:ports

  if (err) {
    fLogSevere("ERROR resolving host: {}", err.message());
    return 0;
  }
  #if 0
  // NOTE: iterating moves this so connect fails
  while (endpoints != end) {
    boost::asio::ip::tcp::endpoint endpoint = *endpoints++;
    std::cout << "ENDPOINT: " << endpoint << "\n";
  }
  #endif

  // ---------------------------------------------------
  // Create the request
  //
  // FIXME: Might need more work on paths the local/absolute, etc.
  // Our URL getPath can return empty, but get requires at least a /
  // FIXME: Should our URL getPath always returns at least a "/"?
  // So currently we have a double // which seems to work
  // std::string request = "GET /"+aURL.getPath()+" HTTP/1.1\r\n"
  // std::string request = "GET /" + aURL.toGetString() + " HTTP/1.1\r\n"
  std::string request = "GET " + aURL.toString() + " HTTP/1.1\r\n"
    + "Host: " + aURL.getHost() + "\r\n"
    + "Connection: close" + "\r\n"
    + "\r\n";
  // std::cout << "REQUEST IS:'" << request << "'\n";

  // ---------------------------------------------------
  // Open socket to http or https and get the data
  // The ssl requires a wrapper around the socket that handles
  // the encryption
  if (scheme == "https") {
    // Special for ssl connection
    ssl::context ctx(ssl::context::sslv23_client);
    ssl::stream<asio::ip::tcp::socket> sslSocket(myIOContext, ctx); // wrapper
    asio::connect(sslSocket.next_layer(), endpoints, err);          // next_layer = socket

    sslSocket.handshake(ssl::stream_base::client);

    // std::cout << ">>>IN HTTPS\n";
    // Use to do work...
    return (readOpenSocketT(sslSocket, request, buf, recursive));
  } else if (scheme == "http") {
    // 'Normal' non-ssl connection
    asio::ip::tcp::socket non_ssl_socket(myIOContext);
    asio::connect(non_ssl_socket, endpoints, err);

    // std::cout << ">>>IN HTTP\n";
    // Use to do work...
    return (readOpenSocketT(non_ssl_socket, request, buf, recursive));
  }

  return 0;
} // BoostConnection::read

int
BoostConnection::readH(const std::string& url, const std::vector<std::string>& headers,
  std::vector<char>& buf)
{
  fLogSevere("Not implemented boost HTTP headers.");
  return read(url, buf); // ignore extra headers for moment
}
