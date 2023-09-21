#include "rStage2Data.h"
#include "rFusion1.h"
#include "rBitset.h"

using namespace rapio;

void
Stage2Data::add(float v, float w, float w2, short x, short y, short z)
{
  const bool compressMissing = false;

  if (compressMissing && (v == Constants::MissingData)) {
    // Only store x,y,z for a missing
    // Since missing tends to be grouped in space, we'll do a simple RLE to compress x,y,z locations
    // Storing a bit per x,y,z. marking MissingData
    size_t i = myMissingSet.getIndex({ (size_t) (x), (size_t) (y), (size_t) (z) });
    myMissingSet.set(i, 1);
    myAddMissingCounter++;
  } else if (v == Constants::DataUnavailable) { // We shouldn't send this right?
  } else {
    myValues.push_back(v);
    myWeights.push_back(w);
    myWeights2.push_back(w2);
    myXs.push_back(x); // X represents LON
    myYs.push_back(y); // Y represents LAT
    myZs.push_back(z); // Z represents HEIGHT.  We're hiding that z is a char
  }
}

void
Stage2Data::RLE()
{
  // RLE in 2D space left to right.  Missing typically groups up in blocks, this is similar I think
  // to the marked lines stuff in w2merger
  // FIXME: Maybe create general RLE interface for other uses

  size_t counter = 0;

  for (size_t z = 0; z < myDimensions[2]; z++) {
    for (size_t y = 0; y < myDimensions[1]; y++) {   // row
      for (size_t x = 0; x < myDimensions[0]; x++) { // col
        size_t index = myMissingSet.getIndex({ (size_t) x, (size_t) y, (size_t) z });
        auto flag    = myMissingSet.get(index);
        if (flag) {
          counter++;
          size_t startx = x;
          size_t starty = y;
          size_t startz = z;
          size_t length = 1;
          // Extend the run horizontally
          while (x + 1 < myDimensions[0]) {
            size_t index = myMissingSet.getIndex({ (size_t) x + 1, (size_t) y, (size_t) z });
            auto value   = myMissingSet.get(index);
            if (value) {
              ++x;
              ++length;
            } else {
              break;
            }
            // New RLE stuff to store...
          }
          // if (counter < 10) {
          //    std::cout << "RLE: " << startx << ", " << starty << ", " << startz << " --> " << length << "\n";
          // }
          myXMissings.push_back(startx);
          myYMissings.push_back(starty);
          myZMissings.push_back(startz);
          myLMissings.push_back(length); // has to be big enough for all of conus
        }
      }
    }
  }
  // LogInfo("RLE found " << counter << " start locations.  Dim size: " << myDimensions[0] << " , " << myDimensions[1] << ", " << myDimensions[2] <<  "\n");
} // Stage2Data::RLE

void
Stage2Data::send(RAPIOFusionOneAlg * alg, Time aTime, const std::string& asName)
{
  // Run length encode the missing bit array.  Since missing groups up this saves quite
  // a bit of space.
  RLE();

  LLH aLocation;
  size_t finalSize  = myXs.size();
  size_t finalSize2 = myXMissings.size(); // size from RLE

  LogInfo(
    "Sizes are " << finalSize << " values, with " << myAddMissingCounter << " missing " << finalSize2 << " (RLE).\n");
  if ((finalSize < 1) && (finalSize2 < 1)) {
    // FIXME: Humm do we still want to send 'something' for push triggering?  Do we need to?
    // Would be better if we don't have to
    LogInfo("--->Special case of size zero...ignoring stage2 output currently.\n");
  } else {
    auto stage2 = DataGrid::Create(asName, "dimensionless", aLocation, aTime, { finalSize, finalSize2 }, { "I", "MI" });
    stage2->setDataType("Stage2Ingest"); // DataType attribute in the file not filename
    stage2->setSubType("Full");
    stage2->setString("Radarname", myRadarName);
    //   stage2->setString("radarName-value", myRadarName); // for IODataType base add.  FIXME: clean up
    stage2->setString("Typename", myTypeName);
    stage2->setFloat("ElevationDegs", myElevationDegs);
    stage2->setLong("xBase", myXBase);
    stage2->setLong("yBase", myYBase);
    // FIXME: general in MRMS and RAPIO
    stage2->setString("NetcdfWriterInfo", "RAPIO:Fusion stage 2 data");
    stage2->setLocation(myCenter); // Radar center location

    // ---------------------------------------------------
    // Value per point storage (dim 0)
    //
    // FIXME: We 'should' be able to use a 'baseX' and 'baseY' for small radars
    // and store this silly small.  Radar will only cover so many squares
    auto& netcdfX        = stage2->addShort1DRef("X", "dimensionless", { 0 });
    auto& netcdfY        = stage2->addShort1DRef("Y", "dimensionless", { 0 });
    auto& netcdfZ        = stage2->addByte1DRef("Z", "dimensionless", { 0 });
    auto& netcdfValues   = stage2->addFloat1DRef(Constants::PrimaryDataName, myUnits, { 0 });
    auto& netcdfWeights  = stage2->addFloat1DRef("W1", "dimensionless", { 0 });
    auto& netcdfWeights2 = stage2->addFloat1DRef("W2", "dimensionless", { 0 });

    // Copying because size is dynamic.  FIXME: Could improve API to handle dynamic arrays?
    // Since we deal with 'small' radar coverage here, think we're ok for moment
    std::copy(myXs.begin(), myXs.end(), netcdfX.data());
    std::copy(myYs.begin(), myYs.end(), netcdfY.data());
    std::copy(myZs.begin(), myZs.end(), netcdfZ.data());
    std::copy(myValues.begin(), myValues.end(), netcdfValues.data());
    std::copy(myWeights.begin(), myWeights.end(), netcdfWeights.data());
    std::copy(myWeights2.begin(), myWeights2.end(), netcdfWeights2.data());

    // ---------------------------------------------------
    // Missing value storage (dim 1)
    //
    auto& netcdfXM = stage2->addShort1DRef("Xm", "dimensionless", { 1 });
    auto& netcdfYM = stage2->addShort1DRef("Ym", "dimensionless", { 1 });
    auto& netcdfZM = stage2->addByte1DRef("Zm", "dimensionless", { 1 });
    auto& netcdfLM = stage2->addShort1DRef("Lm", "dimensionless", { 1 }); // max 65000

    std::copy(myXMissings.begin(), myXMissings.end(), netcdfXM.data());
    std::copy(myYMissings.begin(), myYMissings.end(), netcdfYM.data());
    std::copy(myZMissings.begin(), myZMissings.end(), netcdfZM.data());
    std::copy(myLMissings.begin(), myLMissings.end(), netcdfLM.data());

    // We 'could' return the stage2 object let the algorithm write it...but we can hide
    // it here I think for moment just in case we end up doing something different
    std::map<std::string, std::string> extraParams; // FIXME: yeah I know, need to constant/API cleanup this stuff
    extraParams["showfilesize"] = "yes";
    extraParams["compression"]  = "gz";
    alg->writeOutputProduct(asName, stage2, extraParams);
  }
  // -------------------------------------------------------
} // Stage2Data::send

std::shared_ptr<Stage2Data>
Stage2Data::receive(RAPIOData& rData)
{
  // Stage two reading in stage one files
  auto gsp = rData.datatype<rapio::DataGrid>();

  if (gsp != nullptr) {
    auto& d = *gsp;
    // if (gsp->getTypeName() == "check for reflectivity, etc" if subgrouping
    if (gsp->getDataType() == "Stage2Ingest") { // Check for ANY stage2 file
      LogInfo("Stage2 data noticed: " << rData.getDescription() << "\n");
      DataGrid& d = *gsp;

      // ---------------------------------------------------
      // Restore original Stage2Data object
      // Not sure even need to copy things, we can stream it back without
      // making copies
      // FIXME: we 'could' have a seperate input/output class to avoid some copying.  The advantage to the
      // copying/etc. is that the internals are hidden so we can do what we want, the disadvantage is
      // having to convert things.
      auto dims = d.getSizes();
      size_t aSize = dims[0];
      size_t aSize2 = dims[1];
      std::string radarName, typeName, units;
      float elevDegs;
      d.getString("Radarname", radarName);
      d.getString("Typename", typeName);
      d.getFloat("ElevationDegs", elevDegs);
      long xBase = 0, yBase = 0;
      d.getLong("xBase", xBase);
      d.getLong("yBase", yBase);
      units = d.getUnits();
      LLH center = d.getLocation();
      std::shared_ptr<Stage2Data> insp =
        std::make_shared<Stage2Data>(Stage2Data(radarName, typeName, elevDegs, units, center, xBase, yBase, dims));
      auto& in = *insp;
      in.setTime(d.getTime());

      // ---------------------------------------------------
      // Value per point storage (dim 0)
      //
      auto& netcdfX        = d.getShort1DRef("X");
      auto& netcdfY        = d.getShort1DRef("Y");
      auto& netcdfZ        = d.getByte1DRef("Z");
      auto& netcdfValues   = d.getFloat1DRef();
      auto& netcdfWeights  = d.getFloat1DRef("W1");
      auto& netcdfWeights2 = d.getFloat1DRef("W2");

      in.myXs.reserve(aSize);
      in.myYs.reserve(aSize);
      in.myZs.reserve(aSize);
      in.myValues.reserve(aSize);
      in.myWeights.reserve(aSize);
      in.myWeights2.reserve(aSize);
      for (size_t i = 0; i < aSize; i++) {
        in.myXs.push_back(netcdfX[i]);
        in.myYs.push_back(netcdfY[i]);
        in.myZs.push_back(netcdfZ[i]);
        in.myValues.push_back(netcdfValues[i]);
        in.myWeights.push_back(netcdfWeights[i]);
        in.myWeights2.push_back(netcdfWeights2[i]);
      }

      // ---------------------------------------------------
      // Missing value storage (dim 1)
      //

      auto& netcdfXM   = d.getShort1DRef("Xm");
      auto& netcdfYM   = d.getShort1DRef("Ym");
      auto& netcdfZM   = d.getByte1DRef("Zm");
      auto& netcdfLM   = d.getShort1DRef("Lm");
      auto& RLElengths = d.getShort1DRef("Lm");

      // Get the RLE total size and dump for debugging
      size_t length = 0;
      for (size_t i = 0; i < aSize2; i++) {
        length += RLElengths[i];
      }

      LogInfo(
        "Size is " << aSize << " x,y,z non-missing values, and " << aSize2 << " RLE missing values expanding to " << length <<
          " total missing.  Total: " << aSize2 + length << "\n");
      if (aSize > 1) { aSize = 1; }
      for (size_t i = 0; i < aSize; i++) {
        LogInfo(
          "   " << i << ": (" << netcdfX[i] << "," << netcdfY[i] << ") stores " << netcdfValues[i] << " with weight " <<
            netcdfWeights[i] << "\n");
      }

      in.myXMissings.reserve(aSize2);
      in.myYMissings.reserve(aSize2);
      in.myZMissings.reserve(aSize2);
      for (size_t i = 0; i < aSize2; i++) {
        in.myXMissings.push_back(netcdfXM[i]);
        in.myYMissings.push_back(netcdfYM[i]);
        in.myZMissings.push_back(netcdfZM[i]);
        in.myLMissings.push_back(RLElengths[i]);
      }

      return insp;
    } else {
      //   LogSevere("We got unrecognized data:" << gsp->getDataType() << "\n");
    }
  }
  return nullptr;
} // Stage2Data::receive

bool
Stage2Data::get(float& v, float& w, float& w2, short& x, short& y, short& z)
{
  // First read the non-missing values...
  if (myCounter < myValues.size()) {
    x  = myXs[myCounter];
    y  = myYs[myCounter];
    z  = myZs[myCounter];
    v  = myValues[myCounter];
    w  = myWeights[myCounter];
    w2 = myWeights2[myCounter];
    myCounter++;
    return true;
  }

  // ...then read the missing values from RLE arrays
  if (myMCounter < myXMissings.size()) {
    // Run length encoding expansion I goofed a few times,
    // so extra description here:
    // lengths: 5, 3
    // values:  1  2
    // --> 1 1 1 1 1 2 2 2  Expected output
    //     0 1 2 3 4 0 1 2  RLECounter
    //     0 0 0 0 0 1 1 1  MCounter
    //     5 5 5 5 5 3 3 3  l (current length)
    // The Simple way, but we want reentrant
    // for (i =0 i < lengths.size() ++i){
    //  for (j = 0; j < lengths[i]; ++j){
    //      return values[i];
    //  }
    // }

    static size_t countout = 0;
    bool more = true;
    auto l    = myLMissings[myMCounter]; // Current length

    // Always snag the current missing
    x = myXMissings[myMCounter] + myRLECounter; // Since we RLE in the X (with no row wrapping)
    y = myYMissings[myMCounter];
    z = myZMissings[myMCounter];
    // if (countout++ < 30) {
    //   std::cout << "AT: " << myXMissings.size() << " " << myMCounter << ", length " << l << " position " <<
    //     myRLECounter << " and (" << x << ", " << y << ", " << z << ")\n";
    // }
    v  = Constants::MissingData;
    w  = 0.0;
    w2 = 0.0;

    // Update for the next missing, if any
    myRLECounter++;
    if (myRLECounter >= l) { // overflow
      myRLECounter = 0;
      myMCounter++;
    }
    return true;
  }

  // Nothing left...
  // std::cout << "END CONDITION " << myMCounter << " and " << myXMissings.size() << "\n";
  return false;
} // Stage2Data::get
