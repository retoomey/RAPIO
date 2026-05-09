#pragma once

#include <rGDALDataType.h>
#include <rGDALSharedContext.h>

#include <memory>

class GDALDataset;

namespace rapio {
/** DataType for holding GDAL catalog information.
 * GDAL can hold multiple things.  This allows a catalog
 * to be gathered and then grabbing a layer/raster
 * similar to grib2
 *
 * @author Robert Toomey */
class GDALDataTypeImp : public GDALDataType {
public:

  /** Create a GDAL DataType */
  GDALDataTypeImp(const std::string& filepath);

  /** Destroy a GDAL DataType */
  virtual
  ~GDALDataTypeImp();

  /** Read the GDAL catalog */
  virtual std::vector<GDALCatalogEntry>
  getCatalog() override;

  /** Get vector layer from name (in the gdal layers) */
  virtual std::shared_ptr<VectorDataType>
  getVectorLayer(const std::string& layerName) override;

private:
  /** Stores GDAL information */
  std::shared_ptr<GDALSharedContext> myContext;
};
}
