#pragma once

#include <map>
#include <string>

namespace rapio {
/**
 * A class for parsing, accessing, changing, and recombining a URL's components.
 *
 * <b>Anatomy of a URL</b>\n
 * The URL
 *<tt>http://joe:blow@example.com:81/loc/script.php?var=val&foo=bar#here</tt>\n
 * has the following components, all of which correspond to fields in the URL
 * class:
 * \li\c scheme (http)
 * \li\c user (joe)
 * \li\c pass (blow)
 * \li\c host (example.com)
 * \li\c port (81)
 * \li\c path (/loc/script.php)
 * \li\c query (query["var"] == "val"; query["foo"] == "bar")
 * \li\c fragment (here)
 *
 * @class URL
 * @ingroup rapio_data
 * @brief Stores a web based URL.
 */
class URL {
public:

  /** Guess a default port for a given scheme, return 0 if not known */
  static int
  defaultPort(const std::string& scheme);

  /** Convert a string to one with HTTP url encoded */
  static std::string
  encodeURL(const std::string& in);

  URL() : port(0){ }

  ~URL(){ }

  // URL(const char * s)
  // {
  //   *this = s;
  // }

  URL(const std::string& s)
  {
    *this = s;
  }

  // Operator functions

  /** Set URL from a std::string */
  URL      &
  operator = (const std::string&);

  /** Set URL from a const char * */
  URL      &
  operator = (const char * s);

  /** Is URL < another URL (string comparison) */
  bool
  operator < (const URL& that) const
  {
    return (toString() < that.toString());
  }

  /** Compare if two URL are equal */
  bool
  operator == (const URL&) const;

  /** Compare if two URL are not equal */
  bool
  operator != (const URL&) const;

  /** Add string to a URL */
  URL       &
  operator += (const std::string& add);

  /**
   * Returns a GET string form of the URL.
   */
  std::string
  toGetString() const;

  /**
   * Returns a string form of the URL, much as you'd see in the URL entry field
   * of a web browser.
   */
  std::string
  toString() const;

  /** Parse subpart of a URL string */
  void
  parseAfterHost(const std::string& url_fragment);

  /** Return the scheme part of URL, if any */
  std::string
  getScheme() const;

  /** Set the scheme of the URL, passing true trys to set port if scheme matches known. */
  void
  setScheme(const std::string& s, bool tryPort = true);

  /** Return the user part of URL, if any */
  std::string
  getUser() const;

  /** Set the user part of URL */
  void
  setUser(const std::string& u);

  /** Return the password part of URL, if any */
  std::string
  getPassword() const;

  /** Set the password part of URL */
  void
  setPassword(const std::string& p);

  /** Return the host part of URL, if any */
  std::string
  getHost() const;

  /** Set the host of URL */
  void
  setHost(const std::string& p);

  /** Return the port of URL */
  unsigned short
  getPort() const;

  /** Set the port of the URL */
  void
  setPort(const unsigned short p);

  /** Return the path part of URL, if any */
  std::string
  getPath() const;

  /** Set the path of URL */
  void
  setPath(const std::string& p);

  /** Return the fragment of URL, if any */
  std::string
  getFragment() const;

  /** Set the fragment of URL */
  void
  setFragment(const std::string& f);

  // Query functions

  /** Get a query key */
  std::string
  getQuery(const std::string& key) const;

  /** Set a query key to string */
  void
  setQuery(const std::string& key, const std::string& val);

  /** Clear all query items */
  void
  clearQuery();

  /** Do we have this query key? */
  bool
  hasQuery(const std::string& key) const;

  /** Is this URL empty? */
  bool
  empty() const;

  /** Is this URL a local file? */
  bool
  isLocal() const;

  /**
   * Gets path's basename.
   * For example, if path is \p /tmp/filename.txt.bz2, then \p filename.txt.bz2
   * will be returned.
   * @see OS::getDirName()
   */
  std::string
  getBaseName() const;

  /**
   * Gets path's dirname.
   * For example, if path is \p /tmp/filename.txt.bz2, then \p /tmp will be
   * returned.
   * @see OS::getDirName()
   */
  std::string
  getDirName() const;

  /**
   * Returns path's suffix in all lowercase.
   * For example, if path is \p /tmp/filename.txt.BZ2, then \p bz2 will be
   * returned.
   * This is to simplify string equality comparisons with getSuffix().
   */
  std::string
  getSuffixLC() const;

  /** Output a URL */
  friend std::ostream&
  operator << (std::ostream&,
    const rapio::URL&);

protected:

  /** The scheme of the url, such as ftp, http, file, or rssd. */
  std::string scheme;

  /** Optional username.  Use if authentication is needed. */
  std::string user;

  /** Optional username.  Use if authentication is needed. */
  std::string pass;

  /** Hostname of the machine to be accessed.  An empty string or \p localhost
   * implies the file is local. */
  std::string host;

  /** The port to connect to host on. */
  unsigned short port;

  /** Path to the file, including directory and basename. */
  std::string path;

  /** Sub-part of the URL to skip to. */
  std::string fragment;

  /** Arguments to be passed along when accessing the URL */
  std::map<std::string, std::string> query;
};
}
