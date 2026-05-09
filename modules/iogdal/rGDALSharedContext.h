#pragma once
#include <gdal_priv.h>
#include <mutex>

namespace rapio {
/** A shared GDAL context for pulled layers or rasters.
 * This keeps the GDAL pointer so we can subquery, allowing
 * us to do things like vector tiles */
struct GDALSharedContext {
  GDALDataset * dataset = nullptr;
  std::mutex    mutex;

  ~GDALSharedContext()
  {
    if (dataset != nullptr) {
      GDALClose(dataset);
      dataset = nullptr;
    }
  }
};
}
