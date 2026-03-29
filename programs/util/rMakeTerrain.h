#pragma once
#include <RAPIO.h>
#include <vector>
#include <string>

namespace rapio {
/**
 * @class MakeTerrain
 * @brief Fetches and stitches SRTM terrain data for a given geographic grid.
 *
 * This program calculates which 1x1 degree SRTM tiles intersect a user-specified
 * LatLonGrid. It uses a highly efficient blocked-processing approach: it downloads
 * (or loads from cache) one tile at a time, populates the exact sub-region of the
 * output grid that the tile covers, and discards the tile to maintain a minimal
 * memory footprint.
 *
 * Note: It can take a while to download files for large areas, but once
 * downloaded it's cached.  The final output file is typically named after
 * the grid specification allowing reuse of it.
 *
 * @ingroup rapio_utility
 * @author Robert Toomey
 */
class MakeTerrain : public rapio::RAPIOProgram {
public:
  /** @brief Default constructor. */
  MakeTerrain(){ };

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Do our tool's job */
  virtual void
  execute() override;

protected:

  /**
   * @brief Determines the SRTM filename and latitude folder for a given coordinate.
   */
  std::string
  getSRTMFilename(int tileLat, int tileLon, std::string& latFolder);

  /**
   * @brief Loads an SRTM .hgt file from the local cache or downloads it if missing.
   */
  bool
  loadSRTMFile(const std::string& filename, const std::string& latFolder, std::vector<short>& data);

  LLCoverageArea myGrid;    /**< The bounding box and resolution for the output grid. */
  std::string myCacheDir;   /**< Local directory path for caching .hgt files. */
  std::string myBaseURL;    /**< Remote base URL for the SRTM dataset. */
  std::string myOutputFile; /**< The requested output filename. */
};
} // namespace rapio
