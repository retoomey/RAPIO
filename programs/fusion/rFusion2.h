#pragma once

/** RAPIO API */
#include <RAPIO.h>
#include "rLLCoverageArea.h"

namespace rapio {
/**
 * Second stage merger/fusion algorithm.  Gathers input from single stage radar
 * processes for the intention of merging data into a final output.
 * For each tilt, a subset of R radars from a larger set of N radars is used,
 * where as the number of tiles increase, R drops due to geographic coverage.
 * Thus we have R subset of or equal to T.
 * We should horizontally scale by breaking into subtiles that cover the larger
 * merger tile.
 * Some goals:
 * 1.  Automatically tile based on given tiling sizes, vs merger using hard coded
 *     lat/lon values.
 * 2.  Attempt to time synchronize to avoid discontinuities along tile edges.
 *     This will be important to allow more than the 4 hardcoded tiles we use
 *     in the current w2merger.
 * 3.  Possibly send data using web/REST vs the raw/fml notification.
 *
 * @author Robert Toomey
 * @author Valliappa Lakshman
 *
 **/
class RAPIOFusionTwoAlg : public rapio::RAPIOAlgorithm {
public:

  /** Create tile algorithm */
  RAPIOFusionTwoAlg(){ };

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Process a new record/datatype */
  virtual void
  processNewData(rapio::RAPIOData& d) override;

protected:

  /** Coordinates for the total merger grid */
  LLCoverageArea myFullGrid;
};
}
