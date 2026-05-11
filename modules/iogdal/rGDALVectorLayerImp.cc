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

  std::lock_guard<std::recursive_mutex> lock(myContext->mutex);

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

std::string
GDALVectorLayerImp::getTileMVT(double minLon, double minLat, double maxLon, double maxLat, int z, int x, int y)
{
  std::lock_guard<std::recursive_mutex> lock(myContext->mutex);

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
  OGRLayer * pDstLayer = poDstDS->CreateLayer(poLayer->GetName(), &oSRS3857, poLayer->GetGeomType(), papszLayerOptions);

  CSLDestroy(papszLayerOptions);

  // Copy field definitions so attributes carry over to MVT!
  OGRFeatureDefn * poDefn = poLayer->GetLayerDefn();

  for (int i = 0; i < poDefn->GetFieldCount(); ++i) {
    pDstLayer->CreateField(poDefn->GetFieldDefn(i));
  }

  if ((minLon != 0) || (minLat != 0) || (maxLon != 0) || (maxLat != 0)) {
    poLayer->SetSpatialFilterRect(minLon, minLat, maxLon, maxLat);
  } else {
    poLayer->SetSpatialFilter(nullptr);
  }

  poLayer->ResetReading();
  OGRFeature * feature;

  while ((feature = poLayer->GetNextFeature()) != nullptr) {
    OGRGeometry * geom = feature->GetGeometryRef();
    if (geom) {
      // FIX: Use full polygons, DO NOT use boundary()
      OGRGeometry * exportGeom = geom->clone();

      if (exportGeom && !exportGeom->IsEmpty()) {
        if (exportGeom->transform(poCT) == OGRERR_NONE) {
          OGRFeature * outFeat = OGRFeature::CreateFeature(pDstLayer->GetLayerDefn());
          outFeat->SetGeometryDirectly(exportGeom);

          // Copy attribute data (properties)
          for (int i = 0; i < feature->GetFieldCount(); ++i) {
            outFeat->SetField(i, feature->GetRawFieldRef(i));
          }

          if (pDstLayer->CreateFeature(outFeat) != OGRERR_NONE) {
            fLogSevere("GDAL: Failed to write feature to MVT tile layer.");
          }
          OGRFeature::DestroyFeature(outFeat);
        } else {
          OGRGeometryFactory::destroyGeometry(exportGeom);
        }
      } else if (exportGeom) {
        OGRGeometryFactory::destroyGeometry(exportGeom);
      }
    }
    OGRFeature::DestroyFeature(feature);
  }

  poLayer->SetSpatialFilter(nullptr);
  OCTDestroyCoordinateTransformation(poCT);

  // CRITICAL: Close the dataset to flush bytes to memory
  GDALClose(poDstDS);

  std::string mvtBuffer = "";
  std::string pbfPath   = vsiDir + "/" + std::to_string(z) + "/" + std::to_string(x) + "/" + std::to_string(y) + ".pbf";
  VSILFILE * fp         = VSIFOpenL(pbfPath.c_str(), "rb");

  if (fp) {
    VSIStatBufL stat;
    // Fix for VSIStatL unused result warning
    if (VSIStatL(pbfPath.c_str(), &stat) == 0) {
      mvtBuffer.resize(stat.st_size);
      VSIFReadL((void *) mvtBuffer.data(), 1, stat.st_size, fp);
    }
    VSIFCloseL(fp);
  }

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
  std::lock_guard<std::recursive_mutex> lock(myContext->mutex);

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
        // FIX: Intersect the full polygon, DO NOT use boundary()
        exportGeom = geom->Intersection(&clipPoly);
      } else {
        exportGeom = geom->clone();
      }

      if (exportGeom && !exportGeom->IsEmpty()) {
        char * jsonStr = exportGeom->exportToJson();
        if (jsonStr) {
          if (!firstFeature) { geojson += ","; }

          // Embed properties (attributes) for frontend popups
          geojson += "{\"type\": \"Feature\", \"properties\": {";
          bool firstField = true;
          for (int i = 0; i < feature->GetFieldCount(); ++i) {
            if (feature->IsFieldSet(i)) {
              if (!firstField) { geojson += ","; }
              geojson += "\"" + std::string(feature->GetFieldDefnRef(i)->GetNameRef()) + "\":\""
                + std::string(feature->GetFieldAsString(i)) + "\"";
              firstField = false;
            }
          }

          geojson += "}, \"geometry\": ";
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
