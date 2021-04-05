#pragma once

#include <rAlgorithm.h>
#include <rRAPIOOptions.h>
#include <rRAPIOData.h>
#include <rIndexType.h>
#include <rRecordNotifier.h>
#include <rConfigParamGroup.h>

#include <string>
#include <vector>

namespace rapio {
/**
 *  The stock default algorithm and all its options and processing
 *
 *  @author Robert Toomey
 *
 */
class RAPIOAlgorithm : public Algorithm {
public:
  /** Construct a stock algorithm */
  RAPIOAlgorithm();

  /** Declare stock algorithm feature or abilities wanted */
  virtual void
  declareFeatures(){ }

  /** Post load, advanced help request.  This help requires the system to be initialized */
  virtual void
  addPostLoadedHelp(RAPIOOptions& o);

  /** Declare all algorithm options algorithm needs */
  virtual void
  declareOptions(RAPIOOptions& o){ };

  /** Process/setup from the given algorithm options */
  virtual void
  processOptions(RAPIOOptions& o){ };

  /** Declare input parameter options (Called before option parse) */
  virtual void
  declareInputParams(RAPIOOptions& o);

  /** Declare output parameter options (Called before option parse) */
  virtual void
  declareOutputParams(RAPIOOptions& o);

  /** Process output parameter options (Called after option parse) */
  virtual void
  processOutputParams(RAPIOOptions& o);

  /** Process input parameter options (Called after option parse) */
  virtual void
  processInputParams(RAPIOOptions& o);

  /** Initialize system wide base parsers */
  virtual void
  initializeBaseParsers();

  /** Initialize system wide setup */
  virtual void
  initializeBaseline();

  /** Initialize algorithm from c arguments and execute.  Used if algorithm is
   * standalone
   * and not chained with other modules.  This simplifies one shot algorithm
   * binaries. */
  void
  executeFromArgs(int argc,
    char *            argv[]);

  /** Should write product? Subclasses can use this bypass generation of
   * storage/cpu
   *  for a given product. Possibly another product in same algorithm depends on
   * this one,
   *  so optimizing depends on coder. */
  bool
  productMatch(const std::string& key,
    std::string                 & productName);

  /** Should write product? Subclasses can use this bypass generation of
   * storage/cpu
   *  for a given product. Possibly another product in same algorithm depends on
   * this one,
   *  so optimizing depends on coder. */
  bool
  isProductWanted(const std::string& key);

  /** Do any required processing on start if needed. */
  virtual void
  initOnStart(const IndexType& idx){ };

  /** Set up notifier if any for records.  This object is called when
   * a new record is generated by system.  The default writes .fml files.
   * This represents the -l parameter ability */
  virtual void
  setUpRecordNotifier();

  /** Set up record filter for incoming read records.  This allows trimming
   * of the input flow.  This represents the -I filter ability for records.  */
  virtual void
  setUpRecordFilter();

  /** After adding wanted inputs and indexes, execute the algorithm */
  virtual void
  execute();

  /** Handle new record, usually from event queue */
  virtual void
  handleRecordEvent(const Record& rec);

  /** Handle end of event index event (sent by archives) */
  virtual void
  handleEndDatasetEvent();

  /** Handle timed event.
   * @param at The actual now time triggering the event.
   * @param sync The pinned sync time we're firing for. */
  virtual void
  handleTimedEvent(const Time& at, const Time& sync);

  /** Process heartbeat in subclasses.
   * @param at The actual now time triggering the event.
   * @param sync The pinned sync time we're firing for. */
  virtual void
  processHeartbeat(const Time& n, const Time& p){ };

  /** Process a matched new record (occurs as the records come in)  Index number
   * is index into declared order */
  virtual void
  processNewData(RAPIOData&){ };

  // 'Features' I'll probably generalize later

  /** Request a added feature to the algorithm by key */
  virtual void
  addFeature(const std::string& featureKey);

  /** Write data to given key.  Key must exist/match the keys from
   * addOutputProduct */
  virtual void
  writeOutputProduct(const std::string& key,
    std::shared_ptr<DataType>         outputData);

  /** Get the maximum history specified by user */
  static TimeDuration
  getMaximumHistory();

  /** Is given time in the time window from -h */
  static bool
  inTimeWindow(const Time& aTime);

  /** Are we a daemon algorithm? For example, waiting on realtime data. */
  static bool
  isDaemon();

  /** Are we reading old records? */
  static bool
  isArchive();

protected:

  /** Indexes we are successfully attached to */
  std::vector<std::shared_ptr<IndexType> > myConnectedIndexes;

  /** Hold the "n" list of notifiers */
  std::string myNotifierList;

  /** Notifiers we are sending notification of new records to */
  std::vector<std::shared_ptr<RecordNotifierType> > myNotifiers;

  /** The cronlist for heartbeat/sync if any */
  std::string myCronList;

  // I believe these things will always be 'global', even
  // if we have multiple algorithm modules.

  /** History time for index storage */
  static TimeDuration myMaximumHistory;

  /** The time of last record received */
  static Time myLastDataTime;

  /** The record read mode */
  static std::string myReadMode;
};

//  end class RAPIOAlgorithm
}
