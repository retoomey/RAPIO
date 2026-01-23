#include "rPluginIngestor.h"

#include <rRAPIOAlgorithm.h>
#include <rIOIndex.h>
#include <rRecordQueue.h>
#include <rConfigParamGroup.h>
#include <rEventLoop.h>

using namespace rapio;
using namespace std;

bool
PluginIngestor::declare(RAPIOProgram * owner, const std::string& name)
{
  static bool once = true; // FIXME: could we make it multi though?

  if (once) {
    owner->addPlugin(new PluginIngestor(name));
    once = false;
  } else {
    fLogSevere("Code error, can only declare ingestor once");
    exit(1);
  }
  return true;
}

void
PluginIngestor::declareOptions(RAPIOOptions& o)
{
  o.require(myName,
    "/data/radar/KTLX/code_index.xml",
    "The input sources");
  o.addGroup(myName, "I/O");
}

void
PluginIngestor::addPostLoadedHelp(RAPIOOptions& o)
{
  // The 'i' plugin relies on the Index::initialize called
  o.addAdvancedHelp(myName, IOIndex::introduceHelp());
}

void
PluginIngestor::processOptions(RAPIOOptions& o)
{
  ConfigParamGroupi parami; // Thinking about deprecating these

  parami.readString(o.getString(myName));
}

void
PluginIngestor::execute(RAPIOProgram * callerin)
{
  // The old setUpInitialIndexes from algorithm
  // belongs here. We only need indexes iff we have the 'i' option

  // Set up record queue for indexes to write to.  This couples the queue
  // with the -i option, I guess it could be made a separate plugin if needed later.
  // Create record queue and record filter/notifier
  RAPIOAlgorithm * caller = (RAPIOAlgorithm *) (callerin); // FIXME: bad design

  std::shared_ptr<RecordQueue> q = std::make_shared<RecordQueue>(caller);

  Record::theRecordQueue = q;
  EventLoop::addEventHandler(q);

  // Try to create an index for each source we want data from
  // and add sorted records to queue
  // FIXME: Archive should probably skip queue in case of giant archive cases,
  // otherwise we load a lot of memory.  We need better handling of this..
  // though for small archives we load all and sort, which is a nice ability
  size_t count         = 0;
  const auto& infos    = ConfigParamGroupi::getIndexInputInfo();
  size_t wanted        = infos.size();
  bool daemon          = caller->isDaemon();
  bool archive         = caller->isArchive();
  TimeDuration history = caller->getMaximumHistory();
  std::string what     = daemon ? "daemon" : "archive";

  if (daemon && archive) { what = "allrecords"; }

  for (size_t p = 0; p < wanted; ++p) {
    const auto& i = infos[p];
    std::shared_ptr<IndexType> in;

    bool success = false;
    in = IOIndex::createIndex(i.protocol, i.indexparams, history);
    myConnectedIndexes.push_back(in); // FIXME: Not sure we need this

    if (in != nullptr) {
      in->setIndexLabel(p); // Mark in order
      success = in->initialRead(daemon, archive);
      if (success) {
        count++;
      }
    }

    std::string how = success ? "Successful" : "Failed";
    fLogInfo("{} {} connection to '{}-->{}'", how, what, i.protocol, i.indexparams);
  }

  // Failed to connect to all needed sources...
  if (count != wanted) {
    fLogSevere("Only connected to {} of {} data sources, ending.", count, wanted);
    exit(1);
  }

  const size_t rSize = Record::theRecordQueue->size();

  fLogInfo("{} initial records from {} sources", rSize, wanted);

  // After all connections/archive reading/filtering, tell
  // RecordQueue to try processing queue.  This is important for the end
  // record event on empty queue (all records were filtered out, etc.)
  q->setReady();

  myActive = true;
} // PluginIngestor::execute
