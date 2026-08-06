#include "rStage2Data.h"
#include "rFusion1.h"
#include "rBitset.h"
#include "rColorTerm.h"

using namespace rapio;

void
Stage2Storage::RLE()
{
  size_t counter = 0;

  for (size_t z = 0; z < myDimensions[2]; z++) {
    for (size_t y = 0; y < myDimensions[1]; y++) {
      for (size_t x = 0; x < myDimensions[0]; x++) {
        auto flag = myMissingSet.get13D(x, y, z);
        if (flag) {
          counter++;
          size_t startx = x;
          size_t starty = y;
          size_t startz = z;
          size_t length = 1;
          while (x + 1 < myDimensions[0]) {
            auto value = myMissingSet.get13D(x + 1, y, z);
            if (value) {
              ++x;
              ++length;
            } else {
              break;
            }
          }
          myTable->addMissing(startx, starty, startz, length);
        }
      }
    }
  }
}

void
Stage2Data::send(RAPIOAlgorithm * alg, Time aTime, const std::string& asName)
{
  size_t finalValueSize      = 0;
  size_t finalMissingRLESize = 0;
  size_t finalMissingSize    = 0;
  size_t count = 0;

  fLogInfo("{}{}---Outputting---{}", ColorTerm::green(), ColorTerm::bold(), ColorTerm::reset());
  for (auto& s:myStorage) {
    s->send(alg, aTime, asName);
    finalValueSize      += s->getAddedValueCount();
    finalMissingSize    += s->getAddedMissingCount();
    finalMissingRLESize += s->getSentMissingRLECount();
    count++;
  }
  fLogInfo("Sent {} total tiles, Values: {}, Missings: {}, (RLE: {})", count, finalValueSize, finalMissingSize,
    finalMissingRLESize);
}

void
Stage2Storage::send(RAPIOAlgorithm * alg, Time aTime, const std::string& asName)
{
  RLE();

  LLH aLocation;
  const size_t finalSize  = myTable->getValueSize();
  const size_t finalSize2 = myTable->getMissingSize();

  myMissingRLECounter = finalSize2;

  if (finalSize != myAddValueCounter) {
    fLogSevere("Table size/added are different? {}  {}", finalSize, myAddValueCounter);
  }
  if ((finalSize < 1) && (finalSize2 < 1)) {
    fLogInfo("Skipping writing {} since we have 0 values.", mySubFolder);
  } else {
    fLogInfo("Writing {} values, with {} missing as {} (RLE)", finalSize, myAddMissingCounter, finalSize2);

    IOConfig extraParams;
    extraParams.set("showfilesize", "yes");
    extraParams.set("outputsubfolder", mySubFolder);

    if (alg->isProductWanted("S2Netcdf")) {
      auto stage2 = DataGrid::Create(asName, "dimensionless", aLocation, aTime, { finalSize, finalSize2 }, { "I", "MI" });

      stage2->setString("Sourcename", getRadarName());
      stage2->setDataType("Stage2Ingest");
      stage2->setSubType("S2");
      stage2->setString("Radarname", getRadarName());
      stage2->setString("Typename", getTypeName());
      stage2->setLong("xBase", getXBase());
      stage2->setLong("yBase", getYBase());
      stage2->setUnits(getUnits());
      stage2->setLocation(getLocation());
      stage2->setString("NetcdfWriterInfo", "RAPIO:Fusion stage 2 data");

      auto& netcdfX    = stage2->addShort1DRef("X", "dimensionless", { 0 });
      auto& netcdfY    = stage2->addShort1DRef("Y", "dimensionless", { 0 });
      auto& netcdfZ    = stage2->addByte1DRef("Z", "dimensionless", { 0 });
      auto& netcdfNums = stage2->addFloat1DRef(Constants::PrimaryDataName, getUnits(), { 0 });
      auto& netcdfDems = stage2->addFloat1DRef("D", "dimensionless", { 0 });

      std::copy(myTable->myXs.begin(), myTable->myXs.end(), netcdfX.data());
      std::copy(myTable->myYs.begin(), myTable->myYs.end(), netcdfY.data());
      std::copy(myTable->myZs.begin(), myTable->myZs.end(), netcdfZ.data());
      std::copy(myTable->myNums.begin(), myTable->myNums.end(), netcdfNums.data());
      std::copy(myTable->myDems.begin(), myTable->myDems.end(), netcdfDems.data());

      auto& netcdfXM = stage2->addShort1DRef("Xm", "dimensionless", { 1 });
      auto& netcdfYM = stage2->addShort1DRef("Ym", "dimensionless", { 1 });
      auto& netcdfZM = stage2->addByte1DRef("Zm", "dimensionless", { 1 });
      auto& netcdfLM = stage2->addShort1DRef("Lm", "dimensionless", { 1 }); 

      std::copy(myTable->myXMissings.begin(), myTable->myXMissings.end(), netcdfXM.data());
      std::copy(myTable->myYMissings.begin(), myTable->myYMissings.end(), netcdfYM.data());
      std::copy(myTable->myZMissings.begin(), myTable->myZMissings.end(), netcdfZM.data());
      std::copy(myTable->myLMissings.begin(), myTable->myLMissings.end(), netcdfLM.data());

      extraParams.set("compression", "gz");
      stage2->setTypeName(asName);
      alg->writeOutputProduct("S2Netcdf", stage2, extraParams);
    }

    if (alg->isProductWanted("S2")) {
      myTable->setSubType("S2");
      myTable->setTime(aTime);
      myTable->setTypeName(asName);
      alg->writeOutputProduct("S2", myTable, extraParams);
    }
  }
}
