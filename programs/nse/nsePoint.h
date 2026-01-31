#pragma once

namespace rapio
{
class nsePoint
{
public:
  nsePoint();

  nsePoint(double temp, double rh, double ht, double pres);
  nsePoint(double temp, double rh, double pres);
  nsePoint(double temp, double dewp, double ht, double pres,
    double uwind, double vwind);
  nsePoint(double temp, double dewp, double ht, double pres,
    double uwind, double vwind, double vvel);
  virtual
  ~nsePoint();

  double
  getTempC() const { return _TempC; };
  double
  getDewPointC() const { return _DewPointC; };
  double
  getUWind() const { return _UWind; };
  double
  getVWind() const { return _VWind; }

  double
  getPressure() const { return _Pres; }

  double
  getHeight() const { return _Height; }

  double
  getSaturationVaporPressure() const { return _SaturationVaporPressure; }

  double
  getVaporPressure() const { return _VaporPressure; }

  double
  getLCLTemp() const { return _LCLTemp; }

  double
  getLCLPres() const { return _LCLPres; }

  double
  getTempK() const { return _TempK; }

  double
  getTheta() const { return _Theta; }

  double
  getThetaE() const { return _ThetaE; }

  double
  getVirtualTempK() const { return _VirtualTempK; }

  double
  getVirtualTempC() const { return (_VirtualTempK - 273.15); }

  double
  getMixingRatio() const { return _MixingRatio; }

  double
  getVerticalVelocity() const { return _VerticalVelocity; }

  double
  getDewPointDepression();
  double
  getWetBulbTempK() const { return _WetBulbTempK; }

  double
  getWetBulbTempC() const { return (_WetBulbTempK - 273.16); }

  // vapor pressure (mb)
  static double
  calcVaporPressure(double MixingRatio, double Pressure);
  // temp (C) from vapor pressure (mb)
  static double
  calcTemperatureFromVaporPressure(double VaporPressure);
  // temp (C) from potential temperature (K) and pressure (mb)
  static double
  calcTemperatureFromTheta(double theta, double pres);
  // dew point (C) from temp (C), relative humidity (%)
  static double
  calcDewpointFromRH(double temp, double rh);

  // calculate the length of a wind vector
  static double
  speedFromUV(double u, double v);

  // Meteorological wind speed / dir (wind is blowing from this direction, 0 = north)
  static void
  SpeedDirFromUV(float& u, float& v, float& speed, float& dir);

  // calculate the density
  static double
  calcDensity(double pres, double tempK);

  // get RH
  double
  getRH() const { return _RH; }

  // get RHice
  double
  getRHice() const { return _RHice; }

private:

  /*
   * used to initialize all the following private member variables.
   */
  void
  init();

  /*
   * Temperature (C)
   */
  double _TempC;

  /*
   * Temperature (K)
   */
  double _TempK;

  /*
   * Relative Humidity (%)
   */
  double _RH;

  /*
   * Relative Humidity over ice (%)
   */
  double _RHice;

  /*
   * Height (m MSL)
   */
  double _Height;

  /*
   * Pressure (mb)
   */
  double _Pres;

  /*
   * U component of wind (from west to east; m/s)
   */
  double _UWind;

  /*
   * U component of wind (from west to east; m/s)
   */
  double _VWind;

  /*
   * Dew Point (C)
   */
  double _DewPointC;

  /*
   * SaturationVaporPressure (mb)
   */
  double _SaturationVaporPressure;

  /*
   * SaturationVaporPressureOverIce (mb)
   */
  double _SaturationVaporPressureOverIce;

  /*
   * Vapor Pressure (mb)
   */
  double _VaporPressure;

  /*
   * Temperature of the Lifting Condensation Level (LCL; C)
   */
  double _LCLTemp;

  /*
   * Pressure of the Lifting Condensation Level (LCL; mb)
   */
  double _LCLPres;

  /*
   * Potential Temperature (K)
   */
  double _Theta;

  /*
   * Equivalent Potential Temperature (K)
   */
  double _ThetaE;

  /*
   * Mixing Ratio (g/kg)
   */
  double _MixingRatio;

  /*
   * Virtual Temperature (K)
   */
  double _VirtualTempK;

  /*
   * vertical velocity
   */
  double _VerticalVelocity;

  /*
   * wet bulb temperature
   */
  double _WetBulbTempK;

  /*
   * compute all the variables for this point, given that we
   * already have set TempC, DewPointC, UWind, VWind, Pres, and Height.
   * This will save lots of CPU cycles when we go to calculate
   * vertically integrated quantities
   */
  void
  calculatePointValues();

  // saturation vapor pressure (mb)
  double
  calcSaturationVaporPressure();
  // saturation vapor pressure over ice (mb)
  double
  calcSaturationVaporPressureOverIce();
  // vapor pressure (mb)
  double
  calcVaporPressure();
  // vapor pressure (mb)
  double
  calcVaporPressure(double t);
  // dew point (C)
  double
  calcDewPointFromRH();
  // temperature of lifting condensation level
  double
  calcLCLTemp();
  // pressure at the LCL
  double
  calcLCLPres();
  // Potential Temperature
  double
  calcPotentialTemperature();
  // Mixing Ratio
  double
  calcMixingRatio();
  // Virtual Temperature
  double
  calcVirtualTemperature();
  // Equivalent Potential Temperature
  double
  calcEquivalentPotentialTemperature();
  // calculate the wet bulb temperature
  double
  calcWetBulb();
}
;
}
