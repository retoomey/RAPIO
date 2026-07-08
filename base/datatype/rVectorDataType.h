#pragma once
#include <rDataType.h>
#include <string>

namespace rapio {
/**
 * Starting with a vector datatype for gdal vector tiles
 * Probably will change design if we add direct rasters
 *
 * Not all vectors would be geometric so will refactor
 *
 * @author Robert Toomey
 */
class VectorDataType : public DataType {
public:
  VectorDataType()
  {
    myDataType = "VectorData";
  }

  virtual
  ~VectorDataType() = default;

  /** Get a full geojson. */
  virtual std::string
  getGeoJSON() = 0;

  /** Get a MVT vector tile (smaller than json) */
  virtual std::string
  getTileMVT(double minLon, double minLat, double maxLon, double maxLat, int z, int x, int y) = 0;

  /** Get a JSON vector tile */
  virtual std::string
  getTileGeoJSON(double minLon, double minLat, double maxLon, double maxLat) = 0;

  // --- Layer Metadata ---

  /** Get the name of the layer */
  virtual std::string
  getLayerName() const = 0;

  /** Get the geometry type of the layer */
  virtual std::string
  getGeometryType() const = 0;

  /** Get the number of features */
  virtual long long
  getFeatureCount() const = 0;

  /** Returns true if the layer has a defined bounding box */
  virtual bool
  getExtent(double& minLon, double& minLat, double& maxLon, double& maxLat) const = 0;

  /** Returns a list of the attribute columns (e.g., "STATE_NAME", "POPULATION") */
  virtual std::vector<std::string>
  getFieldNames() const = 0;
};
}
