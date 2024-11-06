#pragma once

/** RAPIO API */
#include <RAPIO.h>

namespace rapio {
/*
 *  A polar virtual volume algorithm that merges/collapes a virtual volume
 *  into a single polar product. In this case, the polar
 *  subtype is not a tilt in 3D space, but a raw polar coordinate
 *  product.
 *
 *  This has been done in MRMS before, typically the subtypes become
 *  something like "at 0.5" to refer to the what instead of the
 *  3D height.  This output can then be processed say by a one-tilt
 *  fusion algorithm to create a 2D merged product.
 *
 *  For example, we might take a velocity 'max' of a virtual volume
 *  of polar tilts, creating an "at 0.5" max for a current incoming
 *  .5 tilt.  This max would represent the max of the entire volume
 *  and thus no longer have a meaning in height.
 *
 *  This algorithm assumes all tilts have the same number of azimuth
 *  and gates, which simplifies iteration.
 *
 * I'm debating having a stage1 fusion that can just do a preprocessing
 * volume merge algorithm.  So this will also follow the dynamic
 * pluging algorithm model.
 *
 * @author Robert Toomey
 **/
class PolarMerge : public rapio::RAPIOAlgorithm {
public:

  /** Create an example simple algorithm */
  PolarMerge(){ };

  /** Declare all algorithm command line plugins */
  virtual void
  declarePlugins() override;

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Process a new record/datatype.  See the .cc for RAPIOData info */
  virtual void
  processNewData(rapio::RAPIOData& d) override;

  /** Process a new incoming RadialSet */
  bool
  processRadialSet(std::shared_ptr<RadialSet> r);

  /** Initialization done on first incoming data */
  void
  firstDataSetup(std::shared_ptr<RadialSet> r,
    const std::string& radarName, const std::string& typeName);

  /** Process the virtual volume. */
  void
  processVolume(const Time& outTime, float useElevDegs, const std::string& useSubtype);

protected:

  /** The upto inclusion amount */
  float myUptoDegs;

  /** Store first radar name.  Currently we can only handle 1 radar */
  std::string myRadarName;

  /** Store first type name.  Currently we can only handle 1 typename */
  std::string myTypeName;

  /** Store elevation volume handler */
  std::shared_ptr<Volume> myElevationVolume;

  /** Store terrain blockage */
  std::shared_ptr<TerrainBlockage> myTerrainBlockage;

  /** The current data used for merging */
  std::shared_ptr<RadialSet> myMergedSet;
};
}
