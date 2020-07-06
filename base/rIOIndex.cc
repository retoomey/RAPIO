#include "rIOIndex.h"

#include "rFactory.h"
#include "rTime.h"
#include "rStrings.h"
#include "rOS.h"
#include "rConfigDirectoryMapping.h"
#include "rCompression.h"

// Index ability classes
#include "rXMLIndex.h"
#include "rWebIndex.h"
#include "rFMLIndex.h"
#include "rStreamIndex.h"

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
  XMLIndex::introduceSelf();    // Local code_index.xml
  FMLIndex::introduceSelf();    // .fml files
  WebIndex::introduceSelf();    // web connection
  StreamIndex::introduceSelf(); // Stream index
}

IOIndex::~IOIndex()
{ }

std::shared_ptr<IndexType>
IOIndex::createIndex(
  const std::string                       & protocolin,
  const std::string                       & indexparamsin,
  vector<std::shared_ptr<IndexListener> > listeners,
  const TimeDuration                      & maximumHistory)
{
  // Look for a scheme for factory
  std::string protocol    = protocolin;
  std::string indexparams = indexparamsin;

  // ---------------------------------------------------------
  // Legacy support for old style -i options:
  //
  // Web index:
  //   "//vmrms-sr02/KTLX" and http://vmrms-webserv/vmrms-sr02?source=KTLX
  // XML index (no protocol, ends with .xml):
  //   //code_index.xml
  // FML index using FAM (no protocol, ends with fam):
  //   //path/something.fam
  if (protocol.empty()) {
    // Try to make URL from the indexparams...

    URL url(indexparams);

    // If there's a source tag, it's an old web index
    if (!url.getQuery("source").empty()) {
      protocol = WebIndex::WEBINDEX;
    }

    if (protocol.empty()) {
      // We macro machine names to our nssl vmrms data sources like so...
      // "//vmrms-sr02/KTLX" --> http://vmrms-webserv/vmrms-sr02?source=KTLX
      if (url.getHost() == "") {
        std::string p = url.getPath();

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
              //   u = URL(expanded);
              indexparams = expanded; // Change to non-macroed format
            }
            protocol = WebIndex::WEBINDEX;
          }
        }
      }
    }

    // If still empty, Look at suffix for legacy fam/xml
    if (protocol.empty()) {
      std::string suffix = url.getSuffixLC();
      if (suffix == "fam") {
        protocol = FMLIndex::FMLINDEX_FAM;
      } else if (suffix == "xml") {
        protocol = XMLIndex::XMLINDEX;
      } else if (Compression::suffixRecognized(suffix)) {
        URL tmp(url);
        tmp.removeSuffix(); // removes the suffix
        std::string p = tmp.getPath();
        p.resize(p.size() - 1); /// removes the dot
        tmp.setPath(p);
        suffix = tmp.getSuffixLC();

        if (suffix == "xml") {
          protocol = XMLIndex::XMLINDEX;
        }
      }
    }
  }

  if (protocol.empty()) {
    LogSevere("Unable to guess schema from '" << indexparamsin << "'\n");
  } else {
    // use protocol and device to get the index.
    std::shared_ptr<IndexType> factory = Factory<IndexType>::get(protocol,
        "Index protocol");
    if (factory != nullptr) {
      return (factory->createIndexType(protocol, indexparams, listeners, maximumHistory));
    }
  }
  return (nullptr);
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

  std::string p = url.getPath();

  // if no dir, use the current working directory
  if (url.getDirName().empty()) {
    p = OS::getCurrentDirectory() + "/" + p;
  }

  // strip out the filename...
  p = url.getDirName();

  // if the url.query contains source=KVNX, add KVNX to path
  if (url.hasQuery("source")) {
    p = p + '/' + url.getQuery("source");
  }

  // strip out the query...
  url.clearQuery();

  // directory mapping can operate on both host and path...
  if (!url.getHost().empty()) {
    std::string h = url.getHost();
    ConfigDirectoryMapping::doDirectoryMapping(h);
    url.setHost(h);
  }
  ConfigDirectoryMapping::doDirectoryMapping(p);

  url.setPath(p);

  LogDebug("getIndexPath for [" << url_in << "] returns [" << url << "]\n");
  return (url.toString());
} // IOIndex::getIndexPath
