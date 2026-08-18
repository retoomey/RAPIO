#include "rTimeHeightGrid.h"
#include "rConstants.h"

using namespace rapio;
using namespace std;

TimeHeightGrid::TimeHeightGrid() {
  setDataType("TimeHeightGrid");
}

std::shared_ptr<TimeHeightGrid>
TimeHeightGrid::Create(const std::string& TypeName,
                       const std::string& Units,
                       const LLH& location,
                       const Time& baseTime,
                       size_t num_times,
                       size_t num_heights) 
{
  auto grid = std::make_shared<TimeHeightGrid>();
  grid->init(TypeName, Units, location, baseTime, num_times, num_heights);
  return grid;
}

bool 
TimeHeightGrid::init(const std::string& TypeName,
                     const std::string& Units,
                     const LLH& location,
                     const Time& baseTime,
                     size_t num_times,
                     size_t num_heights) 
{
  // Initialize the base DataGrid with (Time, Height) dimensions
  DataGrid::init(TypeName, Units, location, baseTime, { num_times, num_heights }, { "Time", "Ht" });

  // Add the primary 2D data array, mapped to dimensions 0 (Time) and 1 (Ht)
  addFloat2D(Constants::PrimaryDataName, Units, { 0, 1 });

  // Setup the Time Coordinate Variable to CF-compliant standards
  std::string timeUnit = "seconds since " + baseTime.getString("%Y-%m-%d %H:%M:%S UTC");
  addFloat1D("Time", timeUnit, { 0 });

  // Setup the Height Coordinate Variable to CF-compliant standards
  addFloat1D("Ht", "meters", { 1 });

  return true;
}

void 
TimeHeightGrid::setTimeDelta(size_t timeIndex, float secondsSinceBase) 
{
  auto array = getFloat1D("Time");
  if (array) {
    auto& r = array->ref();
    r[timeIndex] = secondsSinceBase;
  }
}

void 
TimeHeightGrid::setHeightLevel(size_t heightIndex, float heightMeters) 
{
  auto array = getFloat1D("Ht");
  if (array) {
    auto& r = array->ref();
    r[heightIndex] = heightMeters;
  }
}
