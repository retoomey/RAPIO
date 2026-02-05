#include "rIOIndex.h"

#include "rFactory.h"
#include "rTime.h"
#include "rStrings.h"
#include "rOS.h"
#include "rConfigDirectoryMapping.h"
#include "rColorTerm.h"

// Index ability classes
#include "rXMLIndex.h"
#include "rWebIndex.h"
#include "rFileIndex.h"
#include "rFMLIndex.h"
#include "rStreamIndex.h"
#include "rFakeIndex.h"
#include "rRedisIndex.h"

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
  FileIndex::introduceSelf();   // General files
  FMLIndex::introduceSelf();    // .fml files
  WebIndex::introduceSelf();    // web connection
  StreamIndex::introduceSelf(); // Stream index
  FakeIndex::introduceSelf();   // Fake index
  RedisIndex::introduceSelf();  // Redis index
}

std::string
IOIndex::introduceHelp()
{
  std::string help;

  help += "Indexes ingest data into the system, either with metadata notifications, or direct files.\n";
  help += "Default no protocol ending in .xml is an xml index.\n";
  help += "Default no protocol ending in .fam is an ifam index.\n";
  help += "Default url with 'source' or web macro is a web index.\n";
  help += "Indexes ingest data into the system.\n";
  help += ColorTerm::blue() + "The following types are registered:" + ColorTerm::reset() + "\n";
  auto e = Factory<IndexType>::getAll();

  for (auto i: e) {
    help += " " + ColorTerm::red() + i.first + ColorTerm::reset() + " : " + i.second->getHelpString(i.first) + "\n";
  }
  return help;
}

IOIndex::~IOIndex()
{ }

std::shared_ptr<IndexType>
IOIndex::createIndex(
  const std::string  & protocolin,
  const std::string  & indexparamsin,
  const TimeDuration & maximumHistory)
{
  // Look for a scheme for factory
  std::string protocol    = protocolin;
  std::string indexparams = indexparamsin;

  // ---------------------------------------------------------
  // Let the indexes check in order for missing protocol support
  URL url(indexparams);

  WebIndex::canHandle(url, protocol, indexparams);
  FMLIndex::canHandle(url, protocol, indexparams);
  XMLIndex::canHandle(url, protocol, indexparams);

  // If STILL empty after indexes check/update protocol...
  if (protocol.empty()) {
    fLogSevere("Unable to guess schema from '{}'", indexparamsin);
  } else {
    // use protocol and device to get the index.
    std::shared_ptr<IndexType> factory = Factory<IndexType>::get(protocol,
        "Index protocol");
    if (factory != nullptr) {
      return (factory->createIndexType(protocol, indexparams, maximumHistory));
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

  fLogDebug("getIndexPath for [{}] returns [{}]", url_in.toString(), url.toString());
  return (url.toString());
} // IOIndex::getIndexPath
