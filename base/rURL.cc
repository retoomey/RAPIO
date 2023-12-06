#include "rURL.h"

#include "rStrings.h"
#include "rError.h"
#include "rOS.h"

#include <cstring>  // strlen()
#include <cstdlib>  // atoi()
#include <libgen.h> // dirname

using namespace rapio;

void
URL::parseAfterHost(const std::string& url_fragment)
{
  auto w(url_fragment);

  path = Strings::peel(w, "?");

  // special case to handle this beauty:
  // file:///n:\ckerr\KINX\code_index.xml?protocol=xml"
  if ((path.size() > 4) && (path[0] == '/') && isalpha(path[1]) &&
    (path[2] == ':') && ((path[3] == '/') || (path[3] == '\\')))
  {
    path.erase(
      path.begin());
  }

  // query
  std::string q(Strings::peel(w, "#"));

  while (!q.empty()) {
    auto first = Strings::peel(q, "&");
    auto key   = Strings::peel(first, "=");
    auto val   = first;
    query[key] = val;
  }

  // fragment
  fragment = w;
}

URL&
URL::operator = (const char * s)
{
  return (*this = std::string(s ? s : ""));
}

URL&
URL::operator += (const std::string& s)
{
  this->path += s;
  return (*this);
}

int
URL::defaultPort(const std::string& scheme)
{
  if (scheme == "http") { return (80); }

  if (scheme == "https") { return (443); }

  if (scheme == "ftp") { return (21); }

  if (scheme == "sftp") { return (22); }

  return (0);
}

std::string
URL::encodeURL(const std::string& in)
{
  // RFC 3986
  // https://en.wikipedia.org/wiki/Percent-encoding
  std::string o;

  for (auto c:in) {
    if (isalnum(c) || (c == '-') || (c == '_') || (c == '.') || (c == '~')) {
      o += c;
    } else {
      char buf[32];
      snprintf(buf, sizeof(buf), "%X", (int ((unsigned char) c)));
      o += "%";
      o += buf;
    }
  }
  return o;
}

URL&
URL::operator = (const std::string& s)
{
  // reset all the fields
  query.clear();
  scheme = user = pass = host = path = fragment = "";
  port   = 0;

  if (s.empty())
  { } else if (s == "-") { // stdin
    path = s;
  } else if (s.find("://") != std::string::npos) { // this is a url
    auto w(s);                                     // Lots of copying of strings going on, we could do string_view or something

    // Grab scheme first
    scheme = Strings::peel(w, "://");

    // Grab username:password@host:port
    auto p         = w.find("/");
    auto authority = w.substr(0, p); // could be until the end of the string right?
    if (authority.find('@') != std::string::npos) {
      pass = Strings::peel(authority, "@");
      user = Strings::peel(pass, ":");
    }
    host = Strings::peel(authority, ":");
    port = authority.empty() ? URL::defaultPort(scheme) : atoi(authority.c_str());
    w    = p == std::string::npos ? "" : w.substr(p);
    parseAfterHost(w);
  } else if (isalpha(s[0]) && (s[1] == ':') && ((s[2] == '\\') || (s[2] == '/'))) {
    parseAfterHost(s);
  } else if (s[0] == '/') { // this is a local file format in Unix
    parseAfterHost(s);
  } else { // treat it as a file with a relative path...
    *this = OS::getCurrentDirectory() + "/" + s;
  }

  if (((host == "xml") || (host == "webindex")) &&
    !query.count("protocol")) { std::swap(host, query["protocol"]); }

  return (*this);
} // =

std::string
URL::toGetString() const
{
  std::string s;

  if (!empty()) {
    if (!scheme.empty()) {
      if (!user.empty() || !pass.empty()) {
        s += URL::encodeURL(user);
        s += ':';
        s += URL::encodeURL(pass);
        s += '@';
      }
      s += URL::encodeURL(host);

      if (path.empty() || (path[0] != '/')) { s += '/'; }
    }
    s += path;

    if (!query.empty()) {
      s += '?';
      auto it(query.begin());
      auto end(query.end());

      do {
        s += URL::encodeURL(it->first);
        s += '=';
        s += URL::encodeURL(it->second);

        if (++it != end) { s += '&'; }
      } while (it != end);
    }

    if (!fragment.empty()) {
      s += '#';
      s += URL::encodeURL(fragment);
    }
  }
  return (s);
} // URL::toGetString

std::string
URL::toString() const
{
  std::string s;

  if (!empty()) {
    if (!scheme.empty()) {
      s += scheme;
      s += "://";

      if (!user.empty() || !pass.empty()) {
        s += URL::encodeURL(user);
        s += ':';
        s += URL::encodeURL(pass);
        s += '@';
      }
      s += URL::encodeURL(host);

      if ((port != 0) && (port != URL::defaultPort(scheme))) {
        // s += ":";  Think printf is quite a bit faster actually
        // s += std::to_string(port);
        char buf[32];
        snprintf(buf, sizeof(buf), ":%d", port);
        s += buf;
      }

      if (path.empty() || (path[0] != '/')) { s += '/'; }
    }
    s += path;

    if (!query.empty()) {
      s += '?';
      auto it(query.begin());
      auto end(query.end());

      do {
        s += URL::encodeURL(it->first);
        s += '=';
        s += URL::encodeURL(it->second);

        if (++it != end) { s += '&'; }
      } while (it != end);
    }

    if (!fragment.empty()) {
      s += '#';
      s += URL::encodeURL(fragment);
    }
  }
  return (s);
} // URL::toString

std::string
URL::getQuery(const std::string& key) const
{
  std::string val;
  auto it = query.find(key);

  if (it != query.end()) { val = query.find(key)->second; }
  return (val);
}

void
URL::setQuery(const std::string& key, const std::string& val)
{
  query[key] = val;
}

void
URL::clearQuery()
{
  query.clear();
}

bool
URL::hasQuery(const std::string& key) const
{
  return (query.find(key) != query.end());
}

std::string
URL::getSuffixLC() const
{
  std::string suffix;
  auto pos = path.rfind('.');

  if (pos != path.npos) { suffix = path.substr(pos + 1); }
  Strings::toLower(suffix);
  return (suffix);
}

std::string
URL::getBaseName() const
{
  std::string scratch_copy(path);
  std::string retval = ::basename(&scratch_copy[0]);

  return retval;
}

std::string
URL::getDirName() const
{
  std::string scratch(path);
  std::string retval = ::dirname(&scratch[0]);

  return retval;
}

bool
URL::operator == (const URL& that) const
{
  return ((this->scheme == that.scheme) &&
         (this->user == that.user) &&
         (this->pass == that.pass) &&
         (this->host == that.host) &&
         (this->port == that.port) &&
         (this->path == that.path) &&
         (this->query == that.query) &&
         (this->fragment == that.fragment));
}

bool
URL::operator != (const URL& that) const
{
  return (!(*this == that));
}

bool
URL::empty() const
{
  return (scheme.empty() &&
         user.empty() &&
         pass.empty() &&
         host.empty() &&
         path.empty() &&
         query.empty() &&
         fragment.empty());
}

bool
URL::isLocal() const
{
  return (host.empty() || (host == "localhost") || (OS::getHostName() == host));
}

std::string
URL::getScheme() const
{
  return scheme;
}

void
URL::setScheme(const std::string& s, bool tryPort)
{
  scheme = s;
  if (tryPort) {
    int p = defaultPort(s);
    if (p != 0) { port = p; }
  }
}

std::string
URL::getUser() const
{
  return user;
}

void
URL::setUser(const std::string& u)
{
  // Going to assume can't have empty user with a password
  if (u.empty()) {
    user = "";
    pass = "";
  } else {
    user = u;
  }
}

std::string
URL::getPassword() const
{
  return pass;
}

void
URL::setPassword(const std::string& p)
{
  pass = p;
}

std::string
URL::getHost() const
{
  return host;
}

void
URL::setHost(const std::string& p)
{
  host = p;
}

unsigned short
URL::getPort() const
{
  return port;
}

void
URL::setPort(const unsigned short p)
{
  port = p;
}

std::string
URL::getPath() const
{
  return path;
}

void
URL::setPath(const std::string& p)
{
  path = p;
}

std::string
URL::getFragment() const
{
  return fragment;
}

void
URL::setFragment(const std::string& f)
{
  fragment = f;
}

namespace rapio {
std::ostream&
operator << (std::ostream& os, const rapio::URL& url)
{
  os << url.toString();
  return (os);
}
}
