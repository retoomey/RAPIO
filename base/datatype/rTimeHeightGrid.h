#pragma once
#include <rDataGrid.h>
#include <rTime.h>
#include <rLLH.h>

namespace rapio {
/**
 * @class TimeHeightGrid
 * @brief Represents a 2D data grid structured with Time as the X-axis and Height as the Y-axis.
 *
 * This class is designed to be CF-compliant for NetCDF output. By setting the dimensions
 * specifically to "Time" and "Ht" and populating them as 1D coordinate variables,
 * downstream visualization tools can automatically render Time-Height cross sections
 * (such as meteograms or ORPG-style RDQVP plots) without manual configuration.
 *
 * @author Robert Toomey
 */
class TimeHeightGrid : public DataGrid {
public:
  TimeHeightGrid();

  /**
   * @brief Factory method to allocate a new TimeHeightGrid.
   *
   * @param TypeName The name of the data type (e.g., "RDQVP_WindSpeed").
   * @param Units The physical units of the primary 2D data (e.g., "m/s").
   * @param location The geographic location (Lat/Lon) of the profile.
   * @param baseTime The anchor time for the grid. The Time axis will be deltas from this.
   * @param num_times The size of the Time dimension (X-axis).
   * @param num_heights The size of the Height dimension (Y-axis).
   * @return std::shared_ptr<TimeHeightGrid> A pointer to the newly allocated grid.
   */
  static std::shared_ptr<TimeHeightGrid>
  Create(const std::string& TypeName,
    const std::string     & Units,
    const LLH             & location,
    const Time            & baseTime,
    size_t                num_times,
    size_t                num_heights);

  /** Public API for users to clone a TimeHeightGrid */
  std::shared_ptr<TimeHeightGrid>
  Clone() const;

  /** Get number of times for set */
  size_t
  getNumTimes() const
  {
    return myDims.size() > 0 ? myDims[0].size() : 0;
  };

  /** Get number of heights for set */
  size_t
  getNumHeights() const
  {
    return myDims.size() > 1 ? myDims[1].size() : 0;
  };

  /**
   * @brief Sets the delta time for a specific index on the Time axis.
   *
   * @param timeIndex The index along the Time dimension.
   * @param secondsSinceBase The number of seconds elapsed since the baseTime.
   */
  void
  setTimeDelta(size_t timeIndex, float secondsSinceBase);

  /**
   * @brief Sets the altitude for a specific index on the Height axis.
   *
   * @param heightIndex The index along the Height dimension.
   * @param heightMeters The altitude in meters above mean sea level.
   */
  void
  setHeightLevel(size_t heightIndex, float heightMeters);

protected:

  /**
   * @brief Internal initialization routine to set up dimensions and CF-compliant metadata.
   *
   * @param TypeName The name of the data type.
   * @param Units The physical units of the primary data.
   * @param location The geographic location.
   * @param baseTime The anchor time for the grid.
   * @param num_times The size of the Time dimension.
   * @param num_heights The size of the Height dimension.
   * @return true if initialization was successful, false otherwise.
   */
  bool
  init(const std::string& TypeName,
    const std::string   & Units,
    const LLH           & location,
    const Time          & baseTime,
    size_t              num_times,
    size_t              num_heights);

  /** Deep copy our fields to a new subclass */
  void
  deep_copy(const std::shared_ptr<TimeHeightGrid>& n) const;
};
} // namespace rapio
