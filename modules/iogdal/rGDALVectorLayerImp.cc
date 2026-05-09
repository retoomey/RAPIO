#include "rGDALVectorLayerImp.h"
#include <ogrsf_frmts.h>
#include <ogr_spatialref.h>
#include <rError.h>
#include "cpl_vsi.h" // Required for VSIFOpenL, etc.

using namespace rapio;

GDALVectorLayerImp::GDALVectorLayerImp(std::shared_ptr<GDALSharedContext> context, const std::string& layerName)
  : myContext(context), myLayerName(layerName), myFeatureCount(0), myHasExtent(false)
{
  myDataType = "VectorData";

  std::lock_guard<std::mutex> lock(myContext->mutex);

  if (myContext->dataset) {
    OGRLayer * poLayer = myContext->dataset->GetLayerByName(myLayerName.c_str());
    if (poLayer) {
      OGRwkbGeometryType geomType = wkbFlatten(poLayer->GetGeomType());
      myGeomType     = OGRGeometryTypeToName(geomType);
      myFeatureCount = poLayer->GetFeatureCount(false);

      OGREnvelope env;
      if (poLayer->GetExtent(&env, false) == OGRERR_NONE) {
        myHasExtent = true;
        myMinX      = env.MinX;
        myMaxX      = env.MaxX;
        myMinY      = env.MinY;
        myMaxY      = env.MaxY;
      }

      OGRFeatureDefn * poDefn = poLayer->GetLayerDefn();
      int fieldCount = poDefn->GetFieldCount();
      for (int i = 0; i < fieldCount; ++i) {
        OGRFieldDefn * poFieldDefn = poDefn->GetFieldDefn(i);
        myFieldNames.push_back(poFieldDefn->GetNameRef());
      }
    } else {
      fLogSevere("GDAL: Layer '{}' not found during initialization.", myLayerName);
    }
  }
}

bool
GDALVectorLayerImp::getExtent(double& minLon, double& minLat, double& maxLon, double& maxLat) const
{
  if (myHasExtent) {
    minLon = myMinX;
    maxLon = myMaxX;
    minLat = myMinY;
    maxLat = myMaxY;
    return true;
  }
  return false;
}

std::string
GDALVectorLayerImp::getGeoJSON()
{
  return getTileGeoJSON(0, 0, 0, 0); // 0 disables spatial filter for the whole file
}

#if 0
std::string
GDALVectorLayerImp::getTileBBox(double minLon, double minLat, double maxLon, double maxLat, int z, int x, int y,
  std::string& outMimeType)
{
  // 1. Check if the MVT driver is available in this build of GDAL
  GDALDriver * mvtDriver = GetGDALDriverManager()->GetDriverByName("MVT");
  std::string payload    = "";

  // 2. Try Binary Mapbox Vector Tiles (MVT) First
  if (mvtDriver != nullptr) {
    payload = getTileMVT(minLon, minLat, maxLon, maxLat, z, x, y);
    if (!payload.empty()) {
      outMimeType = "application/vnd.mapbox-vector-tile";
      return payload;
    }
  }

  // 3. Graceful Fallback: If MVT failed or driver is missing, serve high-perf GeoJSON
  payload     = getTileGeoJSON(minLon, minLat, maxLon, maxLat);
  outMimeType = "application/geo+json";
  return payload;
}

#endif // if 0

#if 0
std::string
GDALVectorLayerImp::getTileMVT(double minLon, double minLat, double maxLon, double maxLat, int z, int x, int y)
{
  std::lock_guard<std::mutex> lock(myContext->mutex);

  if (!myContext->dataset) { return ""; }

  OGRLayer * poLayer = myContext->dataset->GetLayerByName(myLayerName.c_str());

  if (!poLayer) { return ""; }

  GDALDriver * mvtDriver = GetGDALDriverManager()->GetDriverByName("MVT");

  if (!mvtDriver) { return ""; }

  // MVT requires Web Mercator coordinates (EPSG:3857)
  OGRSpatialReference oSRS4326;

  oSRS4326.SetWellKnownGeogCS("WGS84");
  oSRS4326.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER); // Force Lon/Lat

  OGRSpatialReference oSRS3857;

  oSRS3857.importFromEPSG(3857);

  OGRCoordinateTransformation * poCT = OGRCreateCoordinateTransformation(&oSRS4326, &oSRS3857);

  if (!poCT) {
    return ""; // Transformation failure drops to GeoJSON fallback
  }
  // Calculate tile bounding box in EPSG:3857
  double minX = minLon, minY = minLat;
  double maxX = maxLon, maxY = maxLat;

  poCT->Transform(1, &minX, &minY);
  poCT->Transform(1, &maxX, &maxY);

  std::string vsiPath = "/vsimem/tile_" + std::to_string(CPLGetPID()) + "_" + std::to_string(z) + "_"
    + std::to_string(x) + "_" + std::to_string(y) + ".mvt";

  // Inform MVT driver of exactly what coordinates belong in this tile
  char ** papszOptions = nullptr;

  papszOptions = CSLSetNameValue(papszOptions, "FORMAT", "MVT");
  std::string extent = std::to_string(minX) + "," + std::to_string(minY) + "," + std::to_string(maxX) + ","
    + std::to_string(maxY);

  papszOptions = CSLSetNameValue(papszOptions, "SPATIAL_EXTENT", extent.c_str());

  GDALDataset * poDstDS = mvtDriver->Create(vsiPath.c_str(), 0, 0, 0, GDT_Unknown, papszOptions);

  CSLDestroy(papszOptions);
  if (!poDstDS) {
    OCTDestroyCoordinateTransformation(poCT);
    return "";
  }

  OGRLayer * pDstLayer = poDstDS->CreateLayer(poLayer->GetName(), &oSRS3857, poLayer->GetGeomType(), nullptr);

  // ------------------ FEATURE EXTRACTION & CLIPPING ------------------
  OGRPolygon clipPoly;
  bool doClip = (minLon != 0 || minLat != 0 || maxLon != 0 || maxLat != 0);

  if (doClip) {
    poLayer->SetSpatialFilterRect(minLon, minLat, maxLon, maxLat);
    OGRLinearRing ring;
    ring.addPoint(minLon, minLat);
    ring.addPoint(maxLon, minLat);
    ring.addPoint(maxLon, maxLat);
    ring.addPoint(minLon, maxLat);
    ring.addPoint(minLon, minLat);
    clipPoly.addRing(&ring);
  } else {
    poLayer->SetSpatialFilter(nullptr);
  }

  poLayer->ResetReading();
  OGRFeature * feature;

  while ((feature = poLayer->GetNextFeature()) != nullptr) {
    OGRGeometry * geom = feature->GetGeometryRef();
    if (geom) {
      OGRGeometry * exportGeom = nullptr;

      if (doClip) {
        OGRGeometry * boundary = geom->Boundary();
        if (boundary) {
          exportGeom = boundary->Intersection(&clipPoly);
          OGRGeometryFactory::destroyGeometry(boundary);
        } else {
          exportGeom = geom->Intersection(&clipPoly);
        }

        // LOD SIMPLIFICATION
        if (exportGeom && !exportGeom->IsEmpty()) {
          double pixelTolerance    = (maxLon - minLon) / 256.0;
          OGRGeometry * simplified = exportGeom->Simplify(pixelTolerance);
          if (simplified) {
            OGRGeometryFactory::destroyGeometry(exportGeom);
            exportGeom = simplified;
          }
        }
      } else {
        exportGeom = geom->clone();
      }

      if (exportGeom && !exportGeom->IsEmpty()) {
        // MVT requires features to be written in Web Mercator coordinate space
        exportGeom->transform(poCT);

        OGRFeature * outFeat = OGRFeature::CreateFeature(pDstLayer->GetLayerDefn());
        outFeat->SetGeometry(exportGeom);
        pDstLayer->CreateFeature(outFeat);
        OGRFeature::DestroyFeature(outFeat);
      }

      if (exportGeom) { OGRGeometryFactory::destroyGeometry(exportGeom); }
    }
    OGRFeature::DestroyFeature(feature);
  }

  poLayer->SetSpatialFilter(nullptr);
  OCTDestroyCoordinateTransformation(poCT);

  // CRITICAL: Must close dataset to flush Protobuf binary payload to /vsimem/
  GDALClose(poDstDS);

  // Extract the raw bytes from GDAL's RAM disk
  std::string mvtBuffer = "";
  VSILFILE * fp         = VSIFOpenL(vsiPath.c_str(), "rb");

  if (fp) {
    VSIStatBufL stat;
    VSIStatL(vsiPath.c_str(), &stat);
    mvtBuffer.resize(stat.st_size);
    VSIFReadL((void *) mvtBuffer.data(), 1, stat.st_size, fp);
    VSIFCloseL(fp);
  }

  VSIUnlink(vsiPath.c_str()); // Prevent memory leak

  return mvtBuffer;
} // GDALVectorLayerImp::getTileMVT

#endif // if 0
#if 0
std::string
GDALVectorLayerImp::getTileMVT(double minLon, double minLat, double maxLon, double maxLat, int z, int x, int y)
{
  std::lock_guard<std::mutex> lock(myContext->mutex);

  if (!myContext->dataset) { return ""; }

  OGRLayer * poLayer = myContext->dataset->GetLayerByName(myLayerName.c_str());

  if (!poLayer) { return ""; }

  GDALDriver * mvtDriver = GetGDALDriverManager()->GetDriverByName("MVT");

  if (!mvtDriver) { return ""; }

  // Coordinate Transformations: MVT strict requirement is EPSG:3857 (Web Mercator)
  OGRSpatialReference oSRS4326;

  oSRS4326.SetWellKnownGeogCS("WGS84");
  oSRS4326.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER); // Force Lon/Lat

  OGRSpatialReference oSRS3857;

  oSRS3857.importFromEPSG(3857);

  OGRCoordinateTransformation * poCT = OGRCreateCoordinateTransformation(&oSRS4326, &oSRS3857);

  if (!poCT) { return ""; }

  // 1. Create an MVT Directory Dataset in GDAL's RAM disk
  std::string vsiDir = "/vsimem/mvt_" + std::to_string(CPLGetPID()) + "_" + std::to_string(z) + "_"
    + std::to_string(x) + "_" + std::to_string(y);

  char ** papszOptions = nullptr;

  papszOptions = CSLSetNameValue(papszOptions, "FORMAT", "DIRECTORY"); // Force directory mode
  papszOptions = CSLSetNameValue(papszOptions, "COMPRESS", "NO");      // Raw un-gzipped PBF

  GDALDataset * poDstDS = mvtDriver->Create(vsiDir.c_str(), 0, 0, 0, GDT_Unknown, papszOptions);

  CSLDestroy(papszOptions);
  if (!poDstDS) {
    OCTDestroyCoordinateTransformation(poCT);
    return "";
  }

  # if 0
  // 2. Lock the layer to only generate this specific zoom level
  char ** papszLayerOptions = nullptr;
  papszLayerOptions = CSLSetNameValue(papszLayerOptions, "MINZOOM", std::to_string(z).c_str());
  papszLayerOptions = CSLSetNameValue(papszLayerOptions, "MAXZOOM", std::to_string(z).c_str());

  OGRLayer * pDstLayer = poDstDS->CreateLayer(poLayer->GetName(), &oSRS3857, poLayer->GetGeomType(), papszLayerOptions);
  CSLDestroy(papszLayerOptions);

  // 3. Fast Spatial Filter (Grab only intersecting features)
  if ((minLon != 0) || (minLat != 0) || (maxLon != 0) || (maxLat != 0)) {
    poLayer->SetSpatialFilterRect(minLon, minLat, maxLon, maxLat);
  } else {
    poLayer->SetSpatialFilter(nullptr);
  }

  poLayer->ResetReading();
  OGRFeature * feature;

  // 4. Feed EPSG:3857 features to the MVT driver
  // The driver automatically handles clipping to tile bounds and LOD simplification!
  while ((feature = poLayer->GetNextFeature()) != nullptr) {
    OGRGeometry * geom = feature->GetGeometryRef();
    if (geom) {
      OGRGeometry * exportGeom = geom->clone();
      exportGeom->transform(poCT); // Project to Web Mercator

      OGRFeature * outFeat = OGRFeature::CreateFeature(pDstLayer->GetLayerDefn());
      outFeat->SetGeometryDirectly(exportGeom);

      // Copy attributes so we can click on them in Leaflet!
      for (int i = 0; i < feature->GetFieldCount(); ++i) {
        outFeat->SetField(i, feature->GetRawFieldRef(i));
      }

      pDstLayer->CreateFeature(outFeat);
      OGRFeature::DestroyFeature(outFeat);
    }
    OGRFeature::DestroyFeature(feature);
  }
  # endif // if 0
  // 2. Lock the layer to only generate this specific zoom level
  char ** papszLayerOptions = nullptr;
  papszLayerOptions = CSLSetNameValue(papszLayerOptions, "MINZOOM", std::to_string(z).c_str());
  papszLayerOptions = CSLSetNameValue(papszLayerOptions, "MAXZOOM", std::to_string(z).c_str());

  // --- FIX #1: Use wkbUnknown so the MVT driver accepts our converted LineStrings ---
  OGRLayer * pDstLayer = poDstDS->CreateLayer(poLayer->GetName(), &oSRS3857, wkbUnknown, papszLayerOptions);
  CSLDestroy(papszLayerOptions);

  // (Optional but highly recommended) Copy the attribute schema so your frontend can click on states
  OGRFeatureDefn * poDefn = poLayer->GetLayerDefn();
  for (int i = 0; i < poDefn->GetFieldCount(); ++i) {
    pDstLayer->CreateField(poDefn->GetFieldDefn(i));
  }

  // 3. Fast Spatial Filter (Grab only intersecting features)
  if ((minLon != 0) || (minLat != 0) || (maxLon != 0) || (maxLat != 0)) {
    poLayer->SetSpatialFilterRect(minLon, minLat, maxLon, maxLat);
  } else {
    poLayer->SetSpatialFilter(nullptr);
  }

  poLayer->ResetReading();
  OGRFeature * feature;

  // 4. Feed EPSG:3857 features to the MVT driver
  while ((feature = poLayer->GetNextFeature()) != nullptr) {
    OGRGeometry * geom = feature->GetGeometryRef();
    if (geom) {
      // --- FIX #2: Convert Polygons to Boundary Lines ---
      OGRGeometry * exportGeom = geom->Boundary();
      if (!exportGeom) {
        exportGeom = geom->clone(); // Fallback for Points/Lines
      }

      exportGeom->transform(poCT); // Project to Web Mercator

      OGRFeature * outFeat = OGRFeature::CreateFeature(pDstLayer->GetLayerDefn());
      outFeat->SetGeometryDirectly(exportGeom);

      // Copy attributes
      for (int i = 0; i < feature->GetFieldCount(); ++i) {
        outFeat->SetField(i, feature->GetRawFieldRef(i));
      }

      pDstLayer->CreateFeature(outFeat);
      OGRFeature::DestroyFeature(outFeat);
    }
    OGRFeature::DestroyFeature(feature);
  }

  poLayer->SetSpatialFilter(nullptr);
  OCTDestroyCoordinateTransformation(poCT);

  // 5. CRITICAL: Close dataset to execute the tiling and generate the PBF file in RAM
  GDALClose(poDstDS);

  // 6. Extract the generated PBF bytes
  std::string mvtBuffer = "";
  std::string pbfPath   = vsiDir + "/" + std::to_string(z) + "/" + std::to_string(x) + "/" + std::to_string(y) + ".pbf";

  VSILFILE * fp = VSIFOpenL(pbfPath.c_str(), "rb");
  if (fp) {
    VSIStatBufL stat;
    VSIStatL(pbfPath.c_str(), &stat);
    mvtBuffer.resize(stat.st_size);
    VSIFReadL((void *) mvtBuffer.data(), 1, stat.st_size, fp);
    VSIFCloseL(fp);
  }

  // 7. Cleanup the virtual directory tree to prevent RAM leaks
  VSIUnlink(pbfPath.c_str());
  VSIUnlink((vsiDir + "/metadata.json").c_str()); // Auto-generated by FORMAT=DIRECTORY
  VSIRmdir((vsiDir + "/" + std::to_string(z) + "/" + std::to_string(x)).c_str());
  VSIRmdir((vsiDir + "/" + std::to_string(z)).c_str());
  VSIRmdir(vsiDir.c_str());

  return mvtBuffer;
} // GDALVectorLayerImp::getTileMVT

#endif // if 0
std::string
GDALVectorLayerImp::getTileMVT(double minLon, double minLat, double maxLon, double maxLat, int z, int x, int y)
{
  std::lock_guard<std::mutex> lock(myContext->mutex);

  if (!myContext->dataset) { return ""; }

  OGRLayer * poLayer = myContext->dataset->GetLayerByName(myLayerName.c_str());

  if (!poLayer) { return ""; }

  GDALDriver * mvtDriver = GetGDALDriverManager()->GetDriverByName("MVT");

  if (!mvtDriver) { return ""; }

  // Project to Web Mercator (Required for MVT)
  OGRSpatialReference oSRS4326;

  oSRS4326.SetWellKnownGeogCS("WGS84");
  oSRS4326.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

  OGRSpatialReference oSRS3857;

  oSRS3857.importFromEPSG(3857);

  OGRCoordinateTransformation * poCT = OGRCreateCoordinateTransformation(&oSRS4326, &oSRS3857);

  if (!poCT) { return ""; }

  // 1. Create Directory Dataset
  std::string vsiDir = "/vsimem/mvt_" + std::to_string(CPLGetPID()) + "_" + std::to_string(z) + "_"
    + std::to_string(x) + "_" + std::to_string(y);

  char ** papszOptions = nullptr;

  papszOptions = CSLSetNameValue(papszOptions, "FORMAT", "DIRECTORY");
  papszOptions = CSLSetNameValue(papszOptions, "COMPRESS", "NO");

  GDALDataset * poDstDS = mvtDriver->Create(vsiDir.c_str(), 0, 0, 0, GDT_Unknown, papszOptions);

  CSLDestroy(papszOptions);
  if (!poDstDS) {
    OCTDestroyCoordinateTransformation(poCT);
    return "";
  }

  char ** papszLayerOptions = nullptr;

  papszLayerOptions = CSLSetNameValue(papszLayerOptions, "MINZOOM", std::to_string(z).c_str());
  papszLayerOptions = CSLSetNameValue(papszLayerOptions, "MAXZOOM", std::to_string(z).c_str());

  // wkbUnknown is critical so it accepts our boundary LineStrings!
  OGRLayer * pDstLayer = poDstDS->CreateLayer(poLayer->GetName(), &oSRS3857, wkbUnknown, papszLayerOptions);

  CSLDestroy(papszLayerOptions);

  if ((minLon != 0) || (minLat != 0) || (maxLon != 0) || (maxLat != 0)) {
    poLayer->SetSpatialFilterRect(minLon, minLat, maxLon, maxLat);
  } else {
    poLayer->SetSpatialFilter(nullptr);
  }

  poLayer->ResetReading();
  OGRFeature * feature;

  #if 0
  // 2. Feed Features to GDAL
  while ((feature = poLayer->GetNextFeature()) != nullptr) {
    OGRGeometry * geom = feature->GetGeometryRef();
    if (geom) {
      // Convert to lines to prevent square borders!
      OGRGeometry * exportGeom = geom->Boundary();
      if (!exportGeom) {
        exportGeom = geom->clone();
      }

      exportGeom->transform(poCT);

      OGRFeature * outFeat = OGRFeature::CreateFeature(pDstLayer->GetLayerDefn());
      outFeat->SetGeometryDirectly(exportGeom);

      pDstLayer->CreateFeature(outFeat);
      OGRFeature::DestroyFeature(outFeat);
    }
    OGRFeature::DestroyFeature(feature);
  }
  #endif // if 0
  while ((feature = poLayer->GetNextFeature()) != nullptr) {
    OGRGeometry * geom = feature->GetGeometryRef();
    if (geom) {
      // Convert to lines to prevent square borders!
      OGRGeometry * exportGeom = geom->Boundary();
      if (!exportGeom) {
        exportGeom = geom->clone();
      }

      // --- THE MISSING LOD FIX ---
      // Apply our aggressive custom simplification BEFORE the MVT driver sees it
      if (exportGeom && !exportGeom->IsEmpty()) {
        double pixelTolerance    = (maxLon - minLon) / 256.0;
        OGRGeometry * simplified = exportGeom->Simplify(pixelTolerance);
        if (simplified) {
          OGRGeometryFactory::destroyGeometry(exportGeom);
          exportGeom = simplified;
        }
      }
      // ---------------------------

      exportGeom->transform(poCT); // Project to Web Mercator

      OGRFeature * outFeat = OGRFeature::CreateFeature(pDstLayer->GetLayerDefn());
      outFeat->SetGeometryDirectly(exportGeom);

      pDstLayer->CreateFeature(outFeat);
      OGRFeature::DestroyFeature(outFeat);
    }
    OGRFeature::DestroyFeature(feature);
  }

  poLayer->SetSpatialFilter(nullptr);
  OCTDestroyCoordinateTransformation(poCT);

  // 3. Close dataset to force the binary dump to /vsimem/
  GDALClose(poDstDS);

  // 4. Extract bytes
  std::string mvtBuffer = "";
  std::string pbfPath   = vsiDir + "/" + std::to_string(z) + "/" + std::to_string(x) + "/" + std::to_string(y) + ".pbf";

  VSILFILE * fp = VSIFOpenL(pbfPath.c_str(), "rb");
  if (fp) {
    VSIStatBufL stat;
    VSIStatL(pbfPath.c_str(), &stat);
    mvtBuffer.resize(stat.st_size);
    VSIFReadL((void *) mvtBuffer.data(), 1, stat.st_size, fp);
    VSIFCloseL(fp);
  }

  // 5. Cleanup
  VSIUnlink(pbfPath.c_str());
  VSIUnlink((vsiDir + "/metadata.json").c_str());
  VSIRmdir((vsiDir + "/" + std::to_string(z) + "/" + std::to_string(x)).c_str());
  VSIRmdir((vsiDir + "/" + std::to_string(z)).c_str());
  VSIRmdir(vsiDir.c_str());

  return mvtBuffer;
} // GDALVectorLayerImp::getTileMVT

std::string
GDALVectorLayerImp::getTileGeoJSON(double minLon, double minLat, double maxLon, double maxLat)
{
  std::lock_guard<std::mutex> lock(myContext->mutex);

  if (!myContext->dataset) { return "{}"; }

  OGRLayer * poLayer = myContext->dataset->GetLayerByName(myLayerName.c_str());

  if (!poLayer) { return "{}"; }

  OGRPolygon clipPoly;
  bool doClip = (minLon != 0 || minLat != 0 || maxLon != 0 || maxLat != 0);

  if (doClip) {
    poLayer->SetSpatialFilterRect(minLon, minLat, maxLon, maxLat);

    OGRLinearRing ring;
    ring.addPoint(minLon, minLat);
    ring.addPoint(maxLon, minLat);
    ring.addPoint(maxLon, maxLat);
    ring.addPoint(minLon, maxLat);
    ring.addPoint(minLon, minLat);
    clipPoly.addRing(&ring);
  } else {
    poLayer->SetSpatialFilter(nullptr);
  }

  poLayer->ResetReading();
  std::string geojson = "{\"type\": \"FeatureCollection\", \"features\": [";
  bool firstFeature   = true;
  OGRFeature * feature;

  while ((feature = poLayer->GetNextFeature()) != nullptr) {
    OGRGeometry * geom = feature->GetGeometryRef();
    if (geom) {
      OGRGeometry * exportGeom = nullptr;

      if (doClip) {
        OGRGeometry * boundary = geom->Boundary();
        if (boundary) {
          exportGeom = boundary->Intersection(&clipPoly);
          OGRGeometryFactory::destroyGeometry(boundary);
        } else {
          exportGeom = geom->Intersection(&clipPoly);
        }

        if (exportGeom && !exportGeom->IsEmpty()) {
          double pixelTolerance    = (maxLon - minLon) / 256.0;
          OGRGeometry * simplified = exportGeom->Simplify(pixelTolerance);
          if (simplified) {
            OGRGeometryFactory::destroyGeometry(exportGeom);
            exportGeom = simplified;
          }
        }
      } else {
        exportGeom = geom->clone();
      }

      if (exportGeom && !exportGeom->IsEmpty()) {
        char * jsonStr = exportGeom->exportToJson();
        if (jsonStr) {
          if (!firstFeature) { geojson += ","; }

          // STRIPPED PAYLOAD
          geojson += "{\"type\": \"Feature\", \"properties\": {}, \"geometry\": ";
          geojson += jsonStr;
          geojson += "}";

          CPLFree(jsonStr);
          firstFeature = false;
        }
      }

      if (exportGeom) { OGRGeometryFactory::destroyGeometry(exportGeom); }
    }
    OGRFeature::DestroyFeature(feature);
  }

  poLayer->SetSpatialFilter(nullptr);
  geojson += "]}";

  return geojson;
} // GDALVectorLayerImp::getTileGeoJSON
