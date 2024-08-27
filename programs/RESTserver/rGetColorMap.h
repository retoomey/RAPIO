#pragma once

/** RAPIO API */
#include <RAPIO.h>

namespace rapio {
/*
 * Fetch color map in JSON tool.
 * This currently creates an JSON output for
 * a webdisplay color map application for our
 * custom data leaflet module.
 *
 * @author Robert Toomey
 **/
class GetColorMap : public rapio::RAPIOAlgorithm {
public:

  /** Create */
  GetColorMap(){ };

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
};
}
