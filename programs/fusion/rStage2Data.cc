#include "rStage2Data.h"
#include "rFusion1.h"
#include "rBitset.h"

using namespace rapio;

void
Stage2Storage::RLE()
{
  // RLE in 2D space left to right.  Missing typically groups up in blocks, this is similar I think
  // to the marked lines stuff in w2merger
  // FIXME: Maybe create general RLE interface for other uses

  size_t counter = 0;

  for (size_t z = 0; z < myDimensions[2]; z++) {
    for (size_t y = 0; y < myDimensions[1]; y++) {   // row
      for (size_t x = 0; x < myDimensions[0]; x++) { // col
        auto flag = myMissingSet.get13D(x, y, z);
        if (flag) {
          counter++;
          size_t startx = x;
          size_t starty = y;
          size_t startz = z;
          size_t length = 1;
          // Extend the run horizontally
          while (x + 1 < myDimensions[0]) {
            auto value = myMissingSet.get13D(x, y, z);
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
          myTable->addMissing(startx, starty, startz, length);
        }
      }
    }
  }
  // LogInfo("RLE found " << counter << " start locations.  Dim size: " << myDimensions[0] << " , " << myDimensions[1] << ", " << myDimensions[2] <<  "\n");
} // Stage2Storage::RLE

void
Stage2Data::send(RAPIOAlgorithm * alg, Time aTime, const std::string& asName)
{
  // Now we send each of the tiles or areas we have
  size_t finalValueSize      = 0;
  size_t finalMissingRLESize = 0;
  size_t finalMissingSize    = 0;
  size_t count = 0;

  LogInfo(ColorTerm::green() << ColorTerm::bold() << "---Outputting---" << ColorTerm::reset() << "\n");
  for (auto& s:myStorage) {
    s->send(alg, aTime, asName);
    finalValueSize      += s->getAddedValueCount();
    finalMissingSize    += s->getAddedMissingCount();
    finalMissingRLESize += s->getSentMissingRLECount();
    count++;
  }
  LogInfo(
    "Sent " << count << " total tiles, Values: " << finalValueSize << ", Missings: " << finalMissingSize << ", (RLE: " << finalMissingRLESize <<
      ")\n");
}

void
Stage2Storage::send(RAPIOAlgorithm * alg, Time aTime, const std::string& asName)
{
  // Run length encode the missing bit array.  Since missing groups up this saves quite
  // a bit of space.
  RLE();

  // Info from our data
  LLH aLocation;
  const size_t finalSize  = myTable->getValueSize();
  const size_t finalSize2 = myTable->getMissingSize();

  myMissingRLECounter = finalSize2;

  if (finalSize != myAddValueCounter) {
    LogSevere("Table size/added are different? " << finalSize << "  " << myAddValueCounter << "\n");
  }
  if ((finalSize < 1) && (finalSize2 < 1)) {
    LogInfo("Skipping writing " << mySubFolder << " since we have 0 values.\n");
  } else {
    LogInfo(
      "Writing " << finalSize << " values, with " << myAddMissingCounter << " missing as " << finalSize2 <<
        " (RLE).\n");
    std::map<std::string, std::string> extraParams;
    extraParams["showfilesize"]    = "yes";
    extraParams["outputsubfolder"] = mySubFolder;

    // Writing netcdf.  This is more generic readable so I like it for that reason, however
    // our I/O is so critical we tend to use a custom binary for read/write speed at the cost
    // of external readability.

    // FIXME: probably should have a toDataGrid method or something in BinaryTable.
    if (alg->isProductWanted("S2Netcdf")) {
      auto stage2 =
        DataGrid::Create(asName, "dimensionless", aLocation, aTime, { finalSize, finalSize2 }, { "I", "MI" });

      // This is general settings for pathing...
      stage2->setString("Sourcename", getRadarName());
      stage2->setDataType("Stage2Ingest"); // DataType attribute in the file not filename
      stage2->setSubType("S2");

      // Netcdf specific stuff (FIXME: could have a copy all atrributes?)
      stage2->setString("Radarname", getRadarName());
      stage2->setString("Typename", getTypeName());
      stage2->setLong("xBase", getXBase());
      stage2->setLong("yBase", getYBase());
      stage2->setUnits(getUnits());
      stage2->setLocation(getLocation()); // Radar center location
      // FIXME: general in MRMS and RAPIO
      stage2->setString("NetcdfWriterInfo", "RAPIO:Fusion stage 2 data");

      // ---------------------------------------------------
      // Value per point storage (dim 0)
      //
      // FIXME: We 'should' be able to use a 'baseX' and 'baseY' for small radars
      // and store this silly small.  Radar will only cover so many squares
      auto& netcdfX    = stage2->addShort1DRef("X", "dimensionless", { 0 });
      auto& netcdfY    = stage2->addShort1DRef("Y", "dimensionless", { 0 });
      auto& netcdfZ    = stage2->addByte1DRef("Z", "dimensionless", { 0 });
      auto& netcdfNums = stage2->addFloat1DRef(Constants::PrimaryDataName, getUnits(), { 0 });
      auto& netcdfDems = stage2->addFloat1DRef("D", "dimensionless", { 0 });

      // Copying because size is dynamic.  FIXME: Could improve API to handle dynamic arrays?
      // Since we deal with 'small' radar coverage here, think we're ok for moment
      std::copy(myTable->myXs.begin(), myTable->myXs.end(), netcdfX.data());
      std::copy(myTable->myYs.begin(), myTable->myYs.end(), netcdfY.data());
      std::copy(myTable->myZs.begin(), myTable->myZs.end(), netcdfZ.data());
      std::copy(myTable->myNums.begin(), myTable->myNums.end(), netcdfNums.data());
      std::copy(myTable->myDems.begin(), myTable->myDems.end(), netcdfDems.data());

      // ---------------------------------------------------
      // Missing value storage (dim 1)
      //
      auto& netcdfXM = stage2->addShort1DRef("Xm", "dimensionless", { 1 });
      auto& netcdfYM = stage2->addShort1DRef("Ym", "dimensionless", { 1 });
      auto& netcdfZM = stage2->addByte1DRef("Zm", "dimensionless", { 1 });
      auto& netcdfLM = stage2->addShort1DRef("Lm", "dimensionless", { 1 }); // max 65000

      std::copy(myTable->myXMissings.begin(), myTable->myXMissings.end(), netcdfXM.data());
      std::copy(myTable->myYMissings.begin(), myTable->myYMissings.end(), netcdfYM.data());
      std::copy(myTable->myZMissings.begin(), myTable->myZMissings.end(), netcdfZM.data());
      std::copy(myTable->myLMissings.begin(), myTable->myLMissings.end(), netcdfLM.data());

      // We 'could' return the stage2 object let the algorithm write it...but we can hide
      // it here I think for moment just in case we end up doing something different
      extraParams["compression"] = "gz";
      stage2->setTypeName(asName);
      alg->writeOutputProduct("S2Netcdf", stage2, extraParams);
    }

    if (alg->isProductWanted("S2")) {
      // Faster but less generally readable.  Though rdump 'should' be able to read it if iotext if up2date.
      myTable->setSubType("S2");
      myTable->setTime(aTime);

      // Write the raw file out with notification, etc.
      myTable->setTypeName(asName);
      alg->writeOutputProduct("S2", myTable, extraParams);
    }
  }
  // -------------------------------------------------------
} // Stage2Data::send

std::shared_ptr<Stage2Data>
Stage2Data::receive(RAPIOData& rData)
{
  // Stage two reading in stage one files
  auto gsp = rData.datatype<rapio::DataGrid>(); // netcdf

  if (gsp != nullptr) {
    auto& d = *gsp;
    // if (gsp->getTypeName() == "check for reflectivity, etc" if subgrouping
    // FIXME: we could let it be DataGrid and check fields instead.  This would stop the
    // warning we get for missing reader
    if (gsp->getDataType() == "Stage2Ingest") { // Check for ANY stage2 file
      LogInfo("Stage2 data noticed (netcdf) record: " << rData.getDescription() << "\n");
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
      d.getString("Radarname", radarName);
      d.getString("Typename", typeName);
      long xBase = 0, yBase = 0;
      d.getLong("xBase", xBase);
      d.getLong("yBase", yBase);
      units = d.getUnits();
      LLH center = d.getLocation();
      std::shared_ptr<Stage2Data> insp =
        std::make_shared<Stage2Data>(Stage2Data(radarName, typeName, units, center, xBase, yBase, dims));
      auto& in = *insp;
      in.setTime(d.getTime());

      // ---------------------------------------------------
      // Value per point storage (dim 0)
      //
      auto& netcdfX    = d.getShort1DRef("X");
      auto& netcdfY    = d.getShort1DRef("Y");
      auto& netcdfZ    = d.getByte1DRef("Z");
      auto& netcdfNums = d.getFloat1DRef();
      auto& netcdfDems = d.getFloat1DRef("D");

      in.myTable->myXs.reserve(aSize);
      in.myTable->myYs.reserve(aSize);
      in.myTable->myZs.reserve(aSize);
      in.myTable->myNums.reserve(aSize);
      in.myTable->myDems.reserve(aSize);
      for (size_t i = 0; i < aSize; i++) {
        in.myTable->myXs.push_back(netcdfX[i]);
        in.myTable->myYs.push_back(netcdfY[i]);
        in.myTable->myZs.push_back(netcdfZ[i]);
        in.myTable->myNums.push_back(netcdfNums[i]);
        in.myTable->myDems.push_back(netcdfDems[i]);
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
          "   " << i << ": (" << netcdfX[i] << "," << netcdfY[i] << ") stores " << netcdfNums[i] << " with weight " <<
            netcdfDems[i] << "\n");
      }

      in.myTable->myXMissings.reserve(aSize2);
      in.myTable->myYMissings.reserve(aSize2);
      in.myTable->myZMissings.reserve(aSize2);
      in.myTable->myLMissings.reserve(aSize2);
      for (size_t i = 0; i < aSize2; i++) {
        in.myTable->myXMissings.push_back(netcdfXM[i]);
        in.myTable->myYMissings.push_back(netcdfYM[i]);
        in.myTable->myZMissings.push_back(netcdfZM[i]);
        in.myTable->myLMissings.push_back(RLElengths[i]);
      }

      return insp;
    } else {
      //   LogSevere("We got unrecognized data:" << gsp->getDataType() << "\n");
    }
  }

  // Stage two reading in raw files (a bit simplier right?)
  auto ft = rData.datatype<FusionBinaryTable>();

  if (ft != nullptr) {
    LogInfo("Stage2 data noticed (raw) record: " << rData.getDescription() << "\n");
    std::vector<size_t> dims = { ft->getValueSize(), ft->getMissingSize() };
    LogInfo("Value/missing dims: " << ft->getValueSize() << ", " << ft->getMissingSize() << "\n");
    std::shared_ptr<Stage2Data> insp =
      std::make_shared<Stage2Data>(Stage2Data(ft, dims));
    return insp;
  }
  return nullptr;
} // Stage2Data::receive
