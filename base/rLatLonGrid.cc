#include "rLatLonGrid.h"

using namespace rapio;
using namespace std;

std::string
LatLonGrid::getGeneratedSubtype() const
{
  return (formatString(myLocation.getHeightKM(), 5, 2));
}

LatLonGrid::LatLonGrid(

  // Projection information (metadata of the 2D)
  const LLH   & location,
  const Time  & time,
  const float lat_spacing,
  const float lon_spacing,

  // Basically the 2D array and initial value
  size_t      rows,
  size_t      cols,
  float       value
)
{
  // Lookup for read/write factories
  myDataType = "LatLonGrid";

  init(location, time, lat_spacing, lon_spacing);

  /** Push back primary band.  This is the primary moment
   * of the LatLonGrid set */
  addFloat2D("primary", cols, rows, value);
}

/** Set what defines the lat lon grid in spacetime */
void
LatLonGrid::init(
  const LLH& location,
  const Time& time,
  const float lat_spacing, const float lon_spacing)
{
  myLocation   = location;
  myTime       = time;
  myLatSpacing = lat_spacing;
  myLonSpacing = lon_spacing;

  bool okLat = (myLatSpacing > 0);
  bool okLon = (myLonSpacing > 0);

  if (!(okLat && okLon)) {
    LogSevere("*** WARNING *** non-positive element in grid spacing\n"
      << "(" << lat_spacing << "," << lon_spacing << ")\n");
  }
}
