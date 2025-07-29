#pragma once

/** RAPIO API */
#include <RAPIO.h>

namespace rapio {
/*
 *  Remap tool designed for remapping grid classes such
 *  as LatLonGrid to another grid
 *
 * @author Robert Toomey
 **/
class Remap : public rapio::RAPIOAlgorithm {
public:

  /** Create an example simple algorithm */
  Remap(){ };

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Process a new record/datatype.  See the .cc for RAPIOData info */
  virtual void
  processNewData(rapio::RAPIOData& d) override;

  /** Remap in the algorithm for moment */
  void
  remap(std::shared_ptr<LatLonGrid> llg);

  /** Remap in the algorithm for moment */
  void
  remap(std::shared_ptr<RadialSet> rs);

protected:

  /** Coordinates for the output grid we remap to */
  LLCoverageArea myFullGrid;

  /** Mode of remap such as nearest or cressman */
  std::string myMode;

  /** Size of the matrix for remapping */
  size_t mySize;

  // Radial set only parameters

  /** Gate width in meters of output */
  int myGateWidthMeters;

  /** Number of radials to output */
  int myNumRadials;

  /** Number of gates to output */
  int myNumGates;

  /** Project RadialSet to ground? */
  bool myProjectGround;
};
}
