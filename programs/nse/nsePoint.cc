#include <RAPIO.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include "nsePoint.h"

namespace rapio {

/** 
 * nsePoint is an individual grid point of environmental data. It can be
 * inititialize a number of ways as long as we have temp, moisture, height,
 * pressure, and u/w wind information from some source (usual NWP).
 *
 * @author Travis Smith
 */
void nsePoint::init()
{
  _TempC = Constants::MissingData;
  _TempK = Constants::MissingData;
  _RH = Constants::MissingData;
  _RHice = Constants::MissingData;
  _Height = Constants::MissingData;
  _Pres = Constants::MissingData;
  _UWind = Constants::MissingData;
  _VWind = Constants::MissingData;
  _DewPointC = Constants::MissingData;
  _SaturationVaporPressure = Constants::MissingData;
  _SaturationVaporPressureOverIce = Constants::MissingData;
  _VaporPressure = Constants::MissingData;
  _LCLTemp = Constants::MissingData;
  _LCLPres = Constants::MissingData;
  _Theta = Constants::MissingData;
  _ThetaE = Constants::MissingData;
  _MixingRatio = Constants::MissingData;
  _VirtualTempK = Constants::MissingData;
  _VerticalVelocity = Constants::MissingData;
  _WetBulbTempK = Constants::MissingData;

};

nsePoint::nsePoint()
{
  init();

}

nsePoint::~nsePoint()
{
}

nsePoint::nsePoint(double temp, double rh, double ht, double pres)
{
  init();

  _TempC = temp;
  _RH = rh * 0.01;
  _Pres = pres;
  _Height = ht;
  _DewPointC = calcDewPointFromRH();

  calculatePointValues();
}

nsePoint::nsePoint(double temp, double rh, double pres)
{
  init();

  _TempC = temp;
  _RH = rh * 0.01;
  _Pres = pres;
  _DewPointC = calcDewPointFromRH();

  calculatePointValues();
}


nsePoint::nsePoint(double temp, double dewp, double ht, double pres,
            double uwind, double vwind)
{
  init();

  _TempC = temp;
  _DewPointC = dewp;
  _Height = ht;
  _Pres = pres;
  _UWind = uwind;
  _VWind = vwind;

  //FIXME:  RH from dew point????
  calculatePointValues();

}

nsePoint::nsePoint(double temp, double dewp, double ht, double pres,
            double uwind, double vwind, double vvel)
{
  init();

  _TempC = temp;
  _DewPointC = dewp;
  _Height = ht;
  _Pres = pres;
  _UWind = uwind;
  _VWind = vwind;
  _VerticalVelocity = vvel;

  //FIXME:  RH from dew point????
  calculatePointValues();

}


void nsePoint::calculatePointValues()
{
  // the order of things in this routine may be important, so be
  // careful when changing things around (some parameters are 
  // dependent on others
  //
  if (_TempC != Constants::MissingData) {
    _TempK = _TempC + 273.15;
  } else {
    _TempK = Constants::MissingData;
  }
  _SaturationVaporPressure = calcSaturationVaporPressure();
  _SaturationVaporPressureOverIce = calcSaturationVaporPressureOverIce();
  _VaporPressure = calcVaporPressure( _DewPointC );

  if (_RH == Constants::MissingData) {
    if (_VaporPressure != Constants::MissingData &&
	_SaturationVaporPressure != Constants::MissingData) {
      _RH = _VaporPressure / _SaturationVaporPressure;
    } else {
      _RH = Constants::MissingData;
    }
  }
  if (_VaporPressure != Constants::MissingData &&
	_SaturationVaporPressureOverIce != Constants::MissingData) {
      _RHice = _VaporPressure / _SaturationVaporPressureOverIce;
  } else {
      _RHice = Constants::MissingData;
  }
  _LCLTemp = calcLCLTemp();
  _LCLPres = calcLCLPres();
  _Theta = calcPotentialTemperature();
  _MixingRatio = calcMixingRatio();
  _VirtualTempK = calcVirtualTemperature();
  _ThetaE = calcEquivalentPotentialTemperature();
  _WetBulbTempK = calcWetBulb();
//  if (_ThetaE > 350 && _Pres > 800)
//    std::cout << "p = " << _Pres << " t = " << _TempC << " d = " << _DewPointC << " thte = " << _ThetaE << "\n";
  //
  // check for realistic values of data:
  if ((_TempC < -273.15 || _TempC > 100) && _TempC != Constants::MissingData) {
    fLogInfo (" TempC is out of range: {}", _TempC);
  }
  if ((_DewPointC < -273.15 || _DewPointC > 100) && _DewPointC != Constants::MissingData) {
    fLogInfo (" DewPointC is out of range: {}", _DewPointC);
  }
  if ((_UWind < -100 || _UWind > 100) && _UWind != Constants::MissingData) {
    fLogInfo (" UWind is out of range: {}", _UWind);
  }
  if ((_VWind < -100 || _VWind > 100) && _VWind != Constants::MissingData) {
    fLogInfo (" VWind is out of range: {}", _VWind);
  }
  if ((_Height < -1000 || _Height > 100000) && _Height != Constants::MissingData) {
    fLogInfo (" Height is out of range: {}", _Height);
  }
}

double nsePoint::getDewPointDepression()
{ 
  if (_TempC != Constants::MissingData &&
      _DewPointC != Constants::MissingData ) {
    return (_TempC - _DewPointC);
  }
  else return Constants::MissingData;
}

double nsePoint::calcSaturationVaporPressure()
{
  double ret_val = Constants::MissingData;
  if (_SaturationVaporPressure != Constants::MissingData)
  {
    ret_val = _SaturationVaporPressure;
  }
  else
  {
    // from Bolton (MWR 1980):
    if (_TempC != Constants::MissingData)
    {
      ret_val = 6.112 * exp((17.67 * _TempC)/(_TempC + 243.5));
    }
  }
  return ret_val;
}

double nsePoint::calcSaturationVaporPressureOverIce()
{
  double ret_val = Constants::MissingData;
  if (_SaturationVaporPressureOverIce != Constants::MissingData)
  {
    ret_val = _SaturationVaporPressureOverIce;
  }
  else
  {
    // from Murray, F.W. 1966. ``On the computation of Saturation Vapor Pressure''  J. Appl. Meteor.  6 p.204
    if (_TempC != Constants::MissingData)
    {
      ret_val = 6.1078 * exp((21.8745584 *( _TempK-273.16))/(_TempK - 7.66));
    }
  }
  return ret_val;
}


double nsePoint::calcVaporPressure( double t )
{
  double ret_val = Constants::MissingData;

  // from Bolton (MWR 1980):
  if (t != Constants::MissingData) {
    ret_val = 6.112 * exp((17.67 * t)/(t + 243.5));
  }
  assert(isfinite(ret_val));
  return ret_val;
}
double nsePoint::calcTemperatureFromVaporPressure(double VaporPressure)
{
  // VaporPressure in mb, returns Temp in degree C
  if (VaporPressure == Constants::MissingData) return Constants::MissingData;

  double t = (243.5*log(VaporPressure) - 440.8)/(19.48 - log(VaporPressure));

  assert(isfinite(t));
  return t;
}

double nsePoint::calcTemperatureFromTheta(double theta, double pres)
{
  // use Poisson's equation (see Hess), return Temperature in degree C
  if (theta == Constants::MissingData || 
      pres == Constants::MissingData)
    return Constants::MissingData;
  double t = theta * pow (pres/1000., 0.286) - 273.15;
  return t;
}

double nsePoint::calcDewpointFromRH(double temp, double rh)
{
  double dewp = Constants::MissingData;

  //if rh == 0, let's assume that's a missing value for it
  if ( (temp != Constants::MissingData) &&
       (rh > 0.) )
  {
    double RH = rh * 0.01;
    // from Bolton (MWR 1980):
    double es = 6.112 * exp((17.67 * temp)/(temp + 243.5));
    double e = es * RH;
    dewp = (243.5*log(e) - 440.8) / (19.48 - log(e));
  }

  assert(isfinite(dewp));
  return dewp;

}

double nsePoint::calcVaporPressure( double MixingRatio, double Pressure)
{
  // MixingRatio in g/kg, Pressure in mb (see Hess, p. 59)
  if (MixingRatio == Constants::MissingData ||
      Pressure == Constants::MissingData)
    return Constants::MissingData;

  // are these in a realistic range?
  if (Pressure <= 0 || Pressure > 2000 || MixingRatio <= 0 || MixingRatio >= 50)
  {
    return Constants::MissingData;
  } 
  double ret_val = ( ((MixingRatio/1000)*Pressure)/(0.622+(MixingRatio/1000)) );
  assert(isfinite(ret_val));
  return ret_val;
}
double nsePoint::calcVaporPressure()
{
  double ret_val = Constants::MissingData;

  if (_RH != Constants::MissingData)
  {
    // because saturation pressure may not be calc'd yet.
    double es = calcSaturationVaporPressure();
    if (es != Constants::MissingData)
    {
      // from Bolton (MWR 1980):
      ret_val = es * _RH;
    }
  }
  return ret_val;

}

double nsePoint::calcDewPointFromRH()
{
  // from Bolton (MWR 1980):
//JWBMOO - This returns different number than _VaporPressure for some reason...
  double e = calcVaporPressure();
  if (e == Constants::MissingData) { return (Constants::MissingData); }

  //double dewp = 243.5 * (log(6.112) - log(e) / ( log (e) - log (6.112) - 17.67));
  //double b = log(e/0.6112)/17.27;
  //double dewp = 243.5*(b/(1-b));
  double dewp = (243.5*log(e) - 440.8) / (19.48 - log(e));

  if (isfinite(dewp))
    return dewp;
  else
    return (Constants::MissingData);
}

// FIXME: not tested, yet:


double nsePoint::calcLCLTemp()
{
  double ret_val = Constants::MissingData;

  if (_TempC != Constants::MissingData &&
      _DewPointC != Constants::MissingData)
  {
  //  From Barnes (JAM 1968, p511)
    ret_val = _DewPointC - (0.001296 * _DewPointC + 0.1963) 
              * (_TempC - _DewPointC);
  }

  return ret_val;
}
double nsePoint::calcLCLPres()
{
 double ret_val = Constants::MissingData;
  if (_LCLPres != Constants::MissingData)
  {
    ret_val = _LCLPres;
  }
  else
  {
    // no ref?  Use GEMPAK's formula based on Poisson
    // PLCL = PRES * ( TLCL / TMPK ) ** ( 1 / RKAPPA )
    if (_LCLTemp != Constants::MissingData &&
        _Pres != Constants::MissingData &&
        _TempK != Constants::MissingData)
    {
      ret_val = _Pres * pow(((_LCLTemp+273.15) / _TempK) , (1 / 0.286) );
    }
  }

  return ret_val;
}
double nsePoint::calcPotentialTemperature()
{
 double ret_val = Constants::MissingData;
  if (_Theta != Constants::MissingData)
  {
    ret_val = _Theta;
  }
  else
  {
    // aka "theta" -- use Poisson's equation (Wallace and Hobbs)
    if (_Pres != Constants::MissingData &&
        _TempK != Constants::MissingData)
    {
      ret_val = _TempK * pow((1000.0 / _Pres) , 0.286);
    }
  }

  return ret_val;
}
double nsePoint::calcMixingRatio()
{
  double ret_val = Constants::MissingData;

  if (_MixingRatio != Constants::MissingData)
  {
    ret_val = _MixingRatio;
  }
  else
  {
    if (_VaporPressure != Constants::MissingData &&
        _Pres != Constants::MissingData)
    {
      // Hess (1959)
      double epsillon = .62197;
      ret_val = (epsillon * _VaporPressure) /
                (_Pres - _VaporPressure);
      // convert to g/kg
      ret_val *= 1000.0;
    }
  }

  return ret_val;
}
double nsePoint::calcVirtualTemperature()
{
  double ret_val = Constants::MissingData;

  if (_VirtualTempK != Constants::MissingData)
  {
    ret_val = _VirtualTempK;
  }
  else
  {
    if (_TempK != Constants::MissingData &&
        _MixingRatio != Constants::MissingData)
    {
      // because moist air is less dense than dry air (Newton 1717)
      // eq is originally Hess (1959)
      const double Wtemp = _MixingRatio < 0.1
        ? 0
        : _MixingRatio / 1000.0; // convert to kg/kg (dimensionless)

      ret_val = _TempK * (1.0 + 1.609 * Wtemp) / (1.0 + Wtemp);
    }
  }

  return ret_val;
}
double nsePoint::calcEquivalentPotentialTemperature()
{
  // from Bolton (1980), equation #43
  if (_MixingRatio == Constants::MissingData ||
      _TempK == Constants::MissingData || 
      _LCLTemp == Constants::MissingData) {
    return (Constants::MissingData);
  }

  double Wtemp = 0;
  if (_MixingRatio < 0.1) Wtemp = 0;
  else Wtemp = _MixingRatio;	
  double exponent = 0.2854*(1.0 - 0.00028 * Wtemp);
  double term1 = _TempK * pow((1000/_Pres), exponent);
  double term2 = (3.376/(273.15+_LCLTemp)) - 0.00254;
  double term3 = Wtemp * (1 + 0.00081 * Wtemp);
  double thetae = term1 * exp (term2 * term3);
  if (isfinite(thetae))
    return thetae;
  else
    return Constants::MissingData;
}

double nsePoint::speedFromUV(double u, double v)
{
  double speed;
  if (u == Constants::MissingData || v == Constants::MissingData) {
    speed = Constants::MissingData;
  } else {
    speed = pow(u*u+v*v,0.5);
  }
  assert(isfinite(speed));
  return speed;
}
// Meteorological wind speed / dir (wind is blowing from this direction, 0 = north)
void nsePoint::SpeedDirFromUV (float& u, float& v, float& speed, float& dir)
{

  if (u == Constants::MissingData || v == Constants::MissingData) {
    speed = Constants::MissingData;
    dir =  Constants::MissingData;
  } else {
  // speed:
    speed = pow((double) u* (double) u + (double) v* (double) v,0.5);
  //direction:
    dir = (atan2(u,v) * 180/3.14159) - 180;
    if (dir < 0) dir += 360;
  }
  return;
}

double nsePoint::calcDensity(double pres, double tempK) 
{
  const double Rgas = 287.04;

  double ret_val = (100 * pres / ( Rgas * tempK ));
  if (isfinite(ret_val))
    return ret_val;
  else
    return (Constants::MissingData);
}

double nsePoint::calcWetBulb()
{
  //uses the AWIPS method for Tw calculation at a point
  double ed = calcVaporPressure();
  double es = calcSaturationVaporPressure();
  double DewPointC = calcDewPointFromRH();
  double DewPointK = (DewPointC + 273.16);
  double ew, der;
  double de = .0001;
  int i = 0;

  double s = ((es-ed)/(_TempK-DewPointK));
  double tw = (((.0006355*_TempK*_Pres)+(DewPointK*s))/((.0006355*_Pres)+s));

  if (!isfinite(tw))
    return Constants::MissingData;

  while (de>=.0001 && i<=10) {
    ew = exp(26.66082 - (.0091370924*tw) - (6106.396/tw));
    de = ((.0006355*_Pres)*(_TempK-tw))-(ew-ed);
    der = (ew*(.0091379024-(6106.396/(tw*tw)))-(.0006355*_Pres));
    ++i; 
    tw = (tw-(de/der));
    if (!isfinite(tw))
      return Constants::MissingData;
    }
  
  if (isfinite(tw))
    return tw;
  else
    return Constants::MissingData;
}

} // end namespace rapio
