#include "rMakeTerrain.h"
#include "rLatLonGrid.h"
#include "rOS.h"
#include "rIOURL.h"
#include "rIODataType.h"
#include <cmath>
#include <fstream>
#include <algorithm>
#include <thread>
#include <chrono>

using namespace rapio;

void
MakeTerrain::declareOptions(RAPIOOptions& o)
{
  o.setDescription("Fetches SRTM terrain data for a given grid and outputs a LatLonGrid.");
  o.setAuthors("RAPIO");
  o.declareLegacyGrid("nw(37, -100) se(30.5, -93) s(0.01, 0.01)");
  o.optional("cache", "/tmp/srtm_cache", "Directory to cache downloaded .hgt files");
  o.optional("url", "https://s3.amazonaws.com/elevation-tiles-prod/skadi", "Base URL for SRTM .hgt.gz files");
  o.optional("o", "", "Output NetCDF file name. If blank, auto-generates based on grid parameters.");
}

void
MakeTerrain::processOptions(RAPIOOptions& o)
{
  o.getLegacyGrid(myGrid);
  myCacheDir   = o.getString("cache");
  myBaseURL    = o.getString("url");
  myOutputFile = o.getString("o");

  if (!OS::ensureDirectory(myCacheDir)) {
    fLogSevere("Failed to create or access cache directory: {}", myCacheDir);
    exit(1);
  }
}

std::string
MakeTerrain::getSRTMFilename(int tileLat, int tileLon, std::string& latFolder)
{
  char folderBuf[16];

  snprintf(folderBuf, sizeof(folderBuf), "%c%02d", (tileLat >= 0) ? 'N' : 'S', std::abs(tileLat));
  latFolder = std::string(folderBuf);

  char fileBuf[32];

  snprintf(fileBuf, sizeof(fileBuf), "%s%c%03d.hgt",
    latFolder.c_str(), (tileLon >= 0) ? 'E' : 'W', std::abs(tileLon));
  return std::string(fileBuf);
}

bool
MakeTerrain::loadSRTMFile(const std::string& filename, const std::string& latFolder, std::vector<short>& data)
{
  std::string localPath = myCacheDir + "/" + filename;
  std::string remoteUrl = myBaseURL + "/" + latFolder + "/" + filename + ".gz";

  const int MAX_RETRIES = 3;
  int attempt = 0;

  while (attempt < MAX_RETRIES) {
    // 1. Download if missing from cache
    if (!OS::isRegularFile(localPath)) {
      fLogInfo("Cache miss. Downloading from: {}", remoteUrl);

      std::vector<char> uncompressedBuf;
      if (IOURL::read(URL(remoteUrl), uncompressedBuf) <= 0) {
        fLogSevere("Failed to download or decompress {}", remoteUrl);
      } else {
        // Fast-fail check: If it's a tiny file, it might be an S3 XML error (e.g., Ocean/Void tile)
        if (uncompressedBuf.size() < 1000) {
          std::string errStr(uncompressedBuf.begin(), uncompressedBuf.end());
          if ((errStr.find("NoSuchKey") != std::string::npos) || (errStr.find("AccessDenied") != std::string::npos) ) {
            fLogInfo("Tile does not exist on remote server (Ocean/Void): {}", filename);
            data.clear();
            return true; // We successfully verified it's water! Do not retry.
          }
        }

        // Save valid payload to cache
        std::ofstream out(localPath, std::ios::binary);
        if (out) {
          out.write(uncompressedBuf.data(), uncompressedBuf.size());
          out.close();
        } else {
          fLogSevere("Failed to write to local cache: {}", localPath);
        }
      }
    }

    // 2. Load and Validate the Cache File
    if (OS::isRegularFile(localPath)) {
      std::ifstream in(localPath, std::ios::binary | std::ios::ate);
      if (in) {
        size_t fileSize = in.tellg();
        in.seekg(0, std::ios::beg);

        size_t numShorts = fileSize / 2;

        // STRICT VALIDATION: Must be exactly SRTM1 or SRTM3 byte size
        if ((fileSize == 3601 * 3601 * 2) || (fileSize == 1201 * 1201 * 2)) {
          data.resize(numShorts);
          in.read(reinterpret_cast<char *>(data.data()), fileSize);

          #if IS_BIG_ENDIAN == 0
          for (size_t i = 0; i < numShorts; ++i) {
            OS::byteswap(data[i]);
          }
          #endif

          return true; // Success!
        } else {
          fLogSevere("Corrupt SRTM file size in cache: {} ({} bytes)", filename, fileSize);
          in.close();
          // Fall through to the retry logic below
        }
      }
    }

    // 3. Retry Logic (Triggered by bad download, bad write, or failed size validation)
    attempt++;
    if (attempt < MAX_RETRIES) {
      fLogInfo("Deleting corrupt/missing cache file and retrying (Attempt {} of {})...", attempt + 1, MAX_RETRIES);
      OS::deleteFile(localPath);
      std::this_thread::sleep_for(std::chrono::seconds(2));
    }
  }

  fLogSevere("Failed to acquire valid SRTM tile after {} attempts: {}", MAX_RETRIES, filename);
  return false;
} // MakeTerrain::loadSRTMFile

void
MakeTerrain::execute()
{
  fLogInfo("Executing MakeTerrain Blocked Processing...");

  if (myOutputFile.empty()) {
    myOutputFile = "Terrain_" + myGrid.getParseUniqueString() + ".nc";
  }

  fLogInfo("Generating {}x{} output grid for {}", myGrid.getNumX(), myGrid.getNumY(), myOutputFile);

  auto grid = LatLonGrid::Create("DEM", "Meters",
      LLH(myGrid.getNWLat(), myGrid.getNWLon(), 0),
      Time::CurrentTime(),
      myGrid.getLatSpacing(), myGrid.getLonSpacing(),
      myGrid.getNumY(), myGrid.getNumX());

  auto& data = grid->getFloat2DRef(Constants::PrimaryDataName);

  // Initialize array to MissingData
  for (size_t y = 0; y < myGrid.getNumY(); ++y) {
    for (size_t x = 0; x < myGrid.getNumX(); ++x) {
      data[y][x] = Constants::MissingData;
    }
  }

  double nwLat = myGrid.getNWLat();
  double nwLon = myGrid.getNWLon();
  double seLat = myGrid.getSELat();
  double seLon = myGrid.getSELon();
  double dLat  = myGrid.getLatSpacing();
  double dLon  = myGrid.getLonSpacing();

  // Determine the bounding integer tiles we need to fetch
  int minTileLat = static_cast<int>(std::floor(seLat));
  int maxTileLat = static_cast<int>(std::floor(nwLat));
  int minTileLon = static_cast<int>(std::floor(nwLon));
  int maxTileLon = static_cast<int>(std::floor(seLon));

  // Loop over required SRTM tiles
  for (int tLat = maxTileLat; tLat >= minTileLat; --tLat) {
    for (int tLon = minTileLon; tLon <= maxTileLon; ++tLon) {
      double tileNorth = tLat + 1.0;
      double tileSouth = tLat;
      double tileWest  = tLon;
      double tileEast  = tLon + 1.0;

      // Clamp the tile boundaries to our actual output grid boundaries
      double intersectNorth = std::min(nwLat, tileNorth);
      double intersectSouth = std::max(seLat, tileSouth);
      double intersectWest  = std::max(nwLon, tileWest);
      double intersectEast  = std::min(seLon, tileEast);

      if ((intersectNorth < intersectSouth) || (intersectWest > intersectEast) ) {
        continue; // Floating point edge case, no real overlap
      }

      // Map the intersection bounds to our output array indices (Y goes top-down)
      int startY = std::max(0, static_cast<int>(std::ceil((nwLat - intersectNorth) / dLat)));
      int endY   =
        std::min(static_cast<int>(myGrid.getNumY() - 1), static_cast<int>(std::floor((nwLat - intersectSouth) / dLat)));

      int startX = std::max(0, static_cast<int>(std::ceil((intersectWest - nwLon) / dLon)));
      int endX   =
        std::min(static_cast<int>(myGrid.getNumX() - 1), static_cast<int>(std::floor((intersectEast - nwLon) / dLon)));

      if ((startY > endY) || (startX > endX) ) { continue; }

      std::string latFolder;
      std::string filename = getSRTMFilename(tLat, tLon, latFolder);

      fLogInfo("Processing block: {} (Grid bounds Y:[{}-{}], X:[{}-{}])", filename, startY, endY, startX, endX);

      std::vector<short> tileData;
      bool loaded = loadSRTMFile(filename, latFolder, tileData);

      int dimension = 0;
      if (!loaded) {
        fLogSevere("Wasn't able to download or read a required tile after retries. Exiting.");
        exit(1);
      } else {
        if (tileData.empty()) {
          // It's an ocean/void tile. Leave dimension = 0 so we skip the pixel loop below.
        } else if (tileData.size() == 3601 * 3601) {
          dimension = 3601;
        } else if (tileData.size() == 1201 * 1201) {
          dimension = 1201;
        } else {
          fLogSevere("Corrupt SRTM file size in memory for {}. Exiting.", filename);
          exit(1);
        }
      }

      // Populate the specific sub-grid inside our output array (Skipped entirely for ocean tiles)
      if (dimension > 0) {
        for (int y = startY; y <= endY; ++y) {
          double currentLat = nwLat - y * dLat;
          double latFrac    = currentLat - tLat;

          int row = static_cast<int>(std::round((1.0 - latFrac) * (dimension - 1)));
          if (row < 0) { row = 0; }
          if (row >= dimension) { row = dimension - 1; }

          for (int x = startX; x <= endX; ++x) {
            double currentLon = nwLon + x * dLon;
            double lonFrac    = currentLon - tLon;

            int col = static_cast<int>(std::round(lonFrac * (dimension - 1)));
            if (col < 0) { col = 0; }
            if (col >= dimension) { col = dimension - 1; }

            short val = tileData[row * dimension + col];

            // Don't overwrite with a void if we already mapped a pixel on a tile boundary
            if (val != -32768) {
              data[y][x] = static_cast<float>(val);
            }
          }
        }
      }
      // Memory for tileData is automatically freed here at the end of the loop!
    }
  }

  fLogInfo("Writing terrain grid to {}...", myOutputFile);

  std::map<std::string, std::string> keys;

  keys["filepathmode"] = "direct";
  keys["filename"]     = myOutputFile;

  std::vector<Record> blackHole;

  IODataType::write(grid, myOutputFile, blackHole, "netcdf", keys);

  fLogInfo("Done.");
} // MakeTerrain::execute

int
main(int argc, char * argv[])
{
  MakeTerrain prog;

  prog.executeFromArgs(argc, argv);
}
