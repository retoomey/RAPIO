#pragma once

#include <rUtility.h>
#include <rRecord.h>

#include <string>

namespace rapio {
class RAPIOProgram;
class RAPIOOptions;

/** A program plugin interface for command line options.
 * The design idea here is to encapsulate abilities like heartbeat, etc. within
 * its own module along with its related member fields.
 *
 * Plugins currently call back to the program using virtual plug methods in RAPIOProgram
 *
 * This allows algorithms/programs to declare standard parameter abilities on demand.
 *
 * @author Robert Toomey
 */
class RAPIOPlugin : public Utility {
public:

  /** Declare plugin with unique name */
  RAPIOPlugin(const std::string& name) : myName(name), myActive(false){ }

  /** Get name of plugin */
  std::string
  getName()
  {
    return myName;
  }

  /** Return if plugin is active */
  virtual bool
  isActive()
  {
    return myActive;
  }

  /** Declare options used */
  virtual void
  declareOptions(RAPIOOptions& o)
  { }

  /** Declare advanced help for the plugin */
  virtual void
  addPostLoadedHelp(RAPIOOptions& o)
  { }

  /** Process our options */
  virtual void
  processOptions(RAPIOOptions& o)
  { }

  /** Setup/run the plugin */
  virtual void
  execute(RAPIOProgram * caller)
  { }

  /** Handle a post record event */
  virtual void
  postRecordEvent(RAPIOProgram * caller, const Record& rec)
  { }

protected:
  /** Name of plug, typically related to command line such as 'sync', 'web' */
  std::string myName;

  /** Simple flag telling if plugin is active/running, etc. */
  bool myActive;
};
}
