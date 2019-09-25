#include "rIOIndex.h"

#include "rFactory.h"
#include "rTime.h"
#include "rStrings.h"
#include "rOS.h"
#include "rConfigDirectoryMapping.h"

// Index ability classes
#include "rXMLIndex.h"
#include "rWebIndex.h"
#include "rFMLIndex.h"

using namespace rapio;
using namespace std;

// FIXME: could IndexType and IOIndex merge into single class
void
IOIndex::introduce(const string & protocol,
  std::shared_ptr<IndexType>    factory)
{
  Factory<IndexType>::introduce(protocol, factory);
}

void
IOIndex::introduceSelf()
{
  // Indexes we support by default
  XMLIndex::introduceSelf(); // Local code_index.xml
  FMLIndex::introduceSelf(); // .fml files
  WebIndex::introduceSelf(); // web connection
}

IOIndex::~IOIndex()
{ }

std::shared_ptr<IndexType>
IOIndex::createIndex(const URL            & url,
  vector<std::shared_ptr<IndexListener> > listeners,
  const TimeDuration                      & maximumHistory)
{
  // This protocol check is typically for type based on file ending,
  // such as netcdf, xml, etc.
  std::string protocol = url.getQuery("protocol");

  if (protocol.empty() || (protocol == "auto")) {
    std::string suffix = url.getSuffixLC();

    if ((suffix == "gz") || (suffix == "bz2")) {
      URL tmp(url);
      tmp.removeSuffix();                   // removes the gz
      tmp.path.resize(tmp.path.size() - 1); /// removes the dot
      suffix = tmp.getSuffixLC();
    }

    if (suffix == "lb") {
      protocol = "xmllb";
    } else {
      protocol = suffix;
    }

    // LogInfo("Using protocol=" << protocol << " based on suffix=" << suffix <<
    // "\n");
  }

  // We macro machine names to our nssl vmrms data sources like so...
  // "//vmrms-sr02/KTLX" --> http://vmrms-webserv/vmrms-sr02?source=KTLX
  URL u = url;

  if (protocol.empty()) {
    if (url.host == "") {
      std::string p = url.path;

      if (p.size() > 1) {
        if ((p[0] == '/') && (p[1] == '/')) {
          p = p.substr(2);
          std::vector<std::string> pieces;
          Strings::splitWithoutEnds(p, '/', &pieces);

          if (pieces.size() > 1) {
            std::string machine  = pieces[0];
            std::string source   = pieces[1];
            std::string expanded = "http://vmrms-webserv/" + machine
              + "?source=" + source;
            LogInfo("Web macro source sent to " << expanded << "\n");
            u = URL(expanded);
          }
        }
      }
    }

    // Go ahead and try a webindex protocol
    // This avoids needing '&protocol=webindex' on all our web served source
    // strings.
    protocol = "webindex";
  }

  if (u.path.empty() || protocol.empty()) {
    LogSevere("Unable to get protocol out of " << u << "\n");
    return (0);
  }

  // use protocol and device to get the index.
  if (protocol == "fam") { protocol = "fml"; } // Because it's NOT fam, it's fml files
  std::shared_ptr<IndexType> factory = Factory<IndexType>::get(protocol,
      "Index protocol");

  if (factory == nullptr) {
    return (0);
  }

  std::shared_ptr<IndexType> newindex = factory->createIndexType(
    u, listeners, maximumHistory);

  return (newindex);
} // IOIndex::createIndex

/**
 * getIndexPath goes through the following steps to convert lbName to an
 * indexPath:
 *
 * <ol>
 * <li>lbName's baseName() is removed.
 * <li>If no directory is specified in lbName, the cwd is added.
 * <li>Substitutions from misc/directoryMapping.xml are made.
 * </ol>
 */
std::string
IOIndex::getIndexPath(const URL& url_in)
{
  URL url(url_in);

  // if no dir, use the current working directory
  if (url.getDirName().empty()) {
    url.path = OS::getCurrentDirectory() + "/" + url.path;
  }

  // strip out the filename...
  url.path = url.getDirName();

  // if the url.query contains source=KVNX, add KVNX to path
  if (url.hasQuery("source")) {
    url.path = url.path + '/' + url.getQuery("source");
  }

  // strip out the query...
  url.query.clear();

  // directory mapping can operate on both host and path...
  if (!url.host.empty()) { ConfigDirectoryMapping::doDirectoryMapping(url.host); }
  ConfigDirectoryMapping::doDirectoryMapping(url.path);

  LogDebug("getIndexPath for [" << url_in << "] returns [" << url << "]\n");
  return (url.toString());
}
