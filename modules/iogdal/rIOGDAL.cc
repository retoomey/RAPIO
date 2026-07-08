#include "rIOGDAL.h"

#include "rFactory.h"
#include "rGDALDataTypeImp.h"
#include "rGDALLatLonGrids.h" // Handle standard grids

// GDAL C++ library
#include "gdal_priv.h"

// GDAL simple features
#include "ogr_api.h"
#include "ogrsf_frmts.h"
#include "ogr_p.h"

using namespace rapio;

// Library dynamic link to create this factory
extern "C"
{
void *
createRAPIOIO(void)
{
  auto * z = new IOGDAL();

  z->initialize();
  return reinterpret_cast<void *>(z);
}
};

std::string
IOGDAL::getHelpString(const std::string& key)
{
  std::string help;

  help += "builder that uses the GDAL library and supported formats.";
  return help;
}

void
IOGDAL::initialize()
{
  GDALAllRegister(); // Register/introduce all GDAL drivers
  // CPLPushErrorHandler(CPLQuietErrorHandler);
  // Register the specializers we currently support
  GDALLatLonGrids::introduceSelf(this);
}

IOGDAL::~IOGDAL()
{ }

void
IOGDAL::introduce(const std::string & name,
  std::shared_ptr<GDALType>         new_subclass)
{
  Factory<GDALType>::introduce(name, new_subclass);
}

std::shared_ptr<rapio::GDALType>
IOGDAL::getIOGDAL(const std::string& name)
{
  return (Factory<GDALType>::get(name, "GDAL builder"));
}

std::shared_ptr<DataType>
IOGDAL::createDataType(const std::string& params)
{
  URL url(params);
  std::string filepath = url.toString();

  GDALAllRegister();

  // Use GDALOpenEx and explicitly allow both Rasters and Vectors
  GDALDataset * poDataset = static_cast<GDALDataset *>(GDALOpenEx(
      filepath.c_str(),
      GDAL_OF_RASTER | GDAL_OF_VECTOR | GDAL_OF_READONLY,
      nullptr,
      nullptr,
      nullptr
  ));

  if (poDataset == nullptr) {
    fLogSevere("IOGdal: Failed to open dataset. Is it a valid format?");
    return nullptr;
  }

  // Check if it's something GDAL can actually handle (Raster or Vector)
  bool isSupported = (poDataset->GetRasterCount() > 0 || poDataset->GetLayerCount() > 0);

  GDALClose(poDataset); // Close the probe

  if (isSupported) {
    // INSTEAD of using a specializer here, return the Wrapper directly
    // This allows the caller (WebGUI) to cast to GDALDataType and call getCatalog()
    return std::make_shared<GDALDataTypeImp>(filepath);
  }

  return nullptr;
} // IOGDAL::createDataType

bool
IOGDAL::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>             & keys
)
{
  const std::string type = dt->getDataType();
  std::shared_ptr<IOSpecializer> fmt = getIOSpecializer(type);

  // Fallback: If it's a grid but doesn't have a direct GDAL specializer yet,
  // try treating it as a generic LatLonGrid
  if (fmt == nullptr) {
    auto grid = std::dynamic_pointer_cast<LatLonGrid>(dt);
    if (grid != nullptr) {
      fmt = getIOSpecializer("LatLonGrid");
    }
  }

  if (fmt == nullptr) {
    fLogSevere("IOGDAL: Cannot create a GDAL IO writer for datatype {}", type);
    return false;
  }

  std::string filename;

  if (!resolveFileName(keys, "tif", "gdal-", filename)) {
    return false;
  }

  // The GDALLatLonGrids specializer expects the target path in the "FilePath" key
  keys["FilePath"] = filename;

  bool successful = false;

  try {
    // Route directly to GDALLatLonGrids::write (No BBOX, no manual looping!)
    successful = fmt->write(dt, keys);
  } catch (const std::exception& e) {
    fLogSevere("IOGDAL: Exception during GDAL write: {}", e.what());
    successful = false;
  }

  if (successful) {
    successful = postWriteProcess(filename, keys);
  }

  if (successful) {
    showFileInfo("GDAL writer: ", keys);
  }

  return successful;
} // IOGDAL::encodeDataType
