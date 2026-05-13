#pragma once

#include <rPolarAlgorithm.h>

namespace rapio { 
/** Create your algorithm as a subclass of RAPIOAlgorithm */
class rDRradial : public PolarAlgorithm {
public:

// The basic API messages from the system

  /** Declare all algorithm options */
  virtual void
  declareOptions(RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(RAPIOOptions& o) override;

  /** Process a new record/datatype.  See the .cc for RAPIOData info */
  /* This is the data wrapper, read data, load data, send data, write data */
  /* No science in here please */
  virtual void
  processNewData(RAPIOData& d) override;

  /** Process heartbeat trigger from 'sync option.
   * Note: Do something on a trigger.  For example, you might gather
   * RadialSets as they come in live 'processNewData', and then every 10
   * minutes you write out a product of average or something.
   * @param at The actual now time triggering the event.
   * @param sync The pinned sync time we're firing for. */
  virtual void
  processHeartbeat(const Time& n, const Time& p) override;

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
  processDRradial(std::map<std::string, std::shared_ptr<RadialSet>> & DataMap);

protected:

  // Keep/set your options from processOptions if you need to use them.

  /** My test example optional string parameter */
  std::string myTest;

  /** My test example optional boolean parameter */
  bool myX;

  /** My test example REQUIRED stringparameter */
  std::string myZ;

  /** Track the current elevation we are collecting */
  float current_elevation = -9999.0; // Initialize to a "missing" value
  
  /** Constant for missing data comparison */
  const float MISSING_ELEV = -9999.0;

  /** Where we store the input data until we run */
  std::map<std::string, std::shared_ptr<RadialSet>> myDataMap; 
  
 
private:
  

};
}
