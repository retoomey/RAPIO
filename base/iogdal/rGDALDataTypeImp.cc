#include "rGDALDataTypeImp.h"
#include "rIOGDAL.h"

// GDAL C++ library
#include "gdal_priv.h"

// GDAL simple features
#include "ogr_api.h"
#include "ogrsf_frmts.h"
#include "ogr_p.h"

using namespace rapio;

// Experimental start of reading shapefile, other stuff...
// I have to learn GDAL better and improve this over time
bool
GDALDataTypeImp::readGDALDataset(const std::string& key)
{
  GDALDataset * poDS;

  poDS = (GDALDataset *) GDALOpenEx(key.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
  if (poDS == NULL) {
    LogSevere("Failed to read " << key << "\n");
    return false;
  }

  // If it's a raster right?  I'm getting raster size 512 * 512 for a shapefile?
  // So are all the raster bands in a GDAL file the same size?  We can use that to
  // map to DataGrid arrays...
  int xSize = poDS->GetRasterXSize();
  int ySize = poDS->GetRasterYSize();

  LogSevere("Sizes of raster is " << xSize << ", " << ySize << "\n");
  int rcount = poDS->GetRasterCount();

  LogSevere("Raster count is " << rcount << "\n"); // zero for shapefile...ok works
  for (auto&& poBand: poDS->GetBands() ) {
    //  poBand->GetDescription();
  }

  // Get the layers
  // I want to read a shapefile here for first pass,
  // but we could and probably should generally wrap a GDALDataset
  size_t numLayers = poDS->GetLayerCount();

  for (auto&& poLayer: poDS->GetLayers() ) { // from docs
    LogSevere("Layer " << poLayer->GetName() << "\n");

    // Ok features are read and created it seems...
    OGRFeature * feature = NULL;
    poLayer->ResetReading();
    size_t i = 0;
    // In OK shapefile these are the counties...
    while ((feature = poLayer->GetNextFeature()) != NULL) {
      LogSevere(i << " Feature...\n");

      // Ok the fields are stuff stored per feature, like
      // phone number, etc.
      for (auto&& oField: *feature) {
        switch (oField.GetType()) {
            default:
              LogSevere("FIELD VALUE: " << oField.GetAsString() << "\n");
              break;
        }
      }

      // The geometry of the object. FIXME: How to project to wsr84?
      // Each feature can have _several geometry fields..interesting
      OGRGeometry * poGeometry = feature->GetGeometryRef();

      int geoCount = feature->GetGeomFieldCount();
      LogSevere("FEATURE HAS " << geoCount << " geometry fields\n");
      // Ahh kill me there's like 10 billion types here... wkbUnknown, wkbLineString
      // if (poGeometry != NULL && wkbFlatten(poGeometry->getGeometryType()) == wkbPoint )
      if (poGeometry != NULL) {
        LogSevere("The type is " << wkbFlatten(poGeometry->getGeometryType()) << "\n");
      }
      // if (poGeometry != NULL && wkbFlatten(poGeometry->getGeometryType()) == wkbPoint )
      if ((poGeometry != NULL) && (wkbFlatten(poGeometry->getGeometryType()) == wkbPolygon) ) {
        #if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(2, 3, 0)
        // OGRPoint *poPoint = poGeometry->toPoint();
        OGRPolygon * poPoint = poGeometry->toPolygon();
        #else
        // OGRPoint *poPoint = (OGRPoint *)poGeometry;
        OGRPolygon * poPoint = (OGRPolygon *) poGeometry;
        #endif
        //        LogSevere("POINT: " << poPoint->getX() <<  poPoint->getY() <<  "\n");

        // char * test = poGeometry->exportToJson();  Wow it works?
        ///  char * test = poGeometry->exportToKML();
        // poGeometry->GetPointCount();
        exit(1);
        // exportToJson()  exportToKML()  free with CPLFree()?
      } else {
        LogSevere("No point geometry!\n");
      }


      OGRFeature::DestroyFeature(feature);
      i++;
    }

    // FIXME: I guess layers don't get deleted?
  }
  LogSevere(">>>>GOT BACK " << numLayers << " layers\n");

  // delete poDS; // One delete doesn't crash so guess we do it?
  // On windows they say call GDALClose()

  GDALClose(poDS);
  return true;
} // GDALDataTypeImp::readGDALDataset
