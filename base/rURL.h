#pragma once

#include <rData.h>
#include <map>
#include <string>

namespace rapio {
/**
 * A class for parsing, accessing, changing, and recombining a URL's components.
 *
 * <b>Compatability with RSS</b>\n
 * URLs can be instantiated from strings containing an rssd pseudo-URL\n
 * or code's deprecated protocol-plus-rssd-URL:
 * \li <b>tensor:/tmp/foo.txt</b> becomes <b>rssd://tensor/tmp/foo.txt</b>
 * \li <b>tensor:/tmp/foo.lb</b> becomes <b>lb://tensor/tmp/foo.lb</b>
 * \li <b>xmllb:tensor:/tmp/foo.lb</b> becomes
 *<b>orpg://tensor/tmp/foo.lb?protocol=xmllb</b>
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
 */
class URL : public Data {
public:

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
  int port;

  /** Path to the file, including directory and basename. */
  std::string path;

  /** Arguments to be passed along when accessing the URL */
  std::map<std::string, std::string> query;

  /** Sub-part of the URL to skip to.  Currently unused by WDSSII. */
  std::string fragment;

public:

  URL() : port(0){ }

  ~URL(){ }

  URL(const char * s)
  {
    *this = s;
  }

  URL(const std::string& s)
  {
    *this = s;
  }

  static URL
  fromHostFileName(const std::string& host,
    const std::string               & file);
  URL      &
  operator = (const std::string&);
  URL      &
  operator = (const char * s);
  bool
  operator < (const URL& that) const
  {
    return (toString() < that.toString());
  }

  bool
  operator == (const URL&) const;
  bool
  operator != (const URL&) const;
  URL       &
  operator += (const std::string& add);

  /**
   * Returns a string form of the URL, much as you'd see in the URL entry field
   * of a web browser.
   */
  std::string
  toString() const;

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
   * Returns path's suffix.
   * For example, if path is \p /tmp/filename.txt.bz2, then \p bz2 will be
   * returned.
   */
  std::string
  getSuffix() const;

  /**
   * Returns path's suffix in all lowercase.
   * For example, if path is \p /tmp/filename.txt.BZ2, then \p bz2 will be
   * returned.
   * This is to simplify string equality comparisons with getSuffix().
   */
  std::string
  getSuffixLC() const;

  std::string
  getQuery(const std::string& key) const;

  bool
  hasQuery(const std::string& key) const
  {
    return (query.find(key) != query.end());
  }

  bool
  empty() const;

  bool
  isLocal() const;

  void
  setQuery(const std::string& key,
    int                     val);

  void
  setQuery(const std::string& key, const std::string& val)
  {
    query[key] = val;
  }

  void
  removeSuffix()
  {
    path.resize(path.size() - getSuffix().size());
  }

  friend std::ostream&
  operator << (std::ostream&,
    const rapio::URL&);
};
}
