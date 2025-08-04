#pragma once

/** RAPIO API */
#include <RAPIO.h>

namespace rapio {
/*
 * Generic dump of legend tool.
 * This currently creates an SVG output for
 * a webdisplay legend based off the datatype of
 * the referenced file.
 *
 * @author Robert Toomey
 **/
class GetLegend : public rapio::RAPIOAlgorithm {
public:

  /** Create */
  GetLegend(){ };

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Process a new record/datatype.  See the .cc for RAPIOData info */
  virtual void
  processNewData(rapio::RAPIOData& d) override;

protected:
  /** Get the width of the legend */
  size_t myWidth;

  /** Get the height of the legend */
  size_t myHeight;
};
}
