#include "rFusion2.h"
#include "rBinaryTable.h"

#include "boost/units/quantity.hpp"
#include "boost/units/systems/si/length.hpp"
#include "boost/units/base_units/imperial/mile.hpp"

// #include "boost/units/systems/si.hpp"
// #include "boost/units/systems/si/prefixes.hpp"
// #include "boost/units/io.hpp"

using namespace rapio;


/** Non-virtual class for storing a type checked value */
class UnitBase
{
public:
  inline float get(){ return value; }

  UnitBase(){ };
  UnitBase(float v) : value(v){ };

protected:
  float value;
};

/** These classes must be non-virtual so they vector store, etc. as the primitive.
 * MRMS used say 'Angle' class generically but you'd have speed issues with internal storage
 * since conversions would be hidden from the user.  Appending the type allows us type
 * safety as well as storage ability */
class AngleRads2;
class AngleDegs2;

class AngleDegs2 : public UnitBase
{
public:
  // AngleDegs2(float v):value(v){};
  AngleDegs2(float v) : UnitBase(v){ };
  AngleRads2
  toRadians();
private:
  // float value;
};

class AngleRads2 : public UnitBase
{
public:
  // AngleRads2(float v):value(v){};
  AngleRads2(float v) : UnitBase(v){ };
  AngleDegs2
  toDegrees();
private:
  //  float value;
};

AngleRads2 AngleDegs2::toRadians(){  return AngleRads2(value / 2.0); }

AngleDegs2 AngleRads2::toDegrees(){  return AngleDegs2(value * 2.0); }

void
RAPIOFusionTwoAlg::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "RAPIO Fusion Stage Two Algorithm.  Gathers data from multiple radars for mergin values");
  o.setAuthors("Robert Toomey");

  // legacy use the t, b and s to define a grid
  o.declareLegacyGrid();
}

/** RAPIOAlgorithms process options on start up */
void
RAPIOFusionTwoAlg::processOptions(RAPIOOptions& o)
{
  o.getLegacyGrid(myFullGrid);
} // RAPIOFusionTwoAlg::processOptions

void
RAPIOFusionTwoAlg::processNewData(rapio::RAPIOData& d)
{
  #if 0
  // FIXME: Add this ability to API?
  std::string s = "Data received: ";
  auto sel      = d.record().getSelections();

  for (auto& s1:sel) {
    s = s + s1 + " ";
  }
  LogInfo(s << " from index " << d.matchedIndexNumber() << "\n");
  #endif

  auto gsp = d.datatype<rapio::DataGrid>();

  if (gsp != nullptr) {
    if (gsp->getDataType() == "Stage2Ingest") { // Check for ANY stage2 file
      // if (gsp->getTypeName() == "check for reflectivity, etc" if subgrouping
      std::string s;
      auto sel = d.record().getSelections();
      for (auto& s1:sel) {
        s = s + s1 + " ";
      }

      LogInfo("Stage2 data noticed: " << s << "\n");
      DataGrid& d = *gsp;

      auto dims    = d.getDims();
      size_t aSize = dims[0].size();
      // const std::string name = d[0].name();
      // Short code but no checks for failure
      // FIXME: We're probably going to need a common shared class/library with Stage1 at some point
      // so we don't end up offsyncing this stuff
      auto& netcdfX      = d.getShort1DRef("X");
      auto& netcdfY      = d.getShort1DRef("Y");
      auto& netcdfValues = d.getFloat1DRef();
      auto& netcdfWeight = d.getFloat1DRef("Range");
      LogInfo("Size is " << aSize << "\n");
      if (aSize > 10) { aSize = 10; }
      for (size_t i = 0; i < aSize; i++) {
        LogInfo(
          "   " << i << ": (" << netcdfX[i] << "," << netcdfY[i] << ") stores " << netcdfValues[i] << " with weight " <<
            netcdfWeight[i] << "\n");
      }
    }
  }
} // RAPIOFusionTwoAlg::processNewData

int
main(int argc, char * argv[])
{
  // Create and run a tile alg
  RAPIOFusionTwoAlg alg = RAPIOFusionTwoAlg();

  alg.executeFromArgs(argc, argv);
}
