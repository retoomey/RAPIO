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
  LogSevere("Got something...\n");
} // RAPIOFusionTwoAlg::processNewData

int
main(int argc, char * argv[])
{
  // Create and run a tile alg
  RAPIOFusionTwoAlg alg = RAPIOFusionTwoAlg();

  alg.executeFromArgs(argc, argv);
}
