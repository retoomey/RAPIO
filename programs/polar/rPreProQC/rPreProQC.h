#pragma once

/** RAPIO API */
#include <RAPIO.h>

// You 'could' declare RAPIO namespace here and
// avoid the rapio:: stuff below, but if you're gonna mix
// with other code such as WDSS2 might explicitly declare
// using namespace rapio;

namespace wdssii { // or whatever you want
/** Create rPreProQC algorithm as a subclass of RAPIOAlgorithm */
class rPreProQC : public rapio::PolarAlgorithm {
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
  //virtual void
  //processHeartbeat(const rapio::Time& n, const rapio::Time& p) override;

  /** The algorithm work function */

  /* assume myDataMap class variable contains all the data needed. 
   * Filling the map and calling this function is
   * the job of processNewData() above */
  /* process this data and add new entries to the map for output products */
  void
  processPreProQC();

protected:
  //options
  std::string ltar_dir;
  //std::string terrain_dir;
  //bool apply_QC = false;
  std::string radar_name = "UNKN";
  //To store the LTAR data. That way we only read it once. 
  //LTAR has an elevation limit to it's applcation. 
  // it should only be applied at the elevation it was collected ( or possibly below)
  // Currently LTAR data is collected only at 0.5 elevation
  std::shared_ptr<rapio::RadialSet> LTAR;

  /** Track the current elevation we are collecting */
  float current_elevation = -9999.0; // Initialize to a "missing" value

  /** Constant for missing data comparison */
  const float MISSING_ELEV = -9999.0;

  /** Where we store the input data until we run */
  std::map<std::string, std::shared_ptr<rapio::RadialSet> > myDataMap;

private:
};
}
