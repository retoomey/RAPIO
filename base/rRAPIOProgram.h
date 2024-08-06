#pragma once

#include <rAlgorithm.h>
#include <rRAPIOOptions.h>
#include <rRAPIOData.h>
#include <rRAPIOPlugin.h>

namespace rapio {
class WebMessage;

/**
 *  The stock default program and all its options and processing
 *  This is used for simple tools, etc. where all the abilities
 *  of algorithms aren't needed.
 *
 *  @author Robert Toomey
 *
 */
class RAPIOProgram : public Algorithm {
public:

  /** Construct a stock program */
  RAPIOProgram(const std::string& display = "Program") : myMacroApplied(false), myDisplayClass(display){ };

  /**
   * @name Public API
   * Methods you typically override in order to create you own program/algorithm
   * @{
   */

  /** Declare extra command line plugins */
  virtual void
  declarePlugins(){ };

  /** Declare extra options program will use */
  virtual void
  declareOptions(RAPIOOptions& o){ };

  /** Gather flags, etc. from the parsed options */
  virtual void
  processOptions(RAPIOOptions& o){ };

  /** Callback for data ingest */
  virtual void
  processNewData(rapio::RAPIOData& d)
  {
    LogDebug("Received data callback, ignoring.\n");
  }

  /** Callback for heartbeat based plugins
   * @param n The actual now time triggering the event.
   * @param p The pinned sync time we're firing for. */
  virtual void
  processHeartbeat(const Time& n, const Time& p)
  {
    LogDebug("Received heartbeat callback, ignoring.\n");
  }

  /** Callback for web plugins. Process a web request message when
   * running with a REST webserver mode */
  virtual void
  processWebMessage(std::shared_ptr<WebMessage> message)
  {
    LogDebug("Received web callback, ignoring.\n");
  }

  /** Initialize program from c arguments and execute.
   * Typically you call this in main to run your program.
   */
  virtual void
  executeFromArgs(int argc,
    char *            argv[]);

  /** @} */

  /**
   * @name Plugin API
   * Methods called by a subclass of RAPIOPlugin to add command line ablilities
   * @{
   */

  /** Add a plugin (called by RAPIOPlugin to register) */
  void
  addPlugin(RAPIOPlugin * p);

  /** Remove a plugin (called by RAPIOPlugin to deregister) */
  void
  removePlugin(const std::string& name);

  /** Get back a plugin type for use or nullptr if plugin doesn't exist. */
  template <typename T>
  T *
  getPlugin(const std::string& name) const
  {
    for (RAPIOPlugin * p:myPlugins) {
      if (p->getName() == name) {
        T * derived = dynamic_cast<T *>(p);
        return derived;
      }
    }
    return nullptr;
  }

  /** @} */

  /** Was the macro applied on leftovers? */
  bool
  isMacroApplied() const { return myMacroApplied; }

  /** Are we running a web server? */
  bool
  isWebServer(const std::string& key = "web") const;

protected:

  /** Initialize system wide base parsers like XML/JSON
   * these are typically critical for initial setup */
  virtual void
  initializeBaseParsers();

  /** Initialize any base modules requiring configuration */
  virtual void
  initializeBaseline();

  /** Declare all default plugins for this class layer,
   * typically you don't need to change at this level.
   * @see declarePlugins */
  virtual void
  initializePlugins();

  /** Declare all default options for this class layer,
   * typically you don't need to change at this level.
   * @see declareOptions */
  virtual void
  initializeOptions(RAPIOOptions& o);

  /** Finalize options by processing, etc. for this layer,
   * typically you don't need to change at this level
   * @see processOptions */
  virtual void
  finalizeOptions(RAPIOOptions& o);

  /** Declare advanced help for options in declareOptions, if any. */
  virtual void
  declareAdvancedHelp(RAPIOOptions& o){ };

  /** Post loaded help.  Algorithms should use declareAdvancedHelp */
  virtual void
  addPostLoadedHelp(RAPIOOptions& o){ };

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

  /** Clean up plugins */
  // ~RAPIOProgram(){
  //   for(auto p: myPlugins){
  //     delete p;
  //   }
  // }

  /** After adding wanted inputs and indexes, execute the algorithm */
  virtual void
  execute();

protected:

  /** Base class such as 'Program' or 'Algorithm'.  Used
   * in the first line of help typically. */
  std::string myDisplayClass;

  /** Has a macro been applied to the command line options? */
  bool myMacroApplied;

  /** List of plugins we own */
  std::vector<RAPIOPlugin *> myPlugins;
};
}
