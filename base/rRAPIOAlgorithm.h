#pragma once

#include <rRAPIOProgram.h>
#include <rAlgorithm.h>
#include <rRAPIOOptions.h>
#include <rRAPIOData.h>
#include <rIndexType.h>
#include <rConfigParamGroup.h>

#include <string>
#include <vector>

namespace rapio {
class WebMessage;

/**
 *  The stock default algorithm and all its options and processing
 *
 *  @author Robert Toomey
 *
 */
class RAPIOAlgorithm : public RAPIOProgram {
public:
  /** Construct a stock algorithm */
  RAPIOAlgorithm(const std::string& display = "Algorithm") : RAPIOProgram(display){ };

  // Public intended open API -----------------------

  /** Process a matched new record (occurs as the records come in)  Index number
   * is index into declared order */
  virtual void
  processNewData(RAPIOData&);

  /** Process a web request message when running with a REST webserver mode */
  virtual void
  processWebMessage(std::shared_ptr<WebMessage> message) override;

  // End Public intended open API -----------------------

  /** Post load, advanced help request.  This help requires the system to be initialized */
  virtual void
  addPostLoadedHelp(RAPIOOptions& o) override;

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

  /** Initialize system wide setup */
  virtual void
  initializeBaseline() override;

  /** Should write given product key?
   * Algorithms can write products such as 'Reflectivity' but
   * multiple instances may require changing the name using the -O
   * ability.  This will return the changed productName to write out,
   * So if -O="Reflectivity=myReflectivity", myReflectivity is returned.
   */
  virtual bool
  isProductWanted(const std::string& key,
    std::string                    & productName);

  /** Set up initial indexes and initial loading */
  virtual void
  setUpInitialIndexes();

  /** Set up the record queue */
  virtual void
  setUpRecordQueue();

  /** After adding wanted inputs and indexes, execute the algorithm */
  virtual void
  execute() override;

  /** Handle new record, usually from event queue */
  virtual void
  handleRecordEvent(const Record& rec);

  /** Handle end of event index event (sent by archives) */
  virtual void
  handleEndDatasetEvent();

  /** Write data based on suffix directly to a given file key,
   * without notification.  You normally want to call writeOutputProduct
   * which will autogenerate file names, multi-output and notify, etc.
   * I'm using this for tiles at moment..I may refactor these two write
   * functions at some point
   */
  virtual bool
  writeDirectOutput(const URL         & path,
    std::shared_ptr<DataType>         outputData,
    std::map<std::string, std::string>& outputParams);

  /** Write data to given key.  Key must exist/match the keys from
   * addOutputProduct */
  virtual void
  writeOutputProduct(const std::string& key,
    std::shared_ptr<DataType>         outputData,
    std::map<std::string, std::string>& outputParams);

  /** Write data to given key.  Key must exist/match the keys from
   * addOutputProduct */
  virtual void
  writeOutputProduct(const std::string& key,
    std::shared_ptr<DataType>         outputData)
  {
    std::map<std::string, std::string> outputParams;

    writeOutputProduct(key, outputData, outputParams);
  }

  /** Get the maximum history specified by user */
  static TimeDuration
  getMaximumHistory();

  /** Is given time in the time window from -h */
  static bool
  inTimeWindow(const Time& aTime);

  /** Are we a daemon algorithm? For example, waiting on realtime data. */
  bool
  isDaemon();

  /** Are we reading old records? */
  bool
  isArchive();

  /** Are we running a web server? */
  bool
  isWebServer();

protected:

  /** Indexes we are successfully attached to */
  std::vector<std::shared_ptr<IndexType> > myConnectedIndexes;

  /** Hold the "n" list of notifiers */
  std::string myNotifierList;

  /** Hold the postwrite command, if any */
  std::string myPostWrite;

  /** History time for index storage */
  static TimeDuration myMaximumHistory;

  /** The time of last record received */
  static Time myLastDataTime;

  /** The record read mode */
  std::string myReadMode;

protected:

  /** Declare all default plugins for this class layer,
   * typically you don't need to change at this level.
   * @see declarePlugins */
  virtual void
  initializePlugins() override;

  /** Declare all default options for this class layer,
   * typically you don't need to change at this level.
   * @see declareOptions */
  virtual void
  initializeOptions(RAPIOOptions& o) override;

  /** Finalize options by processing, etc.,
   * typically you don't need to change at this level.
   * @see processOptions */
  virtual void
  finalizeOptions(RAPIOOptions& o) override;
};

//  end class RAPIOAlgorithm
}
