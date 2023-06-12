#pragma once

#include "rDataType.h"

#include "rLatLonGrid.h"
#include "rLatLonHeightGrid.h"

#include <vector>

namespace rapio {
/** LLHGridN2D
 *
 *  Basically implement a LatLonHeightGrid as a collection of 2D layers vs a 3D cube.
 *  This can be faster vs a 3D array due to memory management.
 *
 * @author Robert Toomey
 */
class LLHGridN2D : public LatLonHeightGrid
{
public:

  /** Construct uninitialized LLHGridN2D for STL and readers */
  LLHGridN2D(){ };

  /** Public API for users to create a single band LLHGridN2D quickly */
  static std::shared_ptr<LLHGridN2D>
  Create(
    const std::string& TypeName,
    const std::string& Units,
    const LLH        & northwest,
    const Time       & gridtime,
    float            lat_spacing,
    float            lon_spacing,
    size_t           num_lats,
    size_t           num_lons,
    size_t           num_levels);

  /** Return grid number i, lazy creation */
  std::shared_ptr<LatLonGrid>
  get(size_t i);

  /** Initialize a LLHGridN2D */
  bool
  init(
    const std::string& TypeName,
    const std::string& Units,
    const LLH        & northwest,
    const Time       & gridtime,
    float            lat_spacing,
    float            lon_spacing,
    size_t           num_lats,
    size_t           num_lons,
    size_t           num_levels);

protected:

  /** The set of LatLonGrids */
  std::vector<std::shared_ptr<LatLonGrid> > myGrids;
};
}