#pragma once

#include <rAlgorithm.h>
#include <rRAPIOOptions.h>
#include <rRAPIOData.h>

namespace rapio {
class RAPIOPlugin;
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

  // Public intended open API -----------------------

  /** Declare extra command line plugins */
  virtual void
  declarePlugins(){ };

  /** Declare extra options program will use */
  virtual void
  declareOptions(RAPIOOptions& o){ };

  /** Declare advanced help for options in declareOptions, if any. */
  virtual void
  declareAdvancedHelp(RAPIOOptions& o){ };

  /** Gather flags, etc. from the parsed options */
  virtual void
  processOptions(RAPIOOptions& o){ };

  /** Initialize system wide base parsers like XML/JSON
   * these are typically critical for initial setup */
  virtual void
  initializeBaseParsers();

  /** Initialize any base modules requiring configuration */
  virtual void
  initializeBaseline(){ };

  /** After adding wanted inputs and indexes, execute the algorithm */
  virtual void
  execute();

  /** Initialize program from c arguments and execute. */
  virtual void
  executeFromArgs(int argc,
    char *            argv[]);

  /** Post loaded help.  Algorithms should use declareAdvancedHelp */
  virtual void
  addPostLoadedHelp(RAPIOOptions& o){ };

  /** Add a plugin (called by RAPIOPlugin to register) */
  void
  addPlugin(RAPIOPlugin * p)
  {
    myPlugins.push_back(p);
  }

  // Callback hooks

  /** Callback for data ingest */
  virtual void
  processNewData(rapio::RAPIOData& d){ };

  /** Callback for heartbeat based plugins
   * @param n The actual now time triggering the event.
   * @param p The pinned sync time we're firing for. */
  virtual void
  processHeartbeat(const Time& n, const Time& p){ };

  /** Callback for web plugins. Process a web request message when
   * running with a REST webserver mode */
  virtual void
  processWebMessage(std::shared_ptr<WebMessage> message){ };

protected:

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

  /** Was the macro applied on leftovers? */
  bool
  isMacroApplied(){ return myMacroApplied; }

  /** Clean up plugins */
  // ~RAPIOProgram(){
  //   for(auto p: myPlugins){
  //     delete p;
  //   }
  // }

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
