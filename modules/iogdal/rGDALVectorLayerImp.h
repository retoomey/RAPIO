#pragma once
#include <rVectorDataType.h>
#include "rGDALSharedContext.h"
#include <memory>
#include <string>
#include <vector>

namespace rapio {
class GDALVectorLayerImp : public VectorDataType {
public:
  GDALVectorLayerImp(std::shared_ptr<GDALSharedContext> context, const std::string& layerName);
  virtual
  ~GDALVectorLayerImp() = default;

  virtual std::string
  getGeoJSON() override;

  virtual std::string
  getTileMVT(double minLon, double minLat, double maxLon, double maxLat, int z, int x, int y) override;
  virtual std::string
  getTileGeoJSON(double minLon, double minLat, double maxLon, double maxLat) override;

  // Metadata Accessors
  virtual std::string
  getLayerName() const override { return myLayerName; }

  virtual std::string
  getGeometryType() const override { return myGeomType; }

  virtual long long
  getFeatureCount() const override { return myFeatureCount; }

  virtual bool
  getExtent(double& minLon, double& minLat, double& maxLon, double& maxLat) const override;
  virtual std::vector<std::string>
  getFieldNames() const override { return myFieldNames; }

private:
  std::shared_ptr<GDALSharedContext> myContext;

  // Cached Metadata
  std::string myLayerName;
  std::string myGeomType;
  long long myFeatureCount;
  bool myHasExtent;
  double myMinX, myMinY, myMaxX, myMaxY;
  std::vector<std::string> myFieldNames;
};
}
