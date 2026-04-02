#include "rMaxAGL.h"
#include "rIODataType.h"
#include "rLatLonHeightGridIterator.h"
#include <algorithm>

using namespace rapio;

void
MaxAGL::declareOptions(RAPIOOptions& o)
{
  VolumeAlgorithm::declareOptions(o); // Get generic options for all volume algorithms

  o.setDescription("Calculates the maximum value in a column from ground to a specified AGL height.");
  o.setAuthors("Robert Toomey");
  o.optional("topaglkm", "4.0", "Top height AGL in kilometers to search up to (default 4km)");
  declareProduct("MaxAGL", "Max value from ground to top agl");
}

void
MaxAGL::processOptions(RAPIOOptions& o)
{
  VolumeAlgorithm::processOptions(o); // Process generic options for all volume algorithms

  myTopAglKMs = o.getFloat("topaglkm");
}

void
MaxAGL::checkOutputGrids(std::shared_ptr<LatLonHeightGrid> input)
{
  const rapio::Time forTime = input->getTime();

  // Rebuild the output grid if the coverage area changes (or on first run)
  if ((myMaxGrid == nullptr) || checkCoverageChange(input)) {
    fLogInfo("Creating new output grid for MaxAGL (0-{} km AGL)...", myTopAglKMs);
    myMaxGrid = rapio::LatLonGrid::Create("MaxAGL", input->getUnits(), forTime, input->getLLCoverageArea());

    char subTypeBuf[32];
    snprintf(subTypeBuf, sizeof(subTypeBuf), "0-%.1fkm", myTopAglKMs);
    myMaxGrid->setDataAttributeValue("SubType", subTypeBuf);

    // Attempt to borrow the colormap from the input (e.g. Reflectivity colors)
    std::string colorMap;
    if (input->getString("ColorMap-value", colorMap)) {
      myMaxGrid->setDataAttributeValue("ColorMap", colorMap);
    }
  }

  // Always sync the time to the incoming data cube
  myMaxGrid->setTime(forTime);
}

/**
 * This column up callback is so common it may become a
 * special class.  Lots of algorithms go through a column like this */
class MaxAGLCallback : public rapio::LatLonHeightGridCallback {
public:
  MaxAGLCallback(float topAglKMs, rapio::ArrayFloat2DPtr max2D,
    std::shared_ptr<rapio::LatLonGridProjection> terrProj,
    const std::vector<float>& hlevels)
    : myTopAglKMs(topAglKMs), myMax2D(max2D),
    myTerrainProj(terrProj), myHLevels(hlevels){ }

  // Fired once per X,Y location before iterating up the Z column
  void
  handleBeginColumn(rapio::LatLonHeightGridIterator * it) override
  {
    // Reset our column state
    mask       = rapio::Constants::DataUnavailable;
    maxVal     = -999999.0f;
    foundValid = false;

    // Gracefully handle missing terrain projection
    if (myTerrainProj != nullptr) {
      double terrHeightM = myTerrainProj->getValueAtLL(it->getCurrentLatDegs(), it->getCurrentLonDegs());
      terrHeightKm = (!rapio::Constants::isGood(terrHeightM)) ? 0.0f : (terrHeightM / 1000.0f);
    } else {
      terrHeightKm = 0.0f; // Default to sea level
    }
  }

  // Fired for every Z voxel in the column
  void
  handleVoxel(rapio::LatLonHeightGridIterator * it) override
  {
    float currentHtKm = myHLevels[it->getCurrentLayerIdx()];

    // Only process if we are within the AGL window
    if ((currentHtKm >= terrHeightKm) && (currentHtKm <= topSearchHeightKm)) {
      float v = it->getValue(); // The iterator grabs the correct 3D value for us

      if (rapio::Constants::isGood(v)) {
        if (!foundValid || (v > maxVal)) {
          maxVal     = v;
          foundValid = true;
        }
      }

      // Update our missing/unavailable mask tracking
      if (v != rapio::Constants::DataUnavailable) {
        mask = rapio::Constants::MissingData;
      }
    }
  }

  // Fired after the Z column finishes
  void
  handleEndColumn(rapio::LatLonHeightGridIterator * it) override
  {
    // Write the final calculated value to the 2D output grid
    (*myMax2D)[it->getCurrentLatIdx()][it->getCurrentLonIdx()] = foundValid ? maxVal : mask;
  }

private:
  float myTopAglKMs;
  rapio::ArrayFloat2DPtr myMax2D;
  std::shared_ptr<rapio::LatLonGridProjection> myTerrainProj;
  const std::vector<float>& myHLevels;

  // Column State
  float maxVal;
  bool foundValid;
  float mask;
  float terrHeightKm;
  float topSearchHeightKm;
};

void
MaxAGL::processVolume(std::shared_ptr<LatLonHeightGrid> input, RAPIOAlgorithm * writer)
{
  // Issue a one-time warning if terrain is missing
  if ((myTerrainProj == nullptr) && !myWarnedTerrain) {
    fLogSevere("No valid terrain provided. Defaulting to 0.0 AGL (sea level) for all columns.");
    myWarnedTerrain = true;
  }
  if (myMaxGrid == nullptr) {
    fLogSevere("Output grid not prepared or null.");
    return;
  }

  // 1. Bind our math to the target output grid and terrain projection
  // 2. Initialize the iterator over the 3D source cube
  // 3. March through the columns using the callback
  MaxAGLCallback myCallback(myTopAglKMs, myMaxGrid->getFloat2DPtr(), myTerrainProj, myHLevelsKMs);
  rapio::LatLonHeightGridIterator iter(*input);

  iter.iterateUpColumns(myCallback);

  // Let the orchestrator (or us if standalone) handle the file write
  std::map<std::string, std::string> extraParams;

  writer->writeOutputProduct("MaxAGL", myMaxGrid, extraParams);
} // MaxAGL::processVolume
