#pragma once

/** RAPIO API */
#include <RAPIO.h>

// You 'could' declare RAPIO namespace here and
// avoid the rapio:: stuff below, but if you're gonna mix
// with other code such as WDSS2 might explicitly declare
// using namespace rapio;

namespace wdssii { // or whatever you want
/** Create your algorithm as a subclass of RAPIOAlgorithm */
class W2SimpleAlg : public rapio::RAPIOAlgorithm {
public:

  /** Create an example simple algorithm */
  W2SimpleAlg(){ };

  // The basic API messages from the system

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Process a new record/datatype.  See the .cc for RAPIOData info */
  virtual void
  processNewData(rapio::RAPIOData& d) override;

  /** Process heartbeat trigger from 'sync option.
   * Note: Do something on a trigger.  For example, you might gather
   * RadialSets as they come in live 'processNewData', and then every 10
   * minutes you write out a product of average or something.
   * @param at The actual now time triggering the event.
   * @param sync The pinned sync time we're firing for. */
  virtual void
  processHeartbeat(const rapio::Time& n, const rapio::Time& p) override;

protected:

  // Keep/set your options from processOptions if you need to use them.

  /** My test example optional string parameter */
  std::string myTest;

  /** My test example optional boolean parameter */
  bool myX;

  /** My test example REQUIRED stringparameter */
  std::string myZ;
};
}
