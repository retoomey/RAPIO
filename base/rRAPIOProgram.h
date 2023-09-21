#pragma once

#include <rAlgorithm.h>
#include <rRAPIOOptions.h>

namespace rapio {
class RAPIOPlugin;

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
  RAPIOProgram() : myMacroApplied(false){ };

  // Public intended open API -----------------------

  /** Initialize system wide base parsers like XML/JSON
   * these are typically critical for initial setup */
  virtual void
  initializeBaseParsers();

  /** Initialize any base modules requiring configuration */
  virtual void
  initializeBaseline(){ };

  /** Declare command line plugins */
  virtual void
  declarePlugins(){ };

  /** Declare all options */
  virtual void
  declareOptions(RAPIOOptions& o){ };

  /** Declare advanced help */
  virtual void
  declareAdvancedHelp(RAPIOOptions& o){ };

  /** Process/setup from the given options */
  virtual void
  processOptions(RAPIOOptions& o){ };

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

protected:

  /** Declare all default options for this layer,
   * typically you don't need to change at this level */
  virtual RAPIOOptions
  initializeOptions();

  /** Finalize options by processing, etc.,
   * typically you don't need to change at this level */
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
  bool myMacroApplied;

  /** List of plugins we own */
  std::vector<RAPIOPlugin *> myPlugins;
};
}
