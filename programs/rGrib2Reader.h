#pragma once

/** RAPIO API */
#include <RAPIO.h>

class Grib2ReaderAlg : public rapio::RAPIOAlgorithm {
public:

  /** Create an example simple algorithm */
  Grib2ReaderAlg(){ };

  // The basic API messages from the system

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Path for our configuration file */
  std::string ConfigModelInfoXML;

  /** read in which fields we want to process for this model */
  virtual void
  whichFieldsToProcess();

  /** Process a new record/datatype.  See the .cc for RAPIOData info */
  virtual void
  processNewData(rapio::RAPIOData& d) override;

protected:
};
