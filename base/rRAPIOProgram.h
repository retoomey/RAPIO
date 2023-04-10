#pragma once

#include <rAlgorithm.h>
#include <rRAPIOOptions.h>

namespace rapio {
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

  /** Declare all options */
  virtual void
  declareOptions(RAPIOOptions& o){ };

  /** Declare command line plugins */
  virtual void
  declarePlugins(){ };

  /** Process/setup from the given options */
  virtual void
  processOptions(RAPIOOptions& o){ };

  /** Post load, advanced help request.  This help requires the system to be initialized */
  virtual void
  addPostLoadedHelp(RAPIOOptions& o){ };

  /** After adding wanted inputs and indexes, execute the algorithm */
  virtual void
  execute();

  /** Initialize program from c arguments and execute. */
  virtual void
  executeFromArgs(int argc,
    char *            argv[]);

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

protected:
  bool myMacroApplied;
};
}
