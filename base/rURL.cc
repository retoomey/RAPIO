#include "rURL.h"

#include "rStrings.h"
#include "rError.h"
#include "rOS.h"

#include <charconv>
#include <cstring>  // strlen()
#include <libgen.h> // dirname

using namespace rapio;

void
URL::parseAfterHost(std::string_view url_fragment)
{
  std::string selfCopy;
  auto aliasesStr = [url_fragment](const std::string& str) {
      if (url_fragment.empty() || str.empty()) { return false; }
      std::less<const char *> lessPtr;

      return !lessPtr(url_fragment.data(), str.data()) &&
             lessPtr(url_fragment.data(), str.data() + str.size());
    };
  bool isAliased = aliasesStr(path) || aliasesStr(fragment);

  if (!isAliased) {
    for (const auto& [k, v] : query) {
      if (aliasesStr(k) || aliasesStr(v)) {
        isAliased = true;
        break;
      }
    }
  }
  if (isAliased) {
    selfCopy.assign(url_fragment);
    url_fragment = selfCopy;
  }

  std::string_view w(url_fragment);

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
  std::string_view q = Strings::peel(w, "#");

  while (!q.empty()) {
    auto first = Strings::peel(q, "&");
    auto key   = Strings::peel(first, "=");
    auto val   = first;
    setQuery(key, val);
  }

  // fragment
  fragment = w;
} // URL::parseAfterHost

URL&
URL::operator = (const char * s)
{
  return (*this = std::string_view(s ? s : ""));
}

URL&
URL::operator += (std::string_view s)
{
  this->path += s;
  return (*this);
}

int
URL::defaultPort(std::string_view scheme)
{
  if (scheme == "http") { return (80); }

  if (scheme == "https") { return (443); }

  if (scheme == "ftp") { return (21); }

  if (scheme == "sftp") { return (22); }

  return (0);
}

std::string
URL::encodeURL(std::string_view in)
{
  // RFC 3986
  // https://en.wikipedia.org/wiki/Percent-encoding
  std::string o;

  // Pre-allocate memory to avoid reallocations in the loop.
  // We know it will be at least in.length(), and a little extra padding helps.
  o.reserve(in.length() * 1.5);

  for (auto c:in) {
    if (isalnum(c) || (c == '-') || (c == '_') || (c == '.') || (c == '~')) {
      o += c;
    } else {
      // Safely formats to a 2-character, uppercase hex string (e.g., "%20" for space)
      o += fmt::format("%{:02X}", static_cast<int>(static_cast<unsigned char>(c)));
    }
  }
  return o;
}

URL&
URL::operator = (std::string_view s)
{
  // Guard against self-aliasing (e.g., assigning a string_view that points into one of this URL's own strings)
  std::string selfCopy;
  auto aliasesStr = [s](const std::string& str) {
      if (s.empty() || str.empty()) { return false; }
      std::less<const char *> lessPtr;

      return !lessPtr(s.data(), str.data()) && lessPtr(s.data(), str.data() + str.size());
    };
  bool isAliased = aliasesStr(scheme) || aliasesStr(user) || aliasesStr(pass) ||
    aliasesStr(host) || aliasesStr(path) || aliasesStr(fragment);

  if (!isAliased) {
    for (const auto& [k, v] : query) {
      if (aliasesStr(k) || aliasesStr(v)) {
        isAliased = true;
        break;
      }
    }
  }
  if (isAliased) {
    selfCopy.assign(s);
    s = selfCopy;
  }

  // reset all the fields
  query.clear();
  scheme = user = pass = host = path = fragment = "";
  port   = 0;

  if (s.empty())
  { } else if (s == "-") { // stdin
    path = s;
  } else if (s.find("://") != std::string_view::npos) { // this is a url
    std::string_view w(s);

    // Grab scheme first
    scheme = Strings::peel(w, "://");

    // Grab username:password@host:port
    auto p         = w.find('/');
    auto authority = w.substr(0, p); // could be until the end of the string right?
    if (authority.find('@') != std::string_view::npos) {
      std::string_view userpass = Strings::peel(authority, "@");
      user = Strings::peel(userpass, ":");
      pass = userpass;
    }
    host = Strings::peel(authority, ":");

    // Safe port parsing (in case of malicious or bad port pass-in)
    if (authority.empty()) {
      port = URL::defaultPort(scheme);
    } else {
      int parsed_port = 0;
      auto [ptr, ec] = std::from_chars(authority.data(), authority.data() + authority.size(), parsed_port);
      // Ports must be strictly between 1 and 65535 and consume the entire port string
      if ((ec == std::errc()) && (ptr == authority.data() + authority.size()) &&
        (parsed_port > 0) && (parsed_port <= 65535))
      {
        port = static_cast<unsigned short>(parsed_port);
      } else {
        port = URL::defaultPort(scheme); // Out of bounds or invalid, fallback
      }
    }

    w = p == std::string_view::npos ? std::string_view() : w.substr(p);
    parseAfterHost(w);
  } else if ((s.size() >= 3) && isalpha(s[0]) && (s[1] == ':') && ((s[2] == '\\') || (s[2] == '/'))) {
    parseAfterHost(s);
  } else if (s[0] == '/') { // this is a local file format in Unix
    parseAfterHost(s);
  } else { // treat it as a file with a relative path...
    *this = OS::getCurrentDirectory() + "/" + std::string(s);
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
      /*    if (!user.empty() || !pass.empty()) {
       *    s += URL::encodeURL(user);
       *    s += ':';
       *    s += URL::encodeURL(pass);
       *    s += '@';
       *  }
       *  s += URL::encodeURL(host);
       */

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
        s += fmt::format(":{}", port);
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
URL::getQuery(std::string_view key) const
{
  auto it = query.find(key);

  if (it != query.end()) { return it->second; }
  return "";
}

void
URL::setQuery(std::string_view key, std::string_view val)
{
  auto it = query.find(key);

  if (it != query.end()) {
    it->second.assign(val);
  } else {
    query.emplace(key, val);
  }
}

void
URL::clearQuery()
{
  query.clear();
}

bool
URL::hasQuery(std::string_view key) const
{
  return (query.find(key) != query.end());
}

std::string
URL::getSuffixLC() const
{
  std::string suffix;
  auto pos = path.rfind('.');

  if (pos != path.npos) {
    suffix = path.substr(pos + 1);

    // HARDEN: If the "suffix" starts with exactly 3 digits, it is the
    // %/ms macro from the timestamp, not a real file extension.
    if ((suffix.length() >= 3) && std::isdigit(suffix[0]) &&
      std::isdigit(suffix[1]) && std::isdigit(suffix[2]))
    {
      return ""; // Reject it and return an empty suffix
    }
  }
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
URL::setScheme(std::string_view s, bool tryPort)
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
URL::setUser(std::string_view u)
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
URL::setPassword(std::string_view p)
{
  pass = p;
}

std::string
URL::getHost() const
{
  return host;
}

void
URL::setHost(std::string_view p)
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
URL::setPath(std::string_view p)
{
  path = p;
}

std::string
URL::getFragment() const
{
  return fragment;
}

void
URL::setFragment(std::string_view f)
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
