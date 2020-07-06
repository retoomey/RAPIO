#pragma once

#include <rAlgorithm.h>
#include <rRAPIOOptions.h>
#include <rRAPIOData.h>
#include <rIndexType.h>
#include <rRecordNotifier.h>

#include <string>
#include <vector>

namespace rapio {
/**
 *  Bring the common code we do all the time in the algorithms into
 *  a standard location.
 *
 *  Routines to handle the standard -I index URL(s).
 *  Routines to handle the standard -i product declarations.
 *  This means all the basic index connections and product filtering
 *  that is duplicated among algorithms.
 *
 *  Some algorithms break down the -I into unique fields, these can
 *  be added using the addInputProduct method directly.
 *
 */
class RAPIOAlgorithm : public Algorithm {
public:

  /** Store database of product information before creation */
  class productInputInfo : public Algorithm {
public:
    std::string name;
    std::string subtype;
  };

  /** Store database of output information before creation */
  class productOutputInfo : public Algorithm {
public:
    /** The product key matcher such as '*', "Reflectivity*" */
    std::string product;

    /** The subtype key matcher, if any */
    std::string subtype;

    /** The product key to translate into */
    std::string toProduct;

    /** The subtype key to translate into, if any */
    std::string toSubtype;
  };

  /** Store database of index information before creation */
  class indexInputInfo : public Algorithm {
public:
    std::string protocol;
    std::string indexparams;
    TimeDuration maximumHistory;
  };

  /** Construct a stock algorithm */
  RAPIOAlgorithm();

  /** Declare stock algorithm feature or abilities wanted */
  virtual void
  declareFeatures(){ }

  /** Declare all algorithm options algorithm needs */
  virtual void
  declareOptions(RAPIOOptions& o) = 0;

  /** Process/setup from the given algorithm options */
  virtual void
  processOptions(RAPIOOptions& o) = 0;

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

  /** Add a single input of the form Name:Subtype, where Subtype is optional */
  void
  addInputProduct(const std::string& product);

  /** Add product inputs, usually from a -I string on command line, separated by
   * spaces */
  void
  addInputProducts(const std::string& aList);

  /** How many input products were there? */
  size_t
  getInputNumber();

  /** Add a single output product name */
  void
  addOutputProduct(const std::string& product);

  /** Add output products */
  void
  addOutputProducts(const std::string& aList);

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

  /** Add single index from url */
  void
  addIndex(const std::string & index,
    const TimeDuration       & maximumHistory);

  /** Add indexes, usually from a -i string on command line, separated by spaces
   */
  void
  addIndexes(const std::string & aList,
    const TimeDuration         & maximumHistory);

  /** Do any required processing on start if needed. */
  virtual void
  initOnStart(const IndexType& idx);

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

  /** Does this record match a pattern on the -I? */
  virtual int
  matches(const Record& rec);

  /** Handle ANY new record from an index, no filtering done here yet */
  virtual void
  handleRecordEvent(const Record& rec);

  /** Handle end of event index event */
  virtual void
  handleEndDatasetEvent();

  /** Handle timed event */
  virtual void
  handleTimedEvent();

  /** Turn on algorithm heartbeat.  Call before execute to get periodic support.
   */
  virtual void
  setHeartbeat(size_t periodInSeconds);

  // Two ways of getting input.  By each record coming in (processNewData) and
  // by a
  // timed heartbeat pulse (no new data, output based on the incoming time)

  /** Process a matched new record (occurs as the records come in)  Index number
   * is index into declared order */
  virtual void
  processNewData(RAPIOData&) = 0;

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
  TimeDuration
  getMaximumHistory();

  /** Are we a realtime algorithm? */
  bool
  isRealtime()
  {
    return (myRealtime);
  }

protected:

  /** Init on start handles the latest record on execute */
  bool myInitOnStart;

  /** Heartbeat in seconds. A zero value means no heartbeat */
  size_t myHeartbeatSecs;

  /** Database of added input products we want from any index */
  std::vector<productInputInfo> myProductInputInfo;

  /** Database of added output products we can generate */
  std::vector<productOutputInfo> myProductOutputInfo;

  /** Database of added indexes we look for products in */
  std::vector<indexInputInfo> myIndexInputInfo;

  /** Indexes we are successfully attached to */
  std::vector<std::shared_ptr<IndexType> > myConnectedIndexes;

  /** Directory or output from "o" option */
  std::string myOutputDir;

  /** Notifier path from "l" option */
  std::string myNotifierPath;

  /** Pointer to active notifier, if any */
  std::shared_ptr<RecordNotifier> myNotifier;

  /** History time for index storage */
  TimeDuration myMaximumHistory;

  /** The realtime mode */
  bool myRealtime;
};

//  end class RAPIOAlgorithm
}
