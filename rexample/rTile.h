#pragma once

/** RAPIO API */
#include <RAPIO.h>

namespace rapio {
class RAPIOTileAlg : public rapio::RAPIOAlgorithm {
public:

  /** Create tile algorithm */
  RAPIOTileAlg(){ };

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Process a new record/datatype */
  virtual void
  processNewData(rapio::RAPIOData& d) override;

  /** Process a web message */
  virtual void
  processWebMessage(std::shared_ptr<WebMessage> message) override;

protected:

  /** Override output params for image output */
  std::map<std::string, std::string> myOverride;

  /** Most recent data for creating tiles */
  std::shared_ptr<DataType> myTileData;
};
}
