#pragma once

#include <rNetwork.h>

#include <rBOOST.h>

BOOST_WRAP_PUSH
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
BOOST_WRAP_POP

namespace rapio {
/** BOOST::asio network reader
 * FIXME: Currently alpha, has issues but have to start somewhere
 * The advantage to getting this to work will be not having a 100%
 * CURL requirement which isn't on every distro.
 *
 * @author Robert Toomey
 */
class BoostConnection : public NetworkConnection {
public:
  /** Initialize us with reusable context/resolver */
  BoostConnection() : myIOContext(), myResolver(myIOContext){ }

  /** Read a url */
  virtual int
  read(const std::string& url, std::vector<char>& buf) override;

  /** Read a url and pass extra HTTP headers */
  virtual int
  readH(const std::string& url, const std::vector<std::string>& headers,
    std::vector<char>& buf) override;

protected:

  /**
   * Read from open socket a given request and process it.
   *
   * Template the reading code since we either call a socket directly,
   * or call a wrapper with same methods/different class structure in
   * order to ssl encrypt/decrypt.  There doesn't appear to be any
   * common superclass.
   *  boost::asio::ip::tcp::socket and
   *  boost::asio::ssl::stream<boost::asio::ip::tcp::socket>
   *
   */
  template <typename T>
  int
  readOpenSocketT(T& socket, const std::string& request, std::vector<char>& buf, size_t& recursive);

  /** Read a url with a recursive level passed */
  int
  readLocal(const std::string& url, std::vector<char>& buf, size_t& recursive);

  /** Our io_context (reusable) */
  boost::asio::io_context myIOContext;

  /** Our resolver (name to ip lookup object) */
  boost::asio::ip::tcp::resolver myResolver;
};
}
