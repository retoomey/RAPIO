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
 *  @author Robert Toomey
 *
 *  FIXME: Clean up protect/public.  Right now they should be sticking
 *  to the functions declared in the example algorithm.
 */
class RAPIOAlgorithm : public Algorithm {
protected:

  /** Store database of product information before creation */
  class productInputInfo : public Algorithm {
public:
    /* Constructor to make sure no fields missed */
    productInputInfo(const std::string& n, const std::string& s)
      : name(n), subtype(s){ }

    std::string name;
    std::string subtype;
  };

  /** Store database of output information before creation */
  class productOutputInfo : public Algorithm {
public:
    /* Constructor to make sure no fields missed */
    productOutputInfo(const std::string& p, const std::string& s,
      const std::string& toP, const std::string& toS)
      : product(p), subtype(s), toProduct(toP), toSubtype(toS){ }

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
    /* Constructor to make sure no fields missed */
    indexInputInfo(const std::string& p, const std::string& i,
      const TimeDuration& h) : protocol(p), indexparams(i), maximumHistory(h){ }

    std::string protocol;
    std::string indexparams;
    TimeDuration maximumHistory;
  };

  /** Store -o output information for the writers */
  class outputInfo : public Algorithm {
public:
    /* Constructor to make sure no fields missed */
    outputInfo(const std::string& f, const std::string o)
      : factory(f), outputinfo(o){ }

    std::string factory;
    std::string outputinfo;
  };

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

  /** Does this record match a pattern on the -I? */
  virtual int
  matches(const Record& rec);

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
  TimeDuration
  getMaximumHistory();

  /** Are we a daemon algorithm? For example, waiting on realtime data. */
  bool
  isDaemon();

  /** Are we reading old records? */
  bool
  isArchive();

protected:

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

  /** Hold the "n" list of notifiers */
  std::string myNotifierList;

  /** Notifiers we are sending notification of new records to */
  std::vector<std::shared_ptr<RecordNotifierType> > myNotifiers;

  /** History time for index storage */
  TimeDuration myMaximumHistory;

  /** The record read mode */
  std::string myReadMode;

  /** The cronlist for heartbeat/sync if any */
  std::string myCronList;

  /** The list of writers to attempt */
  std::vector<outputInfo> myWriters;
};

//  end class RAPIOAlgorithm
}
