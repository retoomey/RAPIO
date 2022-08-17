#pragma once

/** RAPIO API */
#include <RAPIO.h>

namespace rapio {
/*
 * An example we'll try to maintain for creating
 * a general program which will have less default ability than
 * an algorithm.  The line where they divide will shift
 * as things are refactored.
 * The idea here is tools, etc. where you want to do things
 * but don't need the full power of a realtime processing
 * algorithm.
 *
 * @author Robert Toomey
 **/
class SimpleProgram : public rapio::RAPIOProgram {
public:

  /** Create a program */
  SimpleProgram(){ };

  /** Declare all program options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all program options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Do what you want to do */
  virtual void
  execute() override;

protected:

  /** Optional flag */
  std::string myTest;
};
}
