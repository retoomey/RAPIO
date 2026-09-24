#pragma once

#include <rDataType.h>
#include <rRecord.h>
#include <rDataTypeGroup.h>
#include <rRAPIOData.h>

#include <memory>

#include <vector>

namespace rapio {
/** Store a history of objects of some sort.
 * API meant to be called for incoming data of interest.
 *
 * The main algorithm purges the history cache based on the -h
 * history (by default).
 *
 * @ingroup rapio_data
 * @brief Base class for storing history of DataType.
 * @author Robert Toomey
 */
class DataTypeHistory {
public:

  /** Called automatically by the algorithm on new incoming records.
   * RAPIO core will call this to update the history */
  static void
  processNewData(RAPIOData& d);

  /** Purge history for volumes, collections, etc.  Called by
   * RAPIO automatically you don't need to call it.  Use the -h
   * option to determine max time window */
  static void
  purgeTimeWindow(const Time& time);

  /** Register a DataTypeGroup (like Volume or ProductGroup)
   * in order to receive the -h time window from the algorithm.
   * This allows groups to expire their data. */
  static void
  registerForPurging(std::shared_ptr<DataTypeGroup> group);

private:

  // Storage for registered groups that require time purging
  static std::vector<std::shared_ptr<DataTypeGroup> > myPurgeList;
};
}
