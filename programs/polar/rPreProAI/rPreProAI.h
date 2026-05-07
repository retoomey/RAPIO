#pragma once

/** RAPIO API */
#include <RAPIO.h>

// You 'could' declare RAPIO namespace here and
// avoid the rapio:: stuff below, but if you're gonna mix
// with other code such as WDSS2 might explicitly declare
// using namespace rapio;

namespace wdssii { // or whatever you want
/** Create rPreProAI algorithm as a subclass of RAPIOAlgorithm */
class rPreProAI : public rapio::PolarAlgorithm {
public:

  /** Create an example simple algorithm */
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

  /** The algorithm work function */

  /* assume myDataMap contains all the data needed. Filling the map and calling this function is
   * the job of processNewData() above */
  /* process this data and add new entries to the map for output products */
  void
  processPreProAI(std::map<std::string, std::shared_ptr<rapio::RadialSet> > & DataMap);

protected:

  // Keep/set your options from processOptions if you need to use them.
  /** boolean optional string parameter */
  bool qc_option = false;

  /** Track the current elevation we are collecting */
  float current_elevation = -9999.0; // Initialize to a "missing" value

  /** Constant for missing data comparison */
  const float MISSING_ELEV = -9999.0;

  /** Where we store the input data until we run */
  std::map<std::string, std::shared_ptr<rapio::RadialSet> > myDataMap;


private:
};
}
