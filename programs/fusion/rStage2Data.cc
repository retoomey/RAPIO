#include "rStage2Data.h"
#include "rFusion1.h"
#include "rBitset.h"

using namespace rapio;

void
Stage2Data::add(float v, float w, short x, short y, short z)
{
  if (v == Constants::MissingData) {
    // Only store x,y,z for a missing
    // Since missing tends to be grouped in space, we'll do a simple RLE to compress x,y,z locations
    // FIXME: Should probably have a way to turn on/off this feature for testing/comparison
    // Storing a bit per x,y,z. marking MissingData
    size_t i = myMissingSet.getIndex({ (size_t) (x), (size_t) (y), (size_t) (z) });
    myMissingSet.set(i, 1);

    // If not rle, we could just store x,y,z without any length value
    // myXMissings.push_back(x);
    // myYMissings.push_back(y);
    // myZMissings.push_back(z); // We're hiding that z is a char
  } else {
    myValues.push_back(v);
    myWeights.push_back(w);
    myXs.push_back(x);
    myYs.push_back(y);
    myZs.push_back(z); // We're hiding that z is a char
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
        int flag     = myMissingSet.get<int>(index);
        if (flag) {
          counter++;
          size_t startx = x;
          size_t starty = y;
          size_t startz = z;
          size_t length = 1;
          // Extend the run horizontally
          while (x + 1 < myDimensions[0]) {
            size_t index = myMissingSet.getIndex({ (size_t) x + 1, (size_t) y, (size_t) z });
            int value    = myMissingSet.get<int>(index);
            if (value) {
              ++x;
              ++length;
            } else {
              break;
            }
            // New RLE stuff to store...
          }
          if (counter < 10) {
            //  std::cout << "RLE: " << startx << ", " << starty << ", " << startz << " --> " << length << "\n";
          }
          myXMissings.push_back(x);
          myYMissings.push_back(y);
          myZMissings.push_back(z);
          myLMissings.push_back(length); // has to be big enough for all of conus
        }
      }
    }
  }
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

  LogInfo("Sizes are " << finalSize << " values, with " << finalSize2 << " missing\n");
  if ((finalSize < 1) && (finalSize2 < 1)) {
    // FIXME: Humm do we still want to send 'something' for push triggering?  Do we need to?
    // Would be better if we don't have to
    LogInfo("--->Special case of size zero...ignoring stage2 output currently.\n");
  } else {
    auto stage2 = DataGrid::Create(asName, "dimensionless", aLocation, aTime, { finalSize, finalSize2 }, { "I", "MI" });
    stage2->setDataType("Stage2Ingest"); // DataType attribute in the file not filename
    stage2->setSubType("Full");
    stage2->setString("Radarname", myRadarName);
    stage2->setString("Typename", myTypeName);
    // FIXME: general in MRMS and RAPIO
    stage2->setString("NetcdfWriterInfo", "RAPIO:Fusion stage 2 data");
    stage2->setLocation(myCenter); // Radar center location
    //    const LLH center        = r->getRadarLocation();
    // FIXME: Wondering if grouping x,y,z would work better with prefetch.  Difficult
    // unless we use same size for all three
    // FIXME: We 'should' be able to use a 'baseX' and 'baseY' for small radars
    // and store this silly small.  Radar will only cover so many squares
    auto& netcdfX = stage2->addShort1DRef("X", "dimensionless", { 0 });
    auto& netcdfY = stage2->addShort1DRef("Y", "dimensionless", { 0 });
    auto& netcdfZ = stage2->addByte1DRef("Z", "dimensionless", { 0 });

    // Missing values aren't stored
    auto& netcdfXM = stage2->addShort1DRef("Xm", "dimensionless", { 1 });
    auto& netcdfYM = stage2->addShort1DRef("Ym", "dimensionless", { 1 });
    auto& netcdfZM = stage2->addByte1DRef("Zm", "dimensionless", { 1 });
    auto& netcdfLM = stage2->addShort1DRef("Lm", "dimensionless", { 1 }); // max 65000
    // Could be a double or float or whatever here...we could also
    // trim to a precision
    auto& netcdfValues = stage2->addFloat1DRef(Constants::PrimaryDataName, myUnits, { 0 });
    auto& netcdfWeight = stage2->addFloat1DRef("Range", "dimensionless", { 0 });

    // Copying because size is dynamic.  FIXME: Could improve API to handle dynamic arrays?
    std::copy(myXs.begin(), myXs.end(), netcdfX.data());
    std::copy(myYs.begin(), myYs.end(), netcdfY.data());
    std::copy(myZs.begin(), myZs.end(), netcdfZ.data());
    std::copy(myValues.begin(), myValues.end(), netcdfValues.data());
    std::copy(myWeights.begin(), myWeights.end(), netcdfWeight.data());

    std::copy(myXMissings.begin(), myXMissings.end(), netcdfXM.data());
    std::copy(myYMissings.begin(), myYMissings.end(), netcdfYM.data());
    std::copy(myZMissings.begin(), myZMissings.end(), netcdfZM.data());
    std::copy(myLMissings.begin(), myLMissings.end(), netcdfLM.data());

    // We 'could' return the stage2 object let the algorithm write it...but we can hide
    // it here I think for moment just in case we end up doing something different
    std::map<std::string, std::string> extraParams; // FIXME: yeah I know, need to constant/API cleanup this stuff
    extraParams["showfilesize"] = "yes";
    alg->writeOutputProduct(asName, stage2, extraParams);
  }
  // -------------------------------------------------------
} // Stage2Data::send

void
Stage2Data::receive(RAPIOData& d)
{
  // Stage two reading in stage one files
  auto gsp = d.datatype<rapio::DataGrid>();

  if (gsp != nullptr) {
    // if (gsp->getTypeName() == "check for reflectivity, etc" if subgrouping
    if (gsp->getDataType() == "Stage2Ingest") { // Check for ANY stage2 file
      std::string s;
      auto sel = d.record().getSelections();
      for (auto& s1:sel) {
        s = s + s1 + " ";
      }

      LogInfo("Stage2 data noticed: " << s << "\n");
      DataGrid& d = *gsp;

      auto dims     = d.getDims();
      size_t aSize  = dims[0].size();
      size_t aSize2 = dims[1].size();
      // const std::string name = d[0].name();
      // Short code but no checks for failure
      // FIXME: We're probably going to need a common shared class/library with Stage1 at some point
      // so we don't end up offsyncing this stuff
      auto& netcdfX      = d.getShort1DRef("X");
      auto& netcdfY      = d.getShort1DRef("Y");
      auto& netcdfZ      = d.getShort1DRef("Z");
      auto& netcdfValues = d.getFloat1DRef();
      auto& netcdfWeight = d.getFloat1DRef("Range");

      // Get the RLE total size
      auto& RLElengths = d.getShort1DRef("Lm");
      size_t length    = 0;
      for (size_t i = 0; i < aSize2; i++) {
        length += RLElengths[i];
      }

      LogInfo(
        "Size is " << aSize << " x,y,z non-missing values, and " << aSize2 << " RLE missing values expanding to " << length <<
          " total missing.\n");
      if (aSize > 10) { aSize = 10; }
      for (size_t i = 0; i < aSize; i++) {
        LogInfo(
          "   " << i << ": (" << netcdfX[i] << "," << netcdfY[i] << ") stores " << netcdfValues[i] << " with weight " <<
            netcdfWeight[i] << "\n");
      }
    } else {
      LogSevere("We got unrecognized data:" << gsp->getDataType() << "\n");
    }
  }
} // Stage2Data::receive

// Backing up the 'raw' file stuff from fusion1 but I don't want it in there.  If we implement
// it then it would belong in this class
#if 0

std::shared_ptr<RObsBinaryTable>
RAPIOFusionOneAlg::createRawEntries(AngleDegs elevDegs,
  AngleDegs cLat, AngleDegs cLon, LengthKMs cHeight,
  const std::string& aTypeName, const std::string& aUnits,
  const time_t dataTime)
{
  // Entries to write out eventually
  std::shared_ptr<RObsBinaryTable> entries = std::make_shared<RObsBinaryTable>();

  entries->setReadFactory("raw"); // default to raw output if we write it
  auto& t = *entries;

  // FIXME: refactor/methods, etc. This isn't clean by a long shot
  t.radarName = myRadarName;
  t.vcp       = -9999; // Ok will merger be ok with this, need to check
  t.elev      = elevDegs;
  t.typeName  = aTypeName;
  t.setUnits(aUnits);
  t.markedLinesCacheFile = ""; // We ignore marked lines.  We'll see how it goes...
  t.lat       = cLat;          // lat degrees
  t.lon       = cLon;          // lon degrees
  t.ht        = cHeight;       // kilometers assumed by w2merger
  t.data_time = dataTime;
  // Ok Lak guesses valid time based on vcp. What do we do here?
  // I'm gonna use the provided history value
  // If (valid_time < 1 || valid_time < blendingInterval ){
  //   valid_time = blendingInterval;
  // }
  t.valid_time = getMaximumHistory().seconds(); // Use seconds of our -h option?
  return entries;
}

void
RAPIOFusionOneAlg::addRawEntry(const VolumeValue& vv, const Time& time, RObsBinaryTable& t, size_t x, size_t y,
  size_t layer)
{
  // FIXME: Humm we could store time parts in vv.  This is a hack to match times between the two
  // systems.
  RObsBinaryTable::mrmstime aMRMSTime;

  aMRMSTime.epoch_sec = time.getSecondsSinceEpoch();
  aMRMSTime.frac_sec  = time.getFractional();
  const LLCoverageArea& outg = myRadarGrid;

  const size_t outX = x + outg.startX;
  const size_t outY = y + outg.startY;

  // Add entries
  //   t.elevWeightScaled.push_back(int (0.5+(1+90)*100) );

  # define ELEV_SCALE(x) ( int(0.5 + (x + 90) * 100) )
  # define RANGE_SCALE 10

  // if (outX > myFullGrid.numX) {
  //  LogSevere(" CRITICAL X " << outX << " > " << myFullGrid.numX << "\n");
  //  exit(1);
  // }
  // if (outY > myFullGrid.numY) {
  //  LogSevere(" CRITICAL Y " << outY << " > " << myFullGrid.numY << "\n");
  //  exit(1);
  // }

  // Just realized Z will be wrong, since the stage 2 will assume all 33 layers, and
  // we're just doing subsets maybe.  So we'll need to figure that out
  // Lak: 33 2200 2600 as X, Y, Z.  Which seems non-intuitive to me.
  // Me: 2200 2600 33 as X, Y, Z.
  const size_t atZ = myHeightsIndex[layer]; // hack sparse height layer index (or we make object?)
  t.addRawObservation(atZ, outX, outY, vv.dataValue,
    int(vv.virtualRangeKMs * RANGE_SCALE + 0.5),
    //	  entry.elevWeightScaled(), Can't figure this out.  Why a individual scale...oh weight because the
    //	  cloud is the full cube.  Ok so we take current elev..no wait the virtual elevation and scale to char...
    //	  takes -90 to 90 to 0-18000, can still fit into unsigned char...
    //
    ELEV_SCALE(vv.virtualElevDegs),
    vv.virtualAzDegs, aMRMSTime);

  // Per point (I'm gonna have to rework it if I do a new stage 2):
  // t.azimuth  vector of azimuth of the point, right?  Ok we have that.
  // t.aztime  vector of time of the azimuth.  Don't think we have that really.  Will make it the radial set
  // t.x, y, z,  coordinates in the global grid
  // newvalue,
  // scaled_dist (short).   A Scale of the weight for the point:
  // 'quality' was passed into as RadialSet...ignoring it for now not sure it's used
  // RANGE_SCALE = 10
  // int(rangeMeters*RANGE_SCALE_KM*quality + 0.5);
  // int(rangeMeters*RANGE_SCALE_KM + 0.5);
  //    RANGE_SCALE_KM = RANGE_SCALE/1000.0
  // elevWeightScaled (char).
  // AZIMUTH_SCALE 100
  // ELEV_SCALE(x) = ( int (0.5+(x+90)*100) )
  // ELEV_UNSCALE(y) ( (y*0.01)-90)
  // ELEV_WT_SCALE 100
  // I think the stage 2 entries are gonna be difficult to match, there's information
  // that we can't include I think.  Stage 2 is trying to do things we already did?
  // Bleh maybe it will be good enough.  We'll need a better stage 2 I think.
  // FIXME: clean up so much here
  // entries.azimuth.push_back(int(0.5+azDegs*100)); // scale right?
  // RObsBinaryTable::mrmstime aTime;
  // FIXME: fill in time.  Ok what time do we use here?
  // entries.aztime.push_back(aTime); // scale right?
} // RAPIOFusionOneAlg::addRawEntry

#endif // if 0
