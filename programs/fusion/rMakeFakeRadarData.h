#pragma once

/** RAPIO API */
#include <RAPIO.h>
#include "rTerrainBlockage.h"

namespace rapio {
/*
 * Port of MRMS make fake radar data tool to generate
 * artificial radial sets for testing merger/fusion code.
 * This version ignores VCP info which is part of the fusion
 * experiment
 *
 * @author Valliappa Lakshman
 * @author Robert Toomey
 **/
class MakeFakeRadarData : public rapio::RAPIOProgram {
public:

  /** Create */
  MakeFakeRadarData(){ };

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Execute */
  virtual void
  execute() override;

  /** Make the DataGrid for a python chart (first pass alpha) */
  void
  terrainAngleChart(RadialSet& rs);

protected:

  /** Add fake radials to a RadialSet */
  void
  addRadials(RadialSet& rs);

  /** Read terrain DEM if available */
  void
  readTerrain();

  /** Radar name we are using */
  std::string myRadarName;

  /** Beamwidth in degrees */
  AngleDegs myBeamWidthDegs;

  /** Gatewidth in meters */
  LengthMs myGateWidthM;

  /** Radar range in kilometers */
  LengthKMs myRadarRangeKM;

  /** Default radar value before terrain application */
  float myRadarValue;

  /** Chart parameter */
  std::string myChart;

  /** Azimuthial spacing in degrees */
  AngleDegs myAzimuthalDegs;

  /** Calculated number of gates per radial set */
  size_t myNumGates;

  /** Calculated number of radials per radial set */
  size_t myNumRadials;

  /** Path to terrain file */
  std::string myTerrainPath;

  /** DEM if we have it */
  std::shared_ptr<LatLonGrid> myDEM;

  /** Store terrain blockage */
  std::shared_ptr<TerrainBlockage> myTerrainBlockage;
};
}
