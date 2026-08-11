#pragma once

/** RAPIO API */
#include <RAPIO.h>

// You 'could' declare RAPIO namespace here and
// avoid the rapio:: stuff below, but if you're gonna mix
// with other code such as WDSS2 might explicitly declare
// using namespace rapio;
namespace rapio {
/** Create your algorithm as a subclass of RAPIOAlgorithm */
class LTAR : public rapio::PolarAlgorithm {
public:

// The basic API messages from the system

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Process a new record/datatype.  See the .cc for RAPIOData info */
  /* This is the data wrapper, read data, load data, send data, write data */
  /* No science in here please */
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
  /* assume myDataMap contains all the data needed. 
   * Filling the map and calling this function is the job of processNewData()
   *
   * Process this data and add new entries to the map for output products 
   *  (all your science goes in here)
   * 
   * Writing this new data is the job of processNewData()
   */
  void 
  processLTAR(std::map<std::string, std::shared_ptr<rapio::RadialSet>> & DataMap);

protected:

  /** we need to know the radar id we are workig with (not really, but more standard this way) */
  std::string radar_name;

  /** The individual gate sums of the reflectivity data */
  std::shared_ptr<rapio::RadialSet> Refl_sum; 
  
  /** Constant for missing data comparison */
  int num_elevs = 0;

  /** Where we store the input and output data */
  std::map<std::string, std::shared_ptr<rapio::RadialSet>> myDataMap; 
  
 
private:
  

};
}
