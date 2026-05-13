#pragma once

/** RAPIO API (If your algorithm is EXTERNAL to the rapio repo,
 * you can use this header.  If part of the repo, use the individual
 * headers for compiling speed.  So for example, your alg is
 * checked into WDSSII or HYDRO you should use the RAPIO.h header
 * only in your code.  This will keep you independent of changes. */
// #include <RAPIO.h>
#include <rRAPIOAlgorithm.h>

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
class SimpleProgram : public RAPIOProgram {
public:

  /** Create a program */
  SimpleProgram(){ };

  /** Declare all program options */
  virtual void
  declareOptions(RAPIOOptions& o) override;

  /** Process all program options */
  virtual void
  processOptions(RAPIOOptions& o) override;

  /** Do what you want to do */
  virtual void
  execute() override;

protected:

  /** Optional flag */
  std::string myTest;
};
}
