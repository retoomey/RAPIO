#include "rGDALDataTypeImp.h"
#include "rGDALVectorLayerImp.h"
#include "rIOGDAL.h"

#include "rLatLonGrid.h"
#include "rImageDataType.h"

// GDAL C++ library
#include "gdal_priv.h"

// GDAL simple features
#include "ogr_api.h"
#include "ogrsf_frmts.h"
#include "ogr_p.h"

using namespace rapio;

GDALDataTypeImp::GDALDataTypeImp(const std::string& filepath)
{
  myDataType = "GDALData";
  GDALAllRegister();

  // Keep the dataset open for the lifetime of this object
  // We're read only so this should be safe and faster
  myContext = std::make_shared<GDALSharedContext>();
  myContext->dataset = static_cast<GDALDataset *>(GDALOpenEx(
      filepath.c_str(),
      GDAL_OF_RASTER | GDAL_OF_VECTOR | GDAL_OF_READONLY,
      nullptr, nullptr, nullptr
  ));
}

GDALDataTypeImp::~GDALDataTypeImp()
{ }

std::shared_ptr<DataType>
GDALDataTypeImp::getLayer(const std::string& layerName)
{
  std::lock_guard<std::recursive_mutex> lock(myContext->mutex);

  if (!myContext->dataset) { return nullptr; }

  // 1. Check Vector
  OGRLayer * poLayer = myContext->dataset->GetLayerByName(layerName.c_str());

  if (poLayer != nullptr) {
    return std::make_shared<GDALVectorLayerImp>(myContext, layerName);
  }

  // 2. Check Raster
  int numBands = myContext->dataset->GetRasterCount();

  for (int i = 1; i <= numBands; ++i) {
    GDALRasterBand * poBand = myContext->dataset->GetRasterBand(i);
    std::string desc        = poBand->GetDescription();
    std::string bName       = desc.empty() ? "Band_" + std::to_string(i) : desc;

    if (bName == layerName) {
      double geoTransform[6];
      bool hasGeo = (myContext->dataset->GetGeoTransform(geoTransform) == CE_None);

      int numX = poBand->GetXSize();
      int numY = poBand->GetYSize();

      if (hasGeo) {
        // --- GEOMETRY PRESENT: Build LatLonGrid ---
        fLogInfo("GDAL: Returning LatLonGrid for '{}'", layerName);

        // ... (Insert the LatLonGrid creation code we discussed previously) ...
      } else {
        // --- NO GEOMETRY: Build ImageDataType ---
        fLogInfo("GDAL: Returning ImageDataType for '{}'", layerName);

        // Create a 1-channel image grid
        auto imageObj = ImageDataType::Create(layerName, numX, numY, 1);

        // Get the underlying boost::multi_array reference
        auto array = imageObj->getByte3D(Constants::PrimaryDataName);
        auto& data = array->ref();

        // Read directly into the contiguous memory block of the multi_array
        // data.data() points to the raw 1D block representing [Y][X][C]
        CPLErr err = poBand->RasterIO(GF_Read,
            0, 0, numX, numY,
            data.data(), numX, numY,
            GDT_Byte, // Force byte extraction
            0, 0);

        if (err != CE_None) {
          fLogSevere("GDAL: Failed to read image data for {}", layerName);
          return nullptr;
        }

        return imageObj;
      }
    }
  }

  fLogSevere("GDAL: Layer '{}' not found in dataset.", layerName);
  return nullptr;
} // GDALDataTypeImp::getLayer

std::vector<GDALCatalogEntry>
GDALDataTypeImp::getCatalog()
{
  std::vector<GDALCatalogEntry> catalog;

  // Use myContext->mutex and myContext->dataset
  std::lock_guard<std::recursive_mutex> lock(myContext->mutex);

  if (!myContext->dataset) { return catalog; }

  // --- 1. Vector Layers ---
  int numLayers = myContext->dataset->GetLayerCount();

  for (int i = 0; i < numLayers; ++i) {
    OGRLayer * poLayer = myContext->dataset->GetLayer(i);
    if (poLayer) {
      GDALCatalogEntry entry;
      entry.name = poLayer->GetName();
      entry.type = "Vector";

      OGRwkbGeometryType geomType = wkbFlatten(poLayer->GetGeomType());
      entry.geometryType = OGRGeometryTypeToName(geomType);
      entry.count        = poLayer->GetFeatureCount(false);

      OGREnvelope envelope;
      if (poLayer->GetExtent(&envelope, false) == OGRERR_NONE) {
        entry.hasExtent = true;
        entry.minLon    = envelope.MinX;
        entry.maxLon    = envelope.MaxX;
        entry.minLat    = envelope.MinY;
        entry.maxLat    = envelope.MaxY;
      } else {
        entry.hasExtent = false;
      }

      catalog.push_back(entry);
    }
  }

  // --- 2. Raster Bands ---
  double geoTransform[6];
  bool datasetHasTransform = (myContext->dataset->GetGeoTransform(geoTransform) == CE_None);
  int xSize = myContext->dataset->GetRasterXSize();
  int ySize = myContext->dataset->GetRasterYSize();

  int numBands = myContext->dataset->GetRasterCount();

  for (int i = 1; i <= numBands; ++i) {
    GDALRasterBand * poBand = myContext->dataset->GetRasterBand(i);
    if (poBand) {
      GDALCatalogEntry entry;
      std::string desc = poBand->GetDescription();
      entry.name = desc.empty() ? "Band_" + std::to_string(i) : desc;
      entry.type = "Raster";

      entry.geometryType = GDALGetDataTypeName(poBand->GetRasterDataType());
      entry.count        = i;

      if (datasetHasTransform && (xSize > 0) && (ySize > 0)) {
        entry.hasExtent = true;
        entry.minLon    = geoTransform[0];
        entry.maxLat    = geoTransform[3];
        entry.maxLon    = geoTransform[0] + (xSize * geoTransform[1]) + (ySize * geoTransform[2]);
        entry.minLat    = geoTransform[3] + (xSize * geoTransform[4]) + (ySize * geoTransform[5]);
      } else {
        entry.hasExtent = false;
      }

      catalog.push_back(entry);
    }
  }

  return catalog;
} // GDALDataTypeImp::getCatalog
