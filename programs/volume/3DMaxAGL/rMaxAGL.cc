#include "rMaxAGL.h"
#include "rIODataType.h"
#include "rLatLonHeightGridIterator.h"
#include <algorithm>

using namespace rapio;

extern "C"
{
RAPIOAlgorithm *
createRAPIOAlg(void)
{
  auto * z = new rapio::MaxAGL();

  return reinterpret_cast<rapio::RAPIOAlgorithm *>(z);
}
}

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
  const Time forTime = input->getTime();

  // Rebuild the output grid if the coverage area changes (or on first run)
  if ((myMaxGrid == nullptr) || checkCoverageChange(input)) {
    fLogInfo("Creating new output grid for MaxAGL (0-{} km AGL)...", myTopAglKMs);
    myMaxGrid = LatLonGrid::Create("MaxAGL", input->getUnits(), forTime, input->getLLCoverageArea());

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

class MaxAGLCallback : public LatLonHeightGridCallback {
public:
  MaxAGLCallback(float topAGLKMs, ArrayFloat2DPtr max2D)
    : myTopAGLKMs(topAGLKMs), myMax2D(max2D){ }

  // Fired once per X,Y location before iterating up the Z column
  void
  handleBeginColumn(LatLonHeightGridIterator * it) override
  {
    // Reset our column state
    myMask   = Constants::DataUnavailable;
    myMaxVal = -999999.0f;
    myValid  = false;

    myTerrainHeightKMs = it->getTerrainHeightKMs();

    // Top KMs is the terrain height + the value given
    myCutoffKMs = myTerrainHeightKMs + myTopAGLKMs;
  }

  // Fired for every Z voxel in the column
  void
  handleVoxel(LatLonHeightGridIterator * it) override
  {
    float currentHtKMs = it->getCurrentHeightKMs();

    // Only process if we are within the AGL window
    if ((currentHtKMs >= myTerrainHeightKMs) && (currentHtKMs <= myCutoffKMs)) {
      float v = it->getValue();

      if (Constants::isGood(v)) {
        if (!myValid || (v > myMaxVal)) {
          myMaxVal = v;
          myValid  = true;
        }
      }

      // Update our missing/unavailable mask tracking
      if (v != Constants::DataUnavailable) {
        myMask = Constants::MissingData;
      }
    }
  }

  // Fired after the Z column finishes
  void
  handleEndColumn(LatLonHeightGridIterator * it) override
  {
    // Write the final calculated value to the 2D output grid
    (*myMax2D)[it->getCurrentLatIdx()][it->getCurrentLonIdx()] = myValid ? myMaxVal : myMask;
  }

private:
  const float myTopAGLKMs; ///< The AGL in kilometers we want
  float myCutoffKMs;       ///< Cutoff at terrain + myTopAGLKMS
  ArrayFloat2DPtr myMax2D; ///< The output array

  float myMaxVal;           ///< Current max value for column
  bool myValid;             ///< Have we found at least one data value
  float myMask;             ///< Mask when no value counts
  float myTerrainHeightKMs; ///< Terrain height above sea level
};

#if 0
void
MaxAGL::processVolume(std::shared_ptr<LatLonHeightGrid> input, RAPIOAlgorithm * writer)
{
  // Issue a one-time warning if terrain is missing
  if ((haveTerrain() == false) && !myWarnedTerrain) {
    fLogSevere("No valid terrain provided. Defaulting to 0.0 AGL (sea level) for all columns.");
    myWarnedTerrain = true;
  }
  if (myMaxGrid == nullptr) {
    fLogSevere("Output grid not prepared or null.");
    return;
  }

  // Let the VolumeAlgorithm handle terrain, etc. by calling its iterate method.
  // Up feels more natural for this alg.
  MaxAGLCallback myCallback(myTopAglKMs, myMaxGrid->getFloat2DPtr());

  this->iterate(input, myCallback, IterateMode::ColumnsUp);

  // Let the orchestrator (or us if standalone) handle the file write
  std::map<std::string, std::string> extraParams;

  writer->writeOutputProduct("MaxAGL", myMaxGrid, extraParams);
} // MaxAGL::processVolume

#endif // if 0

std::unique_ptr<LatLonHeightGridCallback>
MaxAGL::createCallback()
{
  if ((haveTerrain() == false) && !myWarnedTerrain) {
    fLogSevere("No valid terrain provided. Defaulting to 0.0 AGL (sea level) for all columns.");
    myWarnedTerrain = true;
  }

  if (myMaxGrid == nullptr) {
    fLogSevere("Output grid not prepared or null.");
    return nullptr;
  }

  return std::make_unique<MaxAGLCallback>(myTopAglKMs, myMaxGrid->getFloat2DPtr());
}

void
MaxAGL::writeFinalProducts(RAPIOAlgorithm * writer)
{
  std::map<std::string, std::string> extraParams;

  writer->writeOutputProduct("MaxAGL", myMaxGrid, extraParams);
}
