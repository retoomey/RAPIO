#pragma once

#include <rDataType.h>
#include <rDataGrid.h>
#include <rVectorDataType.h>

namespace rapio {
/** A simple struct to return layer metadata to the WebGUI or Algorithm */
struct GDALCatalogEntry {
  std::string name;
  std::string type; // "Vector" or "Raster"

  // New Smart Fields
  std::string geometryType; // e.g., "Polygon", "Point", "Float32" (for rasters)
  long long   count;        // Feature count (Vectors) or Band index (Rasters)

  // Bounding Box (Extent)
  bool        hasExtent;
  double      minLon, minLat, maxLon, maxLat;

  std::string
  description()
  {
    return fmt::format("{} {} {} {} {} {} {} {} {}",
             name, type, geometryType, count, hasExtent, minLon, minLat, maxLon, maxLat);
  }
};

/** DataType for holding a GDAL database.
 * Interface for implementations of GDAL to
 * implement.  This lazy loads parts of say
 * large GDAL files.
 *
 * @author Robert Toomey */
class GDALDataType : public DataType {
public:
  /** Create a GDAL DataType */
  GDALDataType()
  {
    myDataType = "GDALData";
  }

  /** Destroy a GDAL DataType */
  virtual
  ~GDALDataType() = default;

  /** The Catalog Method scans the GDAL data for info */
  virtual std::vector<GDALCatalogEntry>
  getCatalog() = 0;

  /** Get a vector layer, image or LatLonGrid, etc. (depends on geometric availability) */
  virtual std::shared_ptr<DataType>
  getLayer(const std::string& layerName) = 0;
};
}
