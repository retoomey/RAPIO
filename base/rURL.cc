#include "rURL.h"

#include "rStrings.h"
#include "rError.h"
#include "rOS.h"

#include <cstring>  // strlen()
#include <cstdlib>  // atoi()
#include <cstdio>   // snprintf()
#include <libgen.h> // dirname

using namespace rapio;

namespace {
const std::string::size_type npos = std::string::npos;

std::string
peel(std::string& s, const char * delimiter)
{
  auto p     = s.find(delimiter);
  auto token = s.substr(0, p);

  if (p == std::string::npos) { s.clear(); } else { s.erase(0, p + strlen(delimiter)); }
  return (token);
}

/*
 * http://joe:blow@example.com:81/loc/script.php?var=val&foo=bar#here
 */
void
parseAfterHost(const std::string& url_fragment,
  std::string& path,
  std::map<std::string, std::string>& query,
  std::string& fragment)
{
  auto w(url_fragment);

  path = peel(w, "?");

  // special case to handle this beauty:
  // file:///n:\ckerr\KINX\code_index.xml?protocol=xml"
  if ((path.size() > 4) && (path[0] == '/') && isalpha(path[1]) &&
    (path[2] == ':') && ((path[3] == '/') || (path[3] == '\\')))
  {
    path.erase(
      path.begin());
  }

  // query
  std::string q(peel(w, "#"));

  while (!q.empty()) {
    auto first = peel(q, "&");
    auto key   = peel(first, "=");
    auto val   = first;
    query[key] = val;
  }

  // fragment
  fragment = w;
}

int
defaultPort(const std::string& scheme)
{
  if (scheme == "http") { return (80); }

  if (scheme == "https") { return (443); }

  if (scheme == "ftp") { return (21); }

  if (scheme == "sftp") { return (22); }

  return (0);
}

void
parseAfterScheme(const std::string& url,
  const std::string& scheme,
  std::string& user, std::string& pass,
  std::string& host, int& port,
  std::string& path,
  std::map<std::string, std::string>& query,
  std::string& fragment)
{
  auto w = url;
  auto p = w.find("/");

  // username:password@host:port
  auto authority = w.substr(0, p);

  if (authority.find('@') != npos) {
    pass = peel(authority, "@");
    user = peel(pass, ":");
  }

  host = peel(authority, ":");
  port = authority.empty() ? defaultPort(scheme) : atoi(authority.c_str());

  w = p == npos ? "" : w.substr(p);

  parseAfterHost(w, path, query, fragment);
}

void
parseURL(const std::string& url,
  std::string& scheme, std::string& user,
  std::string& pass, std::string& host, int& port,
  std::string& path,
  std::map<std::string, std::string>& query,
  std::string& fragment)
{
  auto w(url);

  scheme = peel(w, "://");
  parseAfterScheme(w, scheme, user, pass, host, port, path, query, fragment);
}
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

// static
URL
URL::fromHostFileName(const std::string& h, const std::string& p)
{
  URL result;

  if (h.empty()) { result.scheme = "file"; } else {
    result.scheme = "orpg"; // remote file
    result.host   = h;
  }
  result.path = p;
  return (result);
}

URL&
URL::operator = (const std::string& s)
{
  const std::string::size_type npos(std::string::npos);

  // reset all the fields
  query.clear();
  scheme = user = pass = host = path = fragment = "";
  port   = 0;

  if (s.empty())
  { } else if (s == "-") { // stdin
    path = s;
  } else if (s.find("://") != s.npos) { // this is a url
    parseURL(s, scheme, user, pass, host, port, path, query, fragment);
  } else if (isalpha(s[0]) && (s[1] == ':') && ((s[2] == '\\') || (s[2] == '/'))) {
    parseAfterHost(s, path, query, fragment);
  } else if (s.find(":/") != s.npos) { // this is orpginfr format
    scheme = "orpg";
    auto w = s;
    auto p = w.find(":/");
    host = w.substr(0, p);
    w    = p == npos ? "" : w.substr(p + 1);

    parseAfterHost(w, path, query, fragment);

    p = host.find(":");

    if (p != npos) { // xmllb:tensor:/ ...
      query["protocol"] = host.substr(0, p);
      host = host.substr(p + 1);
    }
  } else if (s[0] == '/') { // this is a local file format in Unix
    parseAfterHost(s, path, query, fragment);
  } else { // treat it as a file with a relative path...
    *this = OS::getCurrentDirectory() + "/" + s;
  }

  if (((host == "xmllb") || (host == "xml") || (host == "webindex")) &&
    !query.count("protocol")) { std::swap(host, query["protocol"]); }

  //  LogDebug ("Parsed \"" << s << "\" to " << toString() << '\n');
  return (*this);
} // =

std::string
URL::toString() const
{
  std::string s;
  s.reserve(2048);

  if (!empty()) {
    if (!scheme.empty()) {
      s += scheme;
      s += "://";

      if (!user.empty() || !pass.empty()) {
        s += user;
        s += ':';
        s += pass;
        s += '@';
      }
      s += host;

      if ((scheme != "orpg") && (port != 0) && (port != defaultPort(scheme))) {
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
        s += it->first;
        s += '=';
        s += it->second;

        if (++it != end) { s += '&'; }
      } while (it != end);
    }

    if (!fragment.empty()) {
      s += '#';
      s += fragment;
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
URL::setQuery(const std::string& key, int val)
{
  char buf[32];

  std::snprintf(buf, sizeof(buf), "%d", val);
  query[key] = buf;
}

std::string
URL::getSuffix() const
{
  std::string suffix;
  auto pos = path.rfind('.');

  if (pos != path.npos) { suffix = path.substr(pos + 1); }
  return (suffix);
}

std::string
URL::getSuffixLC() const
{
  std::string suffix(getSuffix());
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
  if (host.empty() || (host == "localhost")) { return (true); }

  std::string hostname;

  // if (OS::getHostName(&hostname, 0) && (hostname == host)) { return (true); }
  if (OS::getHostName() == host) { return (true); }

  return (false);
}

namespace rapio {
std::ostream&
operator << (std::ostream& os, const rapio::URL& url)
{
  os << url.toString();
  return (os);
}
}
