#pragma once

#include <rRAPIOAlgorithm.h>
#include <rRadialSet.h>
#include <rElevationVolume.h>
#include <rTerrainBlockage.h>

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
 * @author Robert Toomey
 **/
class PolarAlgorithm : public RAPIOAlgorithm {
public:

  /** Create a PolarAlgorithm */
  PolarAlgorithm(const std::string& display = "Algorithm") : RAPIOAlgorithm(display){ };

  // FIXME: Humm should we do this stuff higher up so algs can do this
  // if wanted?
  /** Process a new record/datatype.  See the .cc for RAPIOData info */
  virtual void
  processNewData(rapio::RAPIOData& d) override;

  /** Initialization done on first incoming data */
  virtual void
  firstDataSetup(std::shared_ptr<RadialSet> r,
    const std::string& radarName, const std::string& typeName);

  /** Process a new incoming RadialSet */
  virtual bool
  processRadialSet(std::shared_ptr<RadialSet> r);

  /** Process the virtual volume. */
  virtual void
  processVolume(const Time& outTime, float useElevDegs, const std::string& useSubtype);

protected:

  /** Declare all default plugins for this class layer,
   * typically you don't need to change at this level.
   * @see declarePlugins */
  virtual void
  initializePlugins() override;

  /** Declare all default options for this class layer,
   * typically you don't need to change at this level.
   * @see declareOptions */
  virtual void
  initializeOptions(RAPIOOptions& o) override;

  /** Finalize options by processing, etc.,
   * typically you don't need to change at this level.
   * @see processOptions */
  virtual void
  finalizeOptions(RAPIOOptions& o) override;

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
