#pragma once

#include <rRAPIOAlgorithm.h>
#include <rRadialSet.h>
#include <rRadialSetIterator.h>
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

  /** New way using the callback iterator API.  This will allow us to hide
   * the 'normal' ways of iterating over DataTypes.  In particular,
   * the LatLonGrid is complicated. Typically one marches over lat/lon and
   * has to do angle calculations, etc.  We can hide all that from users. */
  class ElevationVolumeCallback : public RadialSetCallback {
public:

    /** Add an Elevation volume to us */
    void
    addVolume(const Volume& e)
    {
      auto& tilts = e.getVolume();

      for (size_t i = 0; i < tilts.size(); ++i) {
        // Scoped as long as ElevationVolume is
        auto pc = tilts[i]->getDataTypePointerCache();
        pointerCache.push_back(pc.get());
      }
    }

    /** Get reference to pointer cache */
    inline std::vector<DataTypePointerCache *>&
    getPointerCache()
    {
      return pointerCache;
    }

    /** For each gate, we've gonna take the absolute max of the other gates
     * in the volume vertically */
    virtual void
    handleGate(RadialSetIterator * it) = 0;

private:
    /** The pointer cache for our volume items. */
    std::vector<DataTypePointerCache *> pointerCache;
  };

  // ------------------------------------
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

  /** Create output RadialSet for the current volume */
  virtual std::shared_ptr<RadialSet>
  createOutputRadialSet(const Time& useTime, float useElevDegs,
    const std::string& useTypename, const std::string& useSubtype);

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

  /** The top elevation angle inclusion amount */
  float myTopDegs;

  /** The bottom elevation angle inclusion amount */
  float myBottomDegs;

  /** Number of output gates, or -1 if max of current volume */
  int myNumGates;

  /** Number of output azimuths, or -1 if max of current volume */
  int myNumAzimuths;

  // FIXME: No reason for these simple polar algorithms we can't handle
  // N moments/radars.  Just need elevation volume per each.

  /** Store first radar name.  Currently we can only handle 1 radar */
  std::string myRadarName;

  /** Store first type name.  Currently we can only handle 1 typename */
  std::string myTypeName;

  /** Store elevation volume handler */
  std::shared_ptr<Volume> myElevationVolume;

  /** Store terrain blockage */
  std::shared_ptr<TerrainBlockage> myTerrainBlockage;
};
}
