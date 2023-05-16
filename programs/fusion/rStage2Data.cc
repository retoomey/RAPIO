#include "rStage2Data.h"
#include "rFusion1.h"

using namespace rapio;

void
Stage2Data::send(RAPIOFusionOneAlg * alg, Time aTime, const std::string& asName)
{
  LLH aLocation;
  size_t finalSize = stage2Xs.size();

  if (finalSize < 1) {
    // FIXME: Humm do we still want to send 'something' for push triggering?  Do we need to?
    // Would be better if we don't have to
    LogInfo("--->Special case of size zero...ignoring stage2 output currently.\n");
  } else {
    auto stage2 = DataGrid::Create(asName, "dimensionless", aLocation, aTime, { finalSize }, { "I" });
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
    // Could be a double or float or whatever here...we could also
    // trim to a precision
    auto& netcdfValues = stage2->addFloat1DRef(Constants::PrimaryDataName, myUnits, { 0 });
    auto& netcdfWeight = stage2->addFloat1DRef("Range", "dimensionless", { 0 });

    // Copying because size is dynamic.  FIXME: Could improve API to handle dynamic arrays?
    std::copy(stage2Xs.begin(), stage2Xs.end(), netcdfX.data());
    std::copy(stage2Ys.begin(), stage2Ys.end(), netcdfY.data());
    std::copy(stage2Zs.begin(), stage2Zs.end(), netcdfZ.data());
    std::copy(stage2Values.begin(), stage2Values.end(), netcdfValues.data());
    std::copy(stage2Weights.begin(), stage2Weights.end(), netcdfWeight.data());

    // We 'could' return the stage2 object let the algorithm write it...but we can hide
    // it here I think for moment just in case we end up doing something different
    alg->writeOutputProduct(asName, stage2);
  }
  // -------------------------------------------------------
} // Stage2Data::send

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
