#pragma once
#include "rDataGrid.h"
#include "rIOConfig.h"
#include "rArray.h"
#include <vector>
#include <memory>
#include <array>
#include <string>

namespace rapio {
class SparseEncoder {
public:
  static bool
  sparse2D(DataGrid& grid, IOConfig& keys);
  static bool
  sparse3D(DataGrid& grid, IOConfig& keys);
  static bool
  sparse3DN(DataGrid& grid, IOConfig& keys, const std::vector<std::shared_ptr<Array<float, 2> > >& grids);

  static void
  unsparse2D(DataGrid& grid, size_t num_x, size_t num_y, IOConfig& keys,
    const std::string& pixelX     = "pixel_x", const std::string& pixelY = "pixel_y",
    const std::string& pixelCount = "pixel_count");

  static void
  unsparse3D(DataGrid& grid, size_t num_x, size_t num_y, size_t num_z, IOConfig& keys,
    const std::string& pixelX = "pixel_x", const std::string& pixelY = "pixel_y",
    const std::string& pixelZ     = "pixel_z",
    const std::string& pixelCount = "pixel_count");

  static void
  unsparseRestore(DataGrid& grid);

private:
  template <size_t N>
  static bool
  sparseT(DataGrid& grid, IOConfig& keys);

  template <size_t N>
  static void
  unsparseT(DataGrid& grid, IOConfig& keys, const std::array<std::string, N>& axisNames, const std::string& countName);
};
} // namespace rapio
