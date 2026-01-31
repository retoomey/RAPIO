#pragma once

#include <vector>
#include "nsePoint.h"

namespace rapio
{
/**
 * nseProfile is a vertical sounding of environmenal data (nsePoints). It allows
 * for simple calaculation of layered and integrated environmental parameters
 **/
class nseProfile
{
public:

  /*
   * Initialize an nseProfile
   */
  nseProfile();
  virtual
  ~nseProfile();

  /*
   * add a grid point
   */
  void
  addPoint(nsePoint pt);

  /*
   * return the point at a specific pressure level
   */
  nsePoint
  getParcelAtPressureLevel(double pLevel);

  /*
   * add the surface point
   */
  void addSfcPoint(nsePoint pt){ sfcPoint = pt; }

  /*
   * after profile has been finalized, call process to initialize
   * variables
   */
  void
  process();

  /*
   * get the surface point
   */
  nsePoint getSfcPoint(){ return sfcPoint; }

  /*
   * get the height of the 0 C isotherm.  If more than one
   * occurance (due to an inversion), then return the highest
   * one.
   */
  double
  getHeightOfIsotherm(double Isotherm);

  /*
   * get the height of the top-most 0 C isotherm and up to two other 0 C
   * crossing of the temperature profile.  Useful for winter weather.
   * Also calculate the melting and freezing energy for these layers (J/kg)
   */
  void
  getHeightOfMeltingLayers(double& ht_melting,
    double &ht_refreezing, double& ht_remelting,
    double& depth_elev_warm_layer, double& depth_sfc_warm_layer,
    double& depth_sfc_cold_layer, double& energy_melting,
    double& energy_freezing, double& energy_remelting);

  /*
   * get the pressure-weighted (FIXME!) mean value of the parameter
   * between two height levels
   */
  double
  getMeanValueLayer(double botHeight, double topHeight,
    const std::vector<double> &param);

  /*
   * get the mean value of the parameter between two height levels
   */
  double
  getMeanValueLayer(double botHeight, double topHeight,
    const std::vector<double> &param, const std::vector<double> &height);

  /*
   * get the mean value of the parameter between to pressure layers
   */
  double
  getMeanValuePressureLayer(double botPres, double topPres,
    const std::vector<double> &param);

  /*
   * get the height of the minimum value of param below presLevel
   */
  double
  getHeightOfMinValue(double presLevel,
    const std::vector<double> &param, const std::vector<double> &height,
    const std::vector<double> &pres, double& minval);

  /*
   * get the point at index location pt
   */
  nsePoint
  getPoint(size_t pt);

  /*
   * set the surface elevation (m MSL)
   */
  void setSurfaceElevation(double ht){  SurfaceElevation = ht; }

  /*
   * get the surface elevation (m MSL)
   */
  double getSurfaceElevation(){ return SurfaceElevation; }

  /*
   * how many grid points are below the ground?
   */
  size_t getNearSurfaceGridPoint(){ return nearSurfaceGridPoint; }

  /*
   * Johns et al. (Tornado Symposium monograph, 1993)
   */
  static void
  getStormMotionJohns1993(
    double MeanSpeed, double MeanU, double MeanV,
    double& StormSpeed, double& StormU, double& StormV);

  /*
   * Storm-relative helicity (
   */
  double
  getStormRelativeHelicity(
    double StormU, double StormV, double LayerDepth);

  /*
   * How many grid points are beneath the earth's surface?
   */
  size_t getPointsBelowGround(){ return nearSurfaceGridPoint; }

  /*
   * How many grid points total (not including the sfc point)?
   */
  size_t getNumPoints(){ return gridPoints.size(); }

  /*
   * get the most unstable parcel in a layer
   */
  nsePoint
  getMostUnstableParcel(double layerDepth);

  /*
   * get the average parcel in a layer
   */
  nsePoint
  getAverageParcel(double layerDepth);

  /*
   * Calculated lifted parcel parameters
   */
  void
  LiftParcel(nsePoint parcel,
    double& LCLHeight, double& EL, double& LFC,
    double& CAPE, double& CIN, double& LI, double& MPL,
    double& LMB, double& MaxB,
    double& CAPE_LFCToLFCPlus3km, double& CAPE_SfcTo3kmAGL);

  /*
   * DCAPE (J/kg) using a parcel that starts at the given height
   */
  double
  getDCAPE(double parcelHeightAGL);

  /*
   * Moist adiabatic temperature (K) given ThetaE (K) and Pressure (mb)
   */
  static double
  getMoistParcelTemp(double thetae, double pres);

  /*
   * Energy-Helicity Index (Davies 17th SLS Conf. preprints, p. 107)
   */
  static double
  EnergyHelicityIndex(double CAPE, double SREH);

  /*
   * storm-relative flow
   */
  double
  getStormRelativeFlow(double height_bot_agl,
    double height_top_agl, double stormU, double stormV);

  /*
   * Bulk Richardson Shear, as defined by Bluestein vol. 2, eqn. 3.4.58
   */
  double
  getBRNshear();

  /*
   * Bulk Richardson Number
   */
  static double
  getBRN(double CAPE, double BRNShearSquared);

  /*
   * get the temperature difference in a pressure layer
   */
  double
  getTempDifferencePressureLayer(double topmb, double botmb);

  /*
   * get the difference of a parameter from the top of a layer to the bottom
   */
  static double
  getParamDifferencePressureLayer(double topmb, double botmb,
    const std::vector<double> &param, const std::vector<double> &pres);

  /*
   * get the wind speed at the given heightAGL meters above the surface.
   * WindType can equal "U", "V", or "Speed" to return the U component,
   * V component, or total scalar value.
   */
  double
  getWindSpeedAtHeightAGL(double heightAGL, std::string WindType);

  /*
   * return the magnitude of the shear vector from the surface to
   * topHeightAGL meters above the surface
   */
  double
  getShearVectorMagnitude(double topHeightAGL);

  /*
   * return the magnitude of the speed shear from the surface to
   * topHeightAGL meters above the surface
   */
  double
  getSpeedShear(double topHeightAGL);

  /*
   * the "deep layer shear" (0-2 km mean to 9-11 km mean)
   */
  double
  getDeepShearVector();

  /*
   * misc indices that people used to use
   */
  void
  getJunkIndices(double LI, double& VerticalTotals,
    double& CrossTotals, double& TotalTotals, double& KIndex,
    double& SWEAT, double& SSI);

  /*
   * return the maximum equivalent potential temperature below the
   * given pressure level (and the height of the max in meters)
   */
  double
  getMaxThetaEBelowPressureLevel(double topPres, double& heightOfMax);

  /*
   * return the average lapse rate in a height layer
   */
  double
  getLapseRateInHeightLayer(double botHeight, double topHeight);

  /*
   * return the average lapse rate in a pressure layer
   */
  double
  getLapseRate(double botPres, double topPres);

  /*
   * return the average CAPE in a layer from the LCL to EL
   */
  static double
  NormalizedCAPE(double CAPE, double ELHeight, double LCLHeight);

  /*
   * return the value of the first argument divided by the second argument
   */
  static double
  Percentage(double partialCAPE, double CAPE);

  /*
   * Determine the curvature of the hodograph curve for this layer.
   * 0 = straight, >0 = cyclonic, <0 = anticyclonic
   */
  double
  HodographCurvature(double depthAGL);

  /*
   * return the length of the hodograph
   */
  double
  HodographLength(double depthAGL);

  /*
   * return the mean shear over the layer
   */
  double
  MeanShear(double depthAGL);

  /*
   * get the height where the wet-bulb temp = 0 (meters)
   */
  double
  getHeightOfWetBulbZero();

  /*
   * get the height where the wet-bulb temp = 4 (meters)
   */
  double
  getHeightOfWetBulbFour();

  /*
   * the vertically integrated wet-bulb temperature (C??)
   */
  double
  getVerticallyIntegratedWetBulbTemp(double WBHeight);

  /*
   * return the dew point depression at a pressure level (C)
   */
  double
  getDewpointDepressionAtPressureLevel(double pres);

  /*
   * return the maximum dew point depression in a layer (C)
   */
  double
  getMaxDewpointDepressionInPressureLayer(double botPres,
    double                                       topPres);

  /*
   * return the first number minus the second number (with missingData check)
   */
  static double
  Difference(double first, double second);

  /*
   * Wind INDEX
   */
  double
  getWINDEX();

  /*
   * convective temperature (C)
   */
  double
  ConvectiveTemperature(nsePoint parcel);

  /*
   * precipitable water (cm)
   */
  double
  PrecipitableWater();

  /*
   * VGP
   */
  double
  VorticityGenerationPotential(double CAPE, double depth);

  /*
   * vertical velocity at a pressure level
   */
  double
  getVerticalVelocityAtPressureLevel(double pres);

private:

  size_t
  calcPointsBelowGround();

  /*
   * elevation of the terrain
   */
  double SurfaceElevation;

  /*
   * the index of the first grid point above the surface
   */
  size_t nearSurfaceGridPoint;

  /*
   * the vertical profile of grid points
   */
  std::vector<nsePoint> gridPoints;

  /*
   * the surface grid point
   */
  nsePoint sfcPoint;
}
;
}
