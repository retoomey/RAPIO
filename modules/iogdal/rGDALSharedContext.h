#pragma once
#include <gdal_priv.h>
#include <mutex>

namespace rapio {
/** A shared GDAL context for pulled layers or rasters.
 * This keeps the GDAL pointer so we can subquery, allowing
 * us to do things like vector tiles */
struct GDALSharedContext {
  GDALDataset *        dataset = nullptr;

  /** Make this recursive so nested GDAL calls on the same thread don't deadlock */
  std::recursive_mutex mutex;

  ~GDALSharedContext()
  {
    if (dataset != nullptr) {
      GDALClose(dataset);
      dataset = nullptr;
    }
  }
};
}
