#pragma once

#include "rIODataType.h"
#include "rLatLonGrid.h"

namespace rapio {

/**
 * Read/Write geospatial raster data (DEMs, GeoTIFFs, etc.) via GDAL.
 *
 * @author Your Name
 */
class GDALLatLonGrids : public IOSpecializer {
public:

  /** Read DataType with given keys */
  virtual std::shared_ptr<DataType>
  read(
    std::map<std::string, std::string>& keys,
    std::shared_ptr<DataType>         dt)
  override;

  /** Write DataType with given keys */
  virtual bool
  write(
    std::shared_ptr<DataType>         dt,
    std::map<std::string, std::string>& keys)
  override;

  /** Do the heavy work of reading the grid using GDAL */
  static std::shared_ptr<DataType>
  readGDALGrid(const std::string& filepath);

  /** Initial introduction of GDALLatLonGrids specializer to the IO factory */
  static void
  introduceSelf(IODataType * owner);
};

} // namespace rapio
