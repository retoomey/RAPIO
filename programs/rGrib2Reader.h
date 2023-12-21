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
  //std::string ConfigModelInfoXML;

  /** read in which fields we want to process for this model */
  virtual void
  whichFieldsToProcess();

  //** get the model projection information and output resolution */
  virtual void
  getModelProjectionInfo(std::string& modeltype);

  /** Process a new record/datatype.  See the .cc for RAPIOData info */
  virtual void
  processNewData(rapio::RAPIOData& d) override;

protected:

private:
  size_t inputx; // # if input model columns
  size_t inputy; // # of input model rows
  size_t outputlons; // # of output columns
  size_t outputlats; // # of output rows
  float nwlon;  // NW corner of output
  float nwlat;  // NW corner of output
  float selon;  // SE corner of output
  float selat;  // SE corner of output
  float latspacing = 0;
  float lonspacing = 0;
  float zspacing = 0;
  std::string proj; 

};
