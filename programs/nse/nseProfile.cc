#include <RAPIO.h>
#include <cassert>
#include <algorithm>
#include <vector>
// #include <util/code_Angle.h>
#include "nseProfile.h"

using namespace rapio;

double
interpVal(double bot, double top, double botval,
  double topval, double targetval);

nseProfile::nseProfile() :
  SurfaceElevation(Constants::MissingData),
  nearSurfaceGridPoint(0)
{ }

nseProfile::~nseProfile()
{ }

void
nseProfile::process()
{
  calcPointsBelowGround();
}

void
nseProfile::addPoint(nsePoint pt)
{
  gridPoints.push_back(pt);
}

nsePoint
nseProfile::getParcelAtPressureLevel(double pLevel)
{
  nsePoint p;

  // FIXME: this only works if we hit the pressure level exactly
  for (size_t i = 0; i < gridPoints.size(); ++i) {
    if (gridPoints[i].getPressure() == pLevel) {
      p = gridPoints[i];
    }
  }
  return p;
}

double
nseProfile::getHeightOfIsotherm(double isotherm)
{
  // work from the top level, downward
  if (gridPoints.size() < 3) {
    return (Constants::MissingData);
  }
  int numPoints = gridPoints.size();

  for (int i = (numPoints - 2); i >= 0; i--) {
    assert(i < numPoints && "Out of range");
    assert((i + 1) < numPoints && "Out of range");
    if ((gridPoints[i + 1].getTempC() != Constants::MissingData) &&
      (gridPoints[i].getTempC() != Constants::MissingData) &&
      (gridPoints[i + 1].getHeight() != Constants::MissingData) &&
      (gridPoints[i].getHeight() != Constants::MissingData) )
    {
      if ((gridPoints[i + 1].getTempC() < isotherm) &&
        (gridPoints[i].getTempC() >= isotherm) )
      {
        // found it, so interpolate
        double ht0c = interpVal(gridPoints[i].getHeight(),
            gridPoints[i + 1].getHeight(),
            gridPoints[i].getTempC(),
            gridPoints[i + 1].getTempC(),
            isotherm);
        // is it below the ground?
        if (ht0c < SurfaceElevation) { return SurfaceElevation; } else { return ht0c; }
      }
    }
  }
  // return (Constants::MissingData);
  return SurfaceElevation;
}

void
nseProfile::getHeightOfMeltingLayers(double& ht_melting,
  double &ht_refreezing, double& ht_remelting,
  double& depth_elev_warm_layer, double& depth_sfc_warm_layer,
  double& depth_sfc_cold_layer, double& energy_melting,
  double& energy_refreezing, double& energy_remelting)
{
  // this works from the top of the sounding downward, and will identify
  // elevated warm layers (useful for winter precipitation).  These are
  // defined as:
  //
  //   HeightOfMelting:     the topmost 0C isotherm
  //   HeightOfRefreezing:  the isotherm with >0C air above <0C air
  //   HeightOfRemelting:   the next lower 0C isotherm (cold above warm)
  //
  // if any of these do not exist, then they are marked as missing.

  // Hi Travis,
  //
  // You asked for formulas for computing the positive and negative areas
  // relative to 0C for use with the winter HCA I proposed. I found the
  // formula in Bourgouin (2000). It's quite simple, and comes from an
  // earlier work by Iribane and Gibson (1981)
  //
  // C|area| = c * Tbar * (ln(theta_top/theta_bottom))
  //
  // Where Tbar is the mean absolute temperature in the layer between
  // theta_top and Theta_bottom
  // theta_top is the potential teperature at teh top of the layer, and
  // theta_bottom is the potential temperature at the bottom of the layer and
  // c is the specific heat at constant pressure for dry air. .
  //
  // Change Tbar to Twbar, the mean absolute wet bulb temperature, and we're
  // done!
  //
  // Kim
  //
  // Refs:
  //
  // Bourgouin, P., 2000: A method to determine precipitation types. Wea. and
  // Forecasting, 15, 583-592
  //
  // Iribane, J. V., and W. L. Godson, 1981: Atmospheric Thermodynamics. D.
  // Reidel, 259 PP.


  ht_melting            = Constants::MissingData;
  ht_refreezing         = Constants::MissingData;
  ht_remelting          = Constants::MissingData;
  depth_elev_warm_layer = Constants::MissingData;
  depth_sfc_warm_layer  = Constants::MissingData;
  depth_sfc_cold_layer  = Constants::MissingData;
  energy_melting        = Constants::MissingData;
  energy_refreezing     = Constants::MissingData;
  energy_remelting      = Constants::MissingData;

  // work from the top level, downward
  if (gridPoints.size() < 3) {
    return;
  }
  int numPoints = gridPoints.size();
  // these are for energy calculations:
  bool doMelt     = false;
  bool doRefreeze = false;
  bool doRemelt   = false;
  int num_wetbulb_pos_aloft   = 0;
  float sum_wetbulb_pos_aloft = 0;
  int num_wetbulb_neg_aloft   = 0;
  float sum_wetbulb_neg_aloft = 0;
  int num_wetbulb_pos_sfc     = 0;
  float sum_wetbulb_pos_sfc   = 0;
  float theta_melting         = Constants::MissingData;
  float theta_remelting       = Constants::MissingData;
  float theta_refreezing      = Constants::MissingData;

  // loop from top to bottom:
  for (int i = (numPoints - 2); i >= 0; i--) {
    assert(i < numPoints && "Out of range");
    assert((i + 1) < numPoints && "Out of range");
    if ((gridPoints[i + 1].getTempC() != Constants::MissingData) &&
      (gridPoints[i].getTempC() != Constants::MissingData) &&
      (gridPoints[i + 1].getHeight() != Constants::MissingData) &&
      (gridPoints[i].getHeight() != Constants::MissingData) )
    {
      // the case where temperature decreases with height, first one:
      if ((gridPoints[i + 1].getTempC() < 0) &&
        (gridPoints[i].getTempC() >= 0) &&
        (ht_melting == Constants::MissingData) )
      {
        // found it, so interpolate
        ht_melting = interpVal(gridPoints[i].getHeight(),
            gridPoints[i + 1].getHeight(),
            gridPoints[i].getTempC(),
            gridPoints[i + 1].getTempC(),
            0);
        theta_melting = interpVal(gridPoints[i].getTheta(),
            gridPoints[i + 1].getTheta(),
            gridPoints[i].getTempC(),
            gridPoints[i + 1].getTempC(),
            0);
        sum_wetbulb_pos_aloft = interpVal(gridPoints[i].getWetBulbTempC(),
            gridPoints[i + 1].getWetBulbTempC(),
            gridPoints[i].getTempC(),
            gridPoints[i + 1].getTempC(),
            0);
        doMelt = true;
        num_wetbulb_pos_aloft = 1;

        // is it below the ground?
        if (ht_melting < SurfaceElevation) {
          ht_melting = Constants::MissingData;
          return;
        }
        depth_sfc_warm_layer = ht_melting - SurfaceElevation;
      }
      // re-freezing
      if ((gridPoints[i + 1].getTempC() > 0) &&
        (gridPoints[i].getTempC() <= 0) &&
        (ht_melting != Constants::MissingData) )
      {
        // found it, so interpolate
        ht_refreezing = interpVal(gridPoints[i].getHeight(),
            gridPoints[i + 1].getHeight(),
            gridPoints[i].getTempC(),
            gridPoints[i + 1].getTempC(),
            0);
        theta_refreezing = interpVal(gridPoints[i].getTheta(),
            gridPoints[i + 1].getTheta(),
            gridPoints[i].getTempC(),
            gridPoints[i + 1].getTempC(),
            0);
        // summation for positive melting energy above this point:
        float temp_wb = interpVal(gridPoints[i].getWetBulbTempC(),
            gridPoints[i + 1].getWetBulbTempC(),
            gridPoints[i].getTempC(),
            gridPoints[i + 1].getTempC(),
            0);
        sum_wetbulb_pos_aloft += temp_wb;
        doMelt = false;
        num_wetbulb_pos_aloft++;

        // summation for negative melting energy below this point:
        sum_wetbulb_neg_aloft = interpVal(gridPoints[i].getWetBulbTempC(),
            gridPoints[i + 1].getWetBulbTempC(),
            gridPoints[i].getTempC(),
            gridPoints[i + 1].getTempC(),
            0);
        num_wetbulb_neg_aloft = 1;
        doRefreeze = true;

        // is it below the ground?
        if (ht_refreezing < SurfaceElevation) {
          ht_refreezing         = Constants::MissingData;
          depth_elev_warm_layer = Constants::MissingData;
          // energy calc stops at surface:
          sum_wetbulb_pos_aloft = sum_wetbulb_pos_aloft - temp_wb + sfcPoint.getWetBulbTempC();
          energy_melting        = 1005.7 * (sum_wetbulb_pos_aloft / (float) num_wetbulb_pos_aloft)
            * log(theta_melting / sfcPoint.getTheta());
          return;
        }
        depth_elev_warm_layer = ht_melting - ht_refreezing;
        depth_sfc_cold_layer  = ht_refreezing - SurfaceElevation;
        depth_sfc_warm_layer  = Constants::MissingData;
        energy_melting        = 1005.7 * (sum_wetbulb_pos_aloft / (float) num_wetbulb_pos_aloft)
          * log(theta_melting / theta_refreezing);
      }
      // re-melting
      if ((gridPoints[i + 1].getTempC() < 0) &&
        (gridPoints[i].getTempC() >= 0) &&
        (ht_melting != Constants::MissingData) &&
        (ht_refreezing != Constants::MissingData) )
      {
        // found it, so interpolate
        ht_remelting = interpVal(gridPoints[i].getHeight(),
            gridPoints[i + 1].getHeight(),
            gridPoints[i].getTempC(),
            gridPoints[i + 1].getTempC(),
            0);
        theta_remelting = interpVal(gridPoints[i].getTheta(),
            gridPoints[i + 1].getTheta(),
            gridPoints[i].getTempC(),
            gridPoints[i + 1].getTempC(),
            0);
        // summation for negative melting energy above this point:
        float temp_wb = interpVal(gridPoints[i].getWetBulbTempC(),
            gridPoints[i + 1].getWetBulbTempC(),
            gridPoints[i].getTempC(),
            gridPoints[i + 1].getTempC(),
            0);
        sum_wetbulb_neg_aloft += temp_wb;
        doRefreeze = false;
        num_wetbulb_neg_aloft++;

        // summation for positive melting energy below this point
        sum_wetbulb_pos_sfc = interpVal(gridPoints[i].getWetBulbTempC(),
            gridPoints[i + 1].getWetBulbTempC(),
            gridPoints[i].getTempC(),
            gridPoints[i + 1].getTempC(),
            0);
        num_wetbulb_pos_sfc = 1;
        doRemelt = true;

        // is it below the ground?
        if (ht_remelting < SurfaceElevation) {
          ht_remelting = Constants::MissingData;
          // energy calc stops at surface:
          sum_wetbulb_neg_aloft = sum_wetbulb_neg_aloft - temp_wb + sfcPoint.getWetBulbTempC();
          energy_refreezing     = 1005.7 * (sum_wetbulb_neg_aloft / (float) num_wetbulb_neg_aloft)
            * log(theta_refreezing / sfcPoint.getTheta());
          return;
        }
        depth_sfc_warm_layer = ht_remelting - SurfaceElevation;
        depth_sfc_cold_layer = Constants::MissingData;
        energy_refreezing    = 1005.7 * (sum_wetbulb_neg_aloft / (float) num_wetbulb_neg_aloft)
          * log(theta_refreezing / theta_remelting);
      }
      if (doMelt) {
        sum_wetbulb_pos_aloft += gridPoints[i].getWetBulbTempC();
        num_wetbulb_pos_aloft++;
      }
      if (doRefreeze) {
        sum_wetbulb_neg_aloft += gridPoints[i].getWetBulbTempC();
        num_wetbulb_neg_aloft++;
      }
      if (doRemelt) {
        sum_wetbulb_pos_sfc += gridPoints[i].getWetBulbTempC();
        num_wetbulb_pos_sfc++;
      }
    } // validity check
  }   // loop

  if (doMelt) {
    energy_melting = 1005.7 * (sum_wetbulb_pos_aloft / (float) num_wetbulb_pos_aloft)
      * log(theta_melting / sfcPoint.getTheta());
  }
  if (doRefreeze) {
    energy_refreezing = 1005.7 * (sum_wetbulb_neg_aloft / (float) num_wetbulb_neg_aloft)
      * log(theta_refreezing / sfcPoint.getTheta());
  }
  if (doRemelt) {
    energy_remelting = 1005.7 * (sum_wetbulb_pos_sfc / (float) num_wetbulb_pos_sfc)
      * log(theta_remelting / sfcPoint.getTheta());
  }
  return;
} // nseProfile::getHeightOfMeltingLayers

double
nseProfile::getMeanValuePressureLayer(double botPres, double topPres,
  const std::vector<double> &param)
{
  double botHeight = Constants::MissingData;
  double topHeight = Constants::MissingData;

  for (size_t i = 0; i < (gridPoints.size() - 1); i++) {
    if ((gridPoints[i].getPressure() >= botPres) &&
      (gridPoints[i + 1].getPressure() < botPres) )
    {
      botHeight = interpVal(gridPoints[i].getHeight(),
          gridPoints[i + 1].getHeight(), gridPoints[i].getPressure(),
          gridPoints[i + 1].getPressure(), botPres);
    } else if ((gridPoints[i].getPressure() >= topPres) &&
      (gridPoints[i + 1].getPressure() < topPres) )
    {
      topHeight = interpVal(gridPoints[i].getHeight(),
          gridPoints[i + 1].getHeight(), gridPoints[i].getPressure(),
          gridPoints[i + 1].getPressure(), topPres);
    }
  }
  if ((botHeight != Constants::MissingData) &&
    (topHeight != Constants::MissingData) )
  {
    double val = getMeanValueLayer(botHeight, topHeight, param);
    return val;
  } else {
    return Constants::MissingData;
  }
}

double
nseProfile::getMeanValueLayer(double botHeight, double topHeight,
  const std::vector<double> &param)
{
  if (gridPoints.size() != param.size()) {
    fLogInfo("getMeanValueLayer requires that the nseProfile size matches the size of the vector to be interpolated.");
    return (Constants::MissingData);
  }

  bool found_top  = false;
  bool found_bot  = false;
  size_t botIndex = 0;
  size_t topIndex = 0;

  double sum_param = 0;

  // FIXME: made this simple -- as a first cut, we won't bother
  // interpolating to get the endpoints just right.  Unless the
  // model's vertical resolution is really crummy, then this
  // probably doesn't matter, anyway!
  //
  for (size_t i = 0; i < (gridPoints.size() - 1); i++) {
    if ((gridPoints[i].getHeight() >= botHeight) && !found_bot) {
      found_bot = true;
      botIndex  = i;
    }
    if ((gridPoints[i].getHeight() >= topHeight) && !found_top) {
      if (i > 0) {
        topIndex  = i - 1;
        found_top = true;
      } else {
        return (Constants::MissingData);
      }
    }

    // assume that the parameter value represents the value from
    // the bottom level to just below the next level (we aren't
    // interpolating to get the exact values at the endpoints)

    if (found_bot && !found_top) {
      sum_param += (param[i]
        * (gridPoints[i + 1].getHeight() - gridPoints[i].getHeight()));
    }
  }
  if (found_bot && found_top) {
    return (sum_param / (topHeight - botHeight));
  } else {
    return (Constants::MissingData);
  }
} // nseProfile::getMeanValueLayer

double
nseProfile::getMeanValueLayer(double botHeight, double topHeight,
  const std::vector<double> &param, const std::vector<double> &height)
{
  // heights in m MSL
  if (height.size() != param.size()) {
    fLogInfo("getMeanValueLayer requires that the height vector size matches the size of the vector to be interpolated.");
    return (Constants::MissingData);
  }
  if ((botHeight == Constants::MissingData) ||
    (topHeight == Constants::MissingData) ||
    !height.size() || !param.size() )
  {
    return Constants::MissingData;
  }

  bool found_top = false;
  bool found_bot = false;

  double sum_param = 0;


  for (size_t i = 0; i < (height.size() - 1); i++) {
    if ((height[i] == Constants::MissingData) ||
      (height[i + 1] == Constants::MissingData) ||
      (((param[i] == Constants::MissingData) ||
      (param[i + 1] == Constants::MissingData) ) &&
      ((height[i] <= topHeight) && (height[i + 1] >= botHeight) )))
    {
      return Constants::MissingData;
    }

    if ((height[i] <= botHeight) && (height[i + 1] > botHeight)) {
      // this is the starting layer
      found_bot = true;
      double paramValAtBot = interpVal(param[i], param[i + 1],
          height[i], height[i + 1], botHeight);
      if (paramValAtBot == Constants::MissingData) {
        return Constants::MissingData;
      }
      double p = (paramValAtBot + param[i]) / 2;
      sum_param += (p * (height[i + 1] - botHeight) );
    } else if ((height[i] >= botHeight) && (height[i + 1] < topHeight)) {
      // all the middle grid points
      sum_param += (((param[i] + param[i + 1]) / 2) * (height[i + 1] - height[i]));
    } else if ((height[i] < topHeight) && (height[i + 1] >= topHeight)) {
      // the top layer
      found_top = true;
      double paramValAtTop = interpVal(param[i], param[i + 1],
          height[i], height[i + 1], topHeight);
      if (paramValAtTop == Constants::MissingData) {
        return Constants::MissingData;
      }
      double p = (paramValAtTop + param[i]) / 2;
      sum_param += (p * (topHeight - height[i]) );
    }
  }

  if (found_bot && found_top && (topHeight != botHeight)) {
    return (sum_param / (topHeight - botHeight));
  } else {
    return (Constants::MissingData);
  }
} // nseProfile::getMeanValueLayer

nsePoint
nseProfile::getPoint(size_t pt)
{
  if (pt >= gridPoints.size()) {
    nsePoint tmp;
    return (tmp);
  } else {
    return gridPoints[pt];
  }
}

size_t
nseProfile::calcPointsBelowGround()
{
  if (sfcPoint.getHeight() == Constants::MissingData) {
    return 0;
  } else {
    for (size_t i = 0; i < gridPoints.size(); ++i) {
      if (gridPoints[i].getHeight() > sfcPoint.getHeight()) {
        nearSurfaceGridPoint = i;
        return i;
      }
    }
  }
  return (gridPoints.size());
}

double
nseProfile::getStormRelativeHelicity(
  double StormU,
  double StormV,
  double LayerDepth)
{
  // From Davies-Jones, Burgess, and Foster (Preprints, 16th Conf. on
  // Severe Local Storms, Oct. 22-26 1990, p. 588-592).

  // StormU and StormV (m/s)
  // LayerDepth (m)

  if ((StormU == Constants::MissingData) || (StormV == Constants::MissingData) ||
    (LayerDepth == Constants::MissingData) )
  {
    return Constants::MissingData;
  }

  double sum = 0;
  double contrib;

  // From the surface to the first gridpoint above the ground:

  contrib = ((gridPoints[nearSurfaceGridPoint].getUWind() - StormU)
    * (sfcPoint.getVWind() - StormV))
    - ((sfcPoint.getUWind() - StormU)
    * (gridPoints[nearSurfaceGridPoint].getVWind() - StormV));

  sum += contrib;

  // find the grid point below the LayerDepth

  size_t nearTopPoint = 0;

  for (size_t i = nearSurfaceGridPoint; i < gridPoints.size(); ++i) {
    if (gridPoints[i].getHeight() < (LayerDepth + sfcPoint.getHeight())) {
      nearTopPoint = i;
    }
  }

  // calculations for all the middle grid points:

  for (size_t i = nearSurfaceGridPoint; i < nearTopPoint; ++i) {
    contrib = ((gridPoints[i + 1].getUWind() - StormU)
      * (gridPoints[i].getVWind() - StormV))
      - ((gridPoints[i].getUWind() - StormU)
      * (gridPoints[i + 1].getVWind() - StormV));
    sum += contrib;
  }

  // calculations from the top grid point to an interpolated point that
  // is (LayerDepth) height above the surface:

  if (nearTopPoint < (gridPoints.size() - 1)) {
    double LayerTop = LayerDepth + sfcPoint.getHeight();
    double Utop     = interpVal(gridPoints[nearTopPoint].getUWind(),
        gridPoints[nearTopPoint + 1].getUWind(),
        gridPoints[nearTopPoint].getHeight(),
        gridPoints[nearTopPoint + 1].getHeight(),
        LayerTop);
    double Vtop = interpVal(gridPoints[nearTopPoint].getVWind(),
        gridPoints[nearTopPoint + 1].getVWind(),
        gridPoints[nearTopPoint].getHeight(),
        gridPoints[nearTopPoint + 1].getHeight(),
        LayerTop);
    contrib = ((Utop - StormU)
      * (gridPoints[nearTopPoint].getVWind() - StormV))
      - ((gridPoints[nearTopPoint].getUWind() - StormU)
      * (Vtop - StormV));
    sum += contrib;
  }

  return sum;
} // nseProfile::getStormRelativeHelicity

void
nseProfile::LiftParcel(nsePoint parcel,
  double& LCLHeight, double& EL, double& LFC,
  double& CAPE, double& CIN, double& LI,
  double& MPL, double& LMB, double& MaxB,
  double& CAPE_LFCToLFCPlus3km, double& CAPE_SfcTo3kmAGL)
{
  // several parameters are calculated as a byproduct of lifting a
  // parcel:
  //   1) CAPE (convective available potential energy)
  //   2) CIN (convective inhibition
  //   3) LFC (level of free convection)
  //   4) EL (equilibrium level)
  //   5) MPL (maximum parcel level -- where CAPE = negative area
  //      above the EL
  //   6) 500mb Lifted Index
  //   7) Level of Maximum Bouyancy -- where (Tparcel - Tenv)/Tenv is
  //      maximized

  // initialize all the stuff that is suppose to be calculated here
  LCLHeight = Constants::MissingData;
  EL        = Constants::MissingData;
  LFC       = Constants::MissingData;
  CAPE      = 0;
  CIN       = 0;
  LI        = Constants::MissingData;
  MPL       = Constants::MissingData;
  LMB       = 0;
  MaxB      = 0;
  CAPE_LFCToLFCPlus3km = 0;
  CAPE_SfcTo3kmAGL     = 0;

  double MPL_energy     = 0;
  double MPL_energy_old = 0;

  // if we are missing info, we can't do the calculation
  if ((parcel.getLCLTemp() == Constants::MissingData) ||
    (parcel.getLCLPres() == Constants::MissingData) )
  {
    // fLogInfo ("Missing parcel info -- LCLTemp = {} LCLPres = {}", parcel.getLCLTemp(), parcel.getLCLPres());
    CAPE = Constants::MissingData;
    CIN  = Constants::MissingData;
    return;
  }

  // Define the dry adiabatic lapse rate (degC/m):
  const double DALR = 0.0098;

  // get the LCL height for this profile
  // FIXME: needs to use pressure-weighted (log10) interpolation
  // rather than linear.
  for (size_t i = 0; i < (gridPoints.size() - 1); ++i) {
    if ((gridPoints[i].getPressure() > parcel.getLCLPres()) &&
      (gridPoints[i + 1].getPressure() < parcel.getLCLPres()) )
    {
      LCLHeight = interpVal(gridPoints[i].getHeight(),
          gridPoints[i + 1].getHeight(), gridPoints[i].getPressure(),
          gridPoints[i + 1].getPressure(), parcel.getLCLPres());
    }
  }
  if (LCLHeight == Constants::MissingData) {
    // in case we are above the surface grid point
    if ((sfcPoint.getPressure() >= parcel.getLCLPres()) &&
      (gridPoints[nearSurfaceGridPoint].getPressure() < parcel.getLCLPres()))
    {
      LCLHeight = interpVal(sfcPoint.getHeight(),
          gridPoints[nearSurfaceGridPoint].getHeight(), sfcPoint.getPressure(),
          gridPoints[nearSurfaceGridPoint].getPressure(), parcel.getLCLPres());
    }
    if (LCLHeight == Constants::MissingData) {
      CAPE = 0;
      CIN  = 0;
      return;
    }
  }

  // Determine the virtual temperature at all levels for the lifted
  // parcel

  //  std::cout << "Lifting parcel T/Td/thetaE = " << parcel.getTempC()
  //    << " / " << parcel.getDewPointC() << " / " << parcel.getThetaE() << "\n";
  std::vector<double> TVparcel;

  for (size_t i = 0; i < gridPoints.size(); ++i) {
    // below the LCL, this is a dry adiabatic process
    if (gridPoints[i].getHeight() < LCLHeight) {
      double TVparcel_temp = parcel.getVirtualTempK()
        - DALR * (gridPoints[i].getHeight() - parcel.getHeight());
      TVparcel.push_back(TVparcel_temp);
      //      std::cout << "Dry Tv parcel/env = " << TVparcel_temp << " / " <<
      //	gridPoints[i].getVirtualTempK() << "\n";
    } else {
      // above the LCL, it is moist adiabatic
      double TVparcel_temp = getMoistParcelTemp(parcel.getThetaE(),
          gridPoints[i].getPressure() );
      TVparcel.push_back(TVparcel_temp);
      //      std::cout << "Wet Tv parcel/env = " << TVparcel_temp << " / " <<
      //	gridPoints[i].getVirtualTempK() << "\n";
    }
    // while we are at it.... if this grid point is the 500 mb level,
    // then calculate the lifted index.

    // FIXME: this only works if we hit 500 mb exactly, which is fine
    // for the RUC presssure-level grids (or any pressure-level grids
    // for that matter), but won't work for data that is irregularly
    // spaced in the vertical.

    if (gridPoints[i].getPressure() == 500) {
      LI = gridPoints[i].getTempK() - TVparcel[i];
    }
  }
  assert(TVparcel.size() == gridPoints.size());

  // define the equilibrium level (EL) as the highest point where the
  // environment curve intersects the lifted parcel curve.  If the
  // parcel is still being lifted at the top of the profile, then
  // return the CAPE but set a flag that warns that the calculation
  // is incomplete

  bool IncompleteCalculation = false;
  size_t i_parceltop         = nearSurfaceGridPoint;

  for (size_t i = nearSurfaceGridPoint; i < gridPoints.size(); ++i) {
    if (gridPoints[i].getVirtualTempK() < TVparcel[i]) { i_parceltop = i; }
  }

  if (gridPoints[i_parceltop].getHeight() <= LCLHeight) {
    // no CAPE for you, since it is defined from the LCL to EL...
    // FIXME: return exerything missing or zero
    CAPE = 0;
    CIN  = 0;
    return;
  }

  if (i_parceltop == (gridPoints.size() - 1) ) {
    // not the true EL, but it is the best we can do, so relay that
    // information back to the calling routine.  CAPE will be affected,
    // too.
    // FIXME: this isn't used just yet.
    IncompleteCalculation = true;
    EL = gridPoints[i_parceltop].getHeight();
  } else {
    // find the height where the Tv difference is zero between the
    // parcel and environment
    double tv_diff_bot =
      TVparcel[i_parceltop] - gridPoints[i_parceltop].getVirtualTempK();
    double tv_diff_top =
      TVparcel[i_parceltop + 1] - gridPoints[i_parceltop + 1].getVirtualTempK();
    EL = interpVal(gridPoints[i_parceltop].getHeight(),
        gridPoints[i_parceltop + 1].getHeight(),
        tv_diff_bot, tv_diff_top, 0);
  }

  // Integrate buoancy from the LCL to the EL.

  const double gravity = 9.81; // m/s^2

  for (size_t i = nearSurfaceGridPoint; i < (gridPoints.size() - 1); ++i) {
    if (gridPoints[i + 1].getHeight() < LCLHeight) { continue; }
    // FIXME: not sure how to handle this -- sometimes one of the
    // TVparcel's is missing (fails to converge...), so for now we
    // will just skip the level.
    if ((gridPoints[i].getVirtualTempK() == Constants::MissingData) ||
      (gridPoints[i + 1].getVirtualTempK() == Constants::MissingData) ||
      (TVparcel[i] == Constants::MissingData) ||
      (TVparcel[i + 1] == Constants::MissingData) ) { continue; }

    double tv_diff_bot = TVparcel[i] - gridPoints[i].getVirtualTempK();
    double tv_diff_top = TVparcel[i + 1] - gridPoints[i + 1].getVirtualTempK();
    double height_bot  = gridPoints[i].getHeight();
    double height_top  = gridPoints[i + 1].getHeight();

    if ((gridPoints[i].getHeight() <= LCLHeight) &&
      (gridPoints[i + 1].getHeight() > LCLHeight) )
    {
      // however, if the LCL is in this layer, then reset the information
      // for the bottom
      tv_diff_bot = 0;
      height_bot  = LCLHeight;
    }

    if ((tv_diff_bot >= 0) && (tv_diff_top >= 0)) {
      // should be all positive area:
      double Tv_env, Tv_parcel;
      if (height_bot == LCLHeight) {
        // the special case that the LCL is in this layer:
        nsePoint lclp(parcel.getLCLTemp(), 100, parcel.getLCLPres());
        Tv_env = (lclp.getVirtualTempK()
          + gridPoints[i + 1].getVirtualTempK() )
          / 2;
        Tv_parcel = ( lclp.getVirtualTempK() + TVparcel[i + 1] ) / 2;
      } else {
        // the typical case - full contribution to positive area:
        Tv_env =
          ( gridPoints[i].getVirtualTempK()
          + gridPoints[i + 1].getVirtualTempK() )
          / 2;
        Tv_parcel = ( TVparcel[i] + TVparcel[i + 1] ) / 2;
      }

      // + CAPE contribution to this layer:
      double energy = gravity * ( ( Tv_parcel - Tv_env ) / Tv_env )
        * ( height_top - height_bot );

      // std::cout << "Tv-env/parcel/energy/height = " <<
      //	Tv_env << " / " << Tv_parcel << " / " << energy << " / " <<
      //	( height_top - height_bot )  << "\n";
      if (energy < 0) {
        fLogInfo("negative CAPE where there should not be... parcel/env {} / {}", Tv_parcel, Tv_env);
      }

      CAPE += energy;
      // FIXME: CAPE_LFCToLFCPlus3km needs to also take into account
      // a partial level at the top
      if ((LFC != Constants::MissingData) &&
        (height_top > LFC) && (height_top < (LFC + 3000)) )
      {
        CAPE_LFCToLFCPlus3km += energy;
      }
      if (height_top < (sfcPoint.getHeight() + 3000)) {
        CAPE_SfcTo3kmAGL += energy;
      }

      // in the rare event that the LFC is exactly at a grid point:
      if ((tv_diff_bot == 0) && (tv_diff_top > 0)) {
        LFC = height_bot;
      }

      // Check to see if this is the level of maximum bouyancy

      double LMB_temp = gravity * ( ( Tv_parcel - Tv_env ) / Tv_env );
      if (LMB_temp > MaxB) {
        // FIXME: we don't multiply by a depth, here, but the original
        // code has this listed as m2/s2.  The problem is that the
        // original code doesn't multiply by a depth...
        MaxB = LMB_temp;
        LMB  = ( height_top + height_bot ) / 2 - sfcPoint.getHeight();
      }
    } else if ((tv_diff_bot < 0) && (tv_diff_top > 0)) {
      // we have an LFC in this layer

      // the height at which the environment and the parcel cross
      double xheight = interpVal(height_bot, height_top,
          tv_diff_bot, tv_diff_top, 0);
      LFC = xheight;

      // FIXME: may want to do pressure (log10) interpolation, here:
      // where the virtual temperature crosses over:
      double xTv = interpVal(TVparcel[i], TVparcel[i + 1],
          height_bot, height_top, xheight);

      // there will be negative area in the bottom part of the layer,
      // and positive energy in the upper part:

      // For the positive part:
      double Tv_env     = ( xTv + gridPoints[i + 1].getVirtualTempK() ) / 2;
      double Tv_parcel  = ( xTv + TVparcel[i + 1] ) / 2;
      double energy_pos = gravity * ( ( Tv_parcel - Tv_env ) / Tv_env )
        * ( height_top - xheight );
      CAPE += energy_pos;
      CAPE_LFCToLFCPlus3km += energy_pos;
      if (height_top < (sfcPoint.getHeight() + 3000)) {
        CAPE_SfcTo3kmAGL += energy_pos;
      }
      // For the negative part:
      Tv_env    = ( gridPoints[i].getVirtualTempK() + xTv ) / 2;
      Tv_parcel = ( TVparcel[i] + xTv ) / 2;
      double energy_neg = gravity * ( ( Tv_parcel - Tv_env ) / Tv_env )
        * ( xheight - height_top );
      CIN += energy_neg;

      // Check to see if this is the level of maximum bouyancy

      double LMB_temp = ( ( Tv_parcel - Tv_env ) / Tv_env );
      if (LMB_temp > MaxB) {
        MaxB = LMB_temp;
        LMB  = ( height_top + height_bot ) / 2 - sfcPoint.getHeight();
      }
    } else if ((tv_diff_bot > 0) && (tv_diff_top <= 0)) {
      // EL in this layer
      // the height at which the environment and the parcel cross
      double xheight = interpVal(height_bot, height_top,
          tv_diff_bot, tv_diff_top, 0);
      // FIXME: may want to do pressure (log10) interpolation, here:
      // where the virtual temperature crosses over:
      double xTv = interpVal(TVparcel[i], TVparcel[i + 1],
          height_bot, height_top, xheight);

      // negative area in the top part of the layer, positive area
      // in the bottom part of the layer:
      //
      // For the negative part:
      double Tv_env     = ( xTv + gridPoints[i + 1].getVirtualTempK() ) / 2;
      double Tv_parcel  = ( xTv + TVparcel[i + 1] ) / 2;
      double energy_neg = gravity * ( ( Tv_parcel - Tv_env ) / Tv_env )
        * ( height_top - xheight );
      // only add to CIN if we are still below the EL:

      if (height_top < EL) {
        CIN += energy_neg;
      }

      if (height_top > EL) {
        MPL_energy -= energy_neg;
      }

      // For the positive part:
      Tv_env    = ( gridPoints[i].getVirtualTempK() + xTv ) / 2;
      Tv_parcel = ( TVparcel[i] + xTv ) / 2;
      double energy_pos = gravity * ( ( Tv_parcel - Tv_env ) / Tv_env )
        * ( xheight - height_top );
      CAPE += energy_pos;
      // FIXME: CAPE_LFCToLFCPlus3km needs to also take into account
      // a partial level at the top
      if ((LFC != Constants::MissingData) &&
        (height_top > LFC) && (height_top < (LFC + 3000)) )
      {
        CAPE_LFCToLFCPlus3km += energy_pos;
      }
      if (height_top < (sfcPoint.getHeight() + 3000)) {
        CAPE_SfcTo3kmAGL += energy_pos;
      }

      // Check to see if this is the level of maximum bouyancy

      double LMB_temp = ( ( Tv_parcel - Tv_env ) / Tv_env );
      if (LMB_temp > MaxB) {
        MaxB = LMB_temp;
        LMB  = ( height_top + height_bot ) / 2 - sfcPoint.getHeight();
      }
    } else {
      // either all negative area (CIN) or above the EL

      double Tv_env, Tv_parcel;
      if (height_bot == LCLHeight) {
        // the special case that the LCL is in this layer:
        nsePoint lclp(parcel.getLCLTemp(), 100, parcel.getLCLPres());
        Tv_env = (lclp.getVirtualTempK()
          + gridPoints[i + 1].getVirtualTempK() )
          / 2;
        Tv_parcel = ( lclp.getVirtualTempK() + TVparcel[i + 1] ) / 2;
      } else {
        // the typical case:
        Tv_env =
          ( gridPoints[i].getVirtualTempK()
          + gridPoints[i + 1].getVirtualTempK() )
          / 2;
        Tv_parcel = ( TVparcel[i] + TVparcel[i + 1] ) / 2;
      }

      // - CAPE contribution to this layer:
      double energy = gravity * ( ( Tv_parcel - Tv_env ) / Tv_env )
        * ( height_top - height_bot );

      if (energy > 0) {
        #if 0
        fLogInfo("positive CAPE where there should not be...\n"
          << "  parcel/env  " << Tv_parcel << " / " << Tv_env << " ht top/bot ="
          << height_top << " / " << height_bot << " tdiff top/bot = " <<
          tv_diff_top << " / " << tv_diff_bot << " i/sfc = " << i << "/"
          << nearSurfaceGridPoint << "\n");

        fLogInfo("Tv_env 1/2 = " << gridPoints[i].getVirtualTempK()
                                 << " / " << gridPoints[i + 1].getVirtualTempK()
                                 << " Tv_parc 1/2 = " << TVparcel[i] << " / " << TVparcel[i + 1]
                                 << "\n");
        #endif // if 0
      }
      if (height_top < EL) { CIN += energy; }
      if (height_top > EL) { MPL_energy -= energy; }
    }
    // if the MPL_energy is now greater than CAPE and we are above the EL,
    // then we need to calculate the MPL.  If it is above the top grid point,
    // then the value will remain flagged as missing.

    if ((CAPE > 0) && (height_top > EL) && (i > 0) &&
      (MPL_energy >= CAPE) && (MPL_energy_old < CAPE) )
    {
      MPL = interpVal(gridPoints[i - 1].getHeight(), gridPoints[i].getHeight(),
          MPL_energy_old, MPL_energy, CAPE);
    }
    MPL_energy_old = MPL_energy;
  }

  // FIXME: this is a safeguard -- for some reason, you will occasionally
  // get an EL with no LFC
  if (LFC == Constants::MissingData) { EL = Constants::MissingData; }

  // make CIN a positive number

  CIN = -CIN;

  // Sometimes CAPE will end up as a very small negative number.  Fix this:

  if (Constants::isGood(CAPE) && (CAPE < 0)) { CAPE = 0; }
  if (Constants::isGood(CAPE_LFCToLFCPlus3km) && (CAPE_LFCToLFCPlus3km < 0)) { CAPE_LFCToLFCPlus3km = 0; }
  if (Constants::isGood(CAPE_SfcTo3kmAGL) && (CAPE_SfcTo3kmAGL < 0)) { CAPE_SfcTo3kmAGL = 0; }

  return;
} // nseProfile::LiftParcel

double
nseProfile::getDCAPE(double parcelHeightAGL)
{
  // calculate the Downdraft CAPE for the parcel at the given height

  if ((gridPoints.size() <= 1) || (parcelHeightAGL == Constants::MissingData)) {
    return Constants::MissingData;
  }

  const double gravity = 9.81; // m/s^2
  double parcelHeightMSL;

  if (sfcPoint.getHeight() != Constants::MissingData) {
    parcelHeightMSL = parcelHeightAGL + sfcPoint.getHeight();
  } else {
    parcelHeightMSL = parcelHeightAGL;
    // or we could return a missing value, perhaps
  }

  nsePoint parcel;
  bool foundParcel = false;

  // find and initialize the parcel that we will be using
  for (size_t i = 1; i < gridPoints.size(); ++i) {
    if ((i >= nearSurfaceGridPoint) &&
      (parcelHeightMSL >= gridPoints[i - 1].getHeight()) &&
      (parcelHeightMSL < gridPoints[i].getHeight()) )
    {
      double pres = interpVal(gridPoints[i - 1].getPressure(),
          gridPoints[i].getPressure(),
          gridPoints[i - 1].getHeight(),
          gridPoints[i].getHeight(),
          parcelHeightMSL);
      double temp = interpVal(gridPoints[i - 1].getTempC(),
          gridPoints[i].getTempC(),
          gridPoints[i - 1].getHeight(),
          gridPoints[i].getHeight(),
          parcelHeightMSL);
      double dewp = interpVal(gridPoints[i - 1].getDewPointC(),
          gridPoints[i].getDewPointC(),
          gridPoints[i - 1].getHeight(),
          gridPoints[i].getHeight(),
          parcelHeightMSL);
      double uwind = Constants::MissingData;
      double vwind = Constants::MissingData;
      parcel      = nsePoint(temp, dewp, parcelHeightMSL, pres, uwind, vwind);
      foundParcel = true;
      break;
    }
  }

  if (!foundParcel) { return Constants::MissingData; }

  // integrate DCAPE from the parcel level to the ground.  This is
  // simply the area between the environment (Tv) and the moist
  // adiabat of parcel descent.

  double TVparcel_last = Constants::MissingData;
  double DCAPE         = 0;

  // calculate the DCAPE for the nearest point to the ground up to the
  // parcelHeightMSL

  size_t numPoints = gridPoints.size();

  for (size_t i = (numPoints - 1); i > nearSurfaceGridPoint; i--) {
    assert(i < numPoints && "Out of range");
    if (gridPoints[i].getHeight() < parcelHeightMSL) {
      assert((i + 1) < numPoints && "Out of range");
      // we are below the parcel's tarting height, so we can integrate
      double height_top = gridPoints[i + 1].getHeight();
      double height_bot = gridPoints[i].getHeight();

      double Tv_env = Constants::MissingData;

      if (gridPoints[i + 1].getHeight() > parcelHeightMSL) {
        // this is the top point, so integrate from parcelHeightMSL to the
        // bottom of the layer

        height_top    = parcel.getHeight();
        TVparcel_last = parcel.getVirtualTempK();
        Tv_env        =
          ( gridPoints[i].getVirtualTempK()
          + parcel.getVirtualTempK() )
          / 2;
      } else {
        // the typical case
        Tv_env =
          ( gridPoints[i].getVirtualTempK()
          + gridPoints[i + 1].getVirtualTempK() )
          / 2;
      }

      // we calculate the parcel's virtual temperature curve from the
      // parcel level to the surface

      double TVparcel_temp = getMoistParcelTemp(parcel.getThetaE(),
          gridPoints[i].getPressure() );

      double Tv_parcel;
      if ((TVparcel_temp != Constants::MissingData) &&
        (TVparcel_last != Constants::MissingData) &&
        (gridPoints[i].getVirtualTempK() != Constants::MissingData) &&
        (gridPoints[i + 1].getVirtualTempK() != Constants::MissingData) &&
        (parcel.getVirtualTempK() != Constants::MissingData) &&
        (height_top != Constants::MissingData) &&
        (height_bot != Constants::MissingData) )
      {
        Tv_parcel = ( TVparcel_temp + TVparcel_last ) / 2;
        double energy = gravity * ( ( Tv_parcel - Tv_env ) / Tv_env )
          * ( height_top - height_bot );
        DCAPE -= energy;
      }

      TVparcel_last = TVparcel_temp;
    }
  }

  // calculate from the surface to the first grid point above ground

  double TVparcel_ns = getMoistParcelTemp(parcel.getThetaE(),
      gridPoints[nearSurfaceGridPoint].getPressure() );
  double TVparcel_sfc = getMoistParcelTemp(parcel.getThetaE(),
      sfcPoint.getPressure() );
  double TV_parcel_sfc = (TVparcel_ns + TVparcel_sfc) / 2;
  double TV_env_sfc    =
    ( gridPoints[nearSurfaceGridPoint].getVirtualTempK()
    + sfcPoint.getVirtualTempK() ) / 2;
  double energy_sfc = gravity * ( ( TV_parcel_sfc - TV_env_sfc ) / TV_env_sfc )
    * ( gridPoints[nearSurfaceGridPoint].getHeight() - sfcPoint.getHeight());

  DCAPE -= energy_sfc;
  if (DCAPE < 0) { DCAPE = 0; }

  // all done
  return DCAPE;
} // nseProfile::getDCAPE

void
nseProfile::getStormMotionJohns1993(
  double MeanSpeed,
  double MeanU,
  double MeanV,
  double & StormSpeed,
  double & StormU,
  double & StormV)
{
  // See p. 587 of Johns et al. (Tornado Symposium book, 1993)

  if ((MeanSpeed == Constants::MissingData) ||
    (MeanU == Constants::MissingData) ||
    (MeanV == Constants::MissingData) )
  {
    StormSpeed = Constants::MissingData;
    StormU     = Constants::MissingData;
    StormV     = Constants::MissingData;
    return;
  }

  double deviantDegrees;
  double deviantSpeedPercent;

  if (MeanSpeed <= 15) {
    deviantDegrees      = 30.0;
    deviantSpeedPercent = 75.0;
  } else {
    deviantDegrees      = 20.0;
    deviantSpeedPercent = 85.0;
  }
  StormSpeed = 0.01 * deviantSpeedPercent * MeanSpeed;

  // Find the angle (conventional notation, where 90 deg is north) towards
  // which the average wind over the specified depth is blowing...

  // FIXME: check the math on this part once it's compiled as part of a program
  // (formerly used WDSS_II Angle class, should work now, but could have errors)
  double MeanAngle;

  if ( (MeanU == 0) && (MeanV == 0)) {
    MeanAngle = 270.0;
  } else if ((MeanU == 0) && (MeanV > 0)) {
    MeanAngle = 90.0;
  } else {
    MeanAngle = atan2(MeanV * DEG_TO_RAD, MeanU * DEG_TO_RAD) * RAD_TO_DEG;
    if (MeanAngle < 0) {
      MeanAngle += 360;
    }
  }

  // Determine the angle (conventional notation, where 90 deg is north)
  // towards which the deviant storm is moving.

  double TempAng;

  if ( (MeanAngle - deviantDegrees) < 0) {
    TempAng = MeanAngle - deviantDegrees + 360;
  } else {
    TempAng = MeanAngle - deviantDegrees;
  }

  StormU = cos(TempAng * DEG_TO_RAD) * StormSpeed;
  StormV = sin(TempAng * DEG_TO_RAD) * StormSpeed;
} // nseProfile::getStormMotionJohns1993

nsePoint
nseProfile::getMostUnstableParcel(double layerDepth)
{
  // return the most unstable parcel in the lowest layerDepth mb

  nsePoint UnstableParcel = sfcPoint;

  for (size_t i = nearSurfaceGridPoint; i < gridPoints.size(); ++i) {
    if ((gridPoints[i].getThetaE() > UnstableParcel.getThetaE()) &&
      ((gridPoints[i].getPressure() + layerDepth) > sfcPoint.getPressure()))
    {
      UnstableParcel = gridPoints[i];
    }
  }
  return UnstableParcel;
}

nsePoint
nseProfile::getAverageParcel(double layerDepth)
{
  nsePoint AverageParcel;

  // interpolate to get the height of the parcel at layerDepth above
  // the surface

  double toppres = sfcPoint.getPressure() - layerDepth;
  double ht = Constants::MissingData;
  size_t i_top = nearSurfaceGridPoint;
  std::vector<double> v_temp, v_dewp, v_uwind, v_vwind;

  // find the height of "layerDepth" mb, and populate vectors
  for (size_t i = nearSurfaceGridPoint; i < (gridPoints.size() - 1); ++i) {
    if ((gridPoints[i].getPressure() > toppres) &&
      (gridPoints[i + 1].getPressure() <= toppres) &&
      (gridPoints[i].getPressure() != Constants::MissingData) &&
      (gridPoints[i + 1].getPressure() != Constants::MissingData) )
    {
      ht = interpVal(gridPoints[i].getHeight(), gridPoints[i + 1].getHeight(),
          gridPoints[i].getPressure(), gridPoints[i + 1].getPressure(), toppres);
      i_top = i + 1;
    }
  }
  for (size_t i = 0; i < gridPoints.size(); ++i) {
    v_temp.push_back(gridPoints[i].getTempC());
    v_dewp.push_back(gridPoints[i].getDewPointC());
    v_uwind.push_back(gridPoints[i].getUWind());
    v_vwind.push_back(gridPoints[i].getVWind());
  }

  if (ht == Constants::MissingData) {
    nsePoint np1;
    return np1;
  }

  // get mean values and create an average grid point
  double temp  = getMeanValueLayer(sfcPoint.getHeight(), ht, v_temp);
  double dewp  = getMeanValueLayer(sfcPoint.getHeight(), ht, v_dewp);
  double uwind = getMeanValueLayer(sfcPoint.getHeight(), ht, v_uwind);
  double vwind = getMeanValueLayer(sfcPoint.getHeight(), ht, v_vwind);
  double pres  = sfcPoint.getPressure() - (layerDepth / 2 );

  /*
   * std::cout << "p/ht/t/td = " << pres << " / " << ht << " / " <<
   *  temp << " / " << dewp << " \n";
   * std::cout << "   -- sfc p/ht/t/td " << sfcPoint.getPressure() << " / "
   *  << sfcPoint.getHeight() << " / " << sfcPoint.getTempC() << " / "
   *  << sfcPoint.getDewPointC() << "\n";
   */
  nsePoint np(temp, dewp, ht, pres, uwind, vwind);

  return np;
} // nseProfile::getAverageParcel

double
nseProfile::getMoistParcelTemp(double thetae, double pres)
{
  // calculate the parcel temperature at the given pressure for a
  // given parcel thetaE, using Newton-Rhapson interation.
  //
  if ((pres == Constants::MissingData) ||
    (thetae == Constants::MissingData) )
  {
    return Constants::MissingData;
  }
  //  std::cout << "calculating temp for thetea/pres = " << thetae << " / " << pres << "\n";
  double maxt = 0;

  if ((thetae - 270) > 0) { maxt = thetae - 270; }

  // generate a first-guess the GEMPAK way (and in degreeC)
  double tguess = (thetae - 0.5 * pow(maxt, 1.05)) * pow(pres / 1000, 0.2)
    - 273.15;
  // set convergence
  double conv = 0.01;
  // 297.621 / 900
  //
  double oldguess1 = -999999999999999.0;
  double oldguess2 = -999999999999999.0;

  // iterate by degrees
  for (size_t i = 1; i <= 100; ++i) {
    nsePoint np1(tguess, 100, pres);
    double thetae1 = np1.getThetaE();
    nsePoint np2(tguess + 1, 100, pres);
    double thetae2 = np2.getThetaE();
    // std::cout << "ThetaE guesses:  " << thetae1 << " / " << thetae2 << "\n";
    if ((thetae1 == Constants::MissingData) ||
      (thetae2 == Constants::MissingData) )
    {
      return Constants::MissingData;
    }

    double correction = ( thetae - thetae1) / (thetae2 - thetae1);
    tguess += correction;
    // ADDED: are we stuck in a loop on either side of the solution?
    if (((oldguess2 - tguess) < 0.1) && (fabs(correction) < 0.3)) {
      // FIXME: not good enough, but need it to help find other bugs
      tguess += 273.15;
      return tguess;
    }
    if (fabs(correction) < conv) {
      // return on solution
      tguess += 273.15;
      return tguess;
    }

    oldguess1 = tguess;
    oldguess2 = oldguess1;
    // std::cout << i << " Tguess = " << tguess << " thetaEs = "
    // << thetae1 << " / " << thetae2 << " corr = " << correction << "\n";
    //    if (i == 100)
    //    std::cout << "Failed to converge!  Tguess = " << tguess << " thetaEs = "
    //      << thetae1 << " / " << thetae2 << " corr = " << correction << "\n";
  }
  // failed to converge
  return Constants::MissingData;
} // nseProfile::getMoistParcelTemp

double
nseProfile::getHeightOfWetBulbZero()
{
  // return the topmost height where the wet-bulb
  // temperature equals zero.
  double lastWBTemp   = Constants::MissingData;
  const int numPoints = gridPoints.size();

  for (int i = (numPoints - 1);
    i >= (int) nearSurfaceGridPoint; i--)
  {
    assert(i < numPoints && "Out of range");
    double WBTemp = getMoistParcelTemp(gridPoints[i].getThetaE(),
        gridPoints[i].getPressure());
    if (lastWBTemp != Constants::MissingData) {
      // this "if" skips the first point...
      if ((WBTemp >= 273.16) && (lastWBTemp < 273.16) && (lastWBTemp !=
        Constants::MissingData) )
      {
        assert((i + 1) < numPoints && "Out of range");
        // we found it
        double ht = interpVal(gridPoints[i].getHeight(),
            gridPoints[i + 1].getHeight(), WBTemp, lastWBTemp, 273.16);
        return ht;
      }
    }
    lastWBTemp = WBTemp;
  }

  // is it between the surface and the first gridpoint above?

  double sfcWBT = getMoistParcelTemp(sfcPoint.getThetaE(),
      sfcPoint.getPressure() );

  if ((sfcWBT >= 273.16) && (lastWBTemp < 273.16) && (lastWBTemp !=
    Constants::MissingData) )
  {
    double ht = interpVal(sfcPoint.getHeight(),
        gridPoints[nearSurfaceGridPoint].getHeight(),
        sfcWBT, lastWBTemp, 273.16);
    return ht;
  }

  return Constants::MissingData;
} // nseProfile::getHeightOfWetBulbZero

double
nseProfile::getHeightOfWetBulbFour()
{
  // return the topmost height where the wet-bulb
  // temperature equals four.
  double lastWBTemp   = Constants::MissingData;
  const int numPoints = gridPoints.size();

  for (int i = (numPoints - 1);
    i >= (int) nearSurfaceGridPoint; i--)
  {
    assert(i < numPoints && "Out of range");
    double WBTemp = getMoistParcelTemp(gridPoints[i].getThetaE(),
        gridPoints[i].getPressure());
    if (lastWBTemp != Constants::MissingData) {
      // this "if" skips the first point...
      if ((WBTemp >= 277.16) && (lastWBTemp < 277.16) && (lastWBTemp !=
        Constants::MissingData) )
      {
        assert((i + 1) < numPoints && "Out of range");
        // we found it
        double ht = interpVal(gridPoints[i].getHeight(),
            gridPoints[i + 1].getHeight(), WBTemp, lastWBTemp, 277.16);
        return ht;
      }
    }
    lastWBTemp = WBTemp;
  }

  // is it between the surface and the first gridpoint above?

  double sfcWBT = getMoistParcelTemp(sfcPoint.getThetaE(),
      sfcPoint.getPressure() );

  if ((sfcWBT >= 277.16) && (lastWBTemp < 277.16) && (lastWBTemp !=
    Constants::MissingData) )
  {
    double ht = interpVal(sfcPoint.getHeight(),
        gridPoints[nearSurfaceGridPoint].getHeight(),
        sfcWBT, lastWBTemp, 277.16);
    return ht;
  }

  return Constants::MissingData;
} // nseProfile::getHeightOfWetBulbFour

double
nseProfile::getVerticallyIntegratedWetBulbTemp(double WBHeight)
{
  // don't have a ref for this -- just a verbatim copy from the
  // NSE fortran code

  if (WBHeight == Constants::MissingData) { return Constants::MissingData; }
  if (WBHeight < sfcPoint.getHeight() ) { return Constants::MissingData; }

  std::vector<double> WBTemp;
  std::vector<double> Height;
  std::vector<double> Density;

  double wbt = getMoistParcelTemp(sfcPoint.getThetaE(),
      sfcPoint.getPressure() ) - 273.16;

  if (wbt < 0) { return 0; }

  WBTemp.push_back(wbt);
  Height.push_back(sfcPoint.getHeight() );
  Density.push_back(nsePoint::calcDensity(sfcPoint.getPressure(),
    sfcPoint.getTempK() ) );

  for (size_t i = nearSurfaceGridPoint; i < (gridPoints.size() - 1); ++i) {
    if (gridPoints[i].getHeight() == Constants::MissingData) {
      return Constants::MissingData;
    }
    if (gridPoints[i].getHeight() < WBHeight) {
      Height.push_back(gridPoints[i].getHeight() );
      Density.push_back(nsePoint::calcDensity(gridPoints[i].getPressure(),
        gridPoints[i].getTempK() ) );
      wbt = getMoistParcelTemp(gridPoints[i].getThetaE(),
          gridPoints[i].getPressure() ) - 273.16;
      WBTemp.push_back(wbt);
    } else {
      WBTemp.push_back(0);
      Height.push_back(WBHeight);
      // don't bother to interpolate density to WBHeight; minimal impact
      Density.push_back(nsePoint::calcDensity(gridPoints[i].getPressure(),
        gridPoints[i].getTempK() ) );
      break;
    }
  }

  assert(Height.size() == Density.size() );
  assert(Height.size() == WBTemp.size() );

  double sumDensity = 0;
  double VertIntWBZ = 0;

  for (size_t i = 0; i < ( Height.size() - 1); ++i) {
    double avgWBT      = 0.5 * ( WBTemp[i] + WBTemp[i + 1] );
    double avgDensity  = 0.5 * ( Density[i] + Density[i + 1] );
    double deltaHeight = Height[i + 1] - Height[i];
    VertIntWBZ += (avgWBT * avgDensity * deltaHeight );
    sumDensity += avgDensity;
  }
  double val = (double) (WBTemp.size() - 1) * 0.001 * VertIntWBZ / sumDensity;

  return val;
} // nseProfile::getVerticallyIntegratedWetBulbTemp

double
nseProfile::EnergyHelicityIndex(double CAPE, double SREH)
{
  if ((CAPE == Constants::MissingData) ||
    (SREH == Constants::MissingData) )
  {
    return Constants::MissingData;
  }

  double ehi = (CAPE * SREH) / 160000;

  return ehi;
}

double
nseProfile::getDeepShearVector()
{
  // the 9-11 km AGL vector minus the 0-2 km AGL vector

  if ((sfcPoint.getUWind() == Constants::MissingData) ||
    (sfcPoint.getVWind() == Constants::MissingData) ||
    (sfcPoint.getHeight() == Constants::MissingData) )
  {
    return Constants::MissingData;
  }

  std::vector<double> uwind;
  std::vector<double> vwind;
  std::vector<double> height;

  uwind.push_back(sfcPoint.getUWind() );
  vwind.push_back(sfcPoint.getVWind() );
  height.push_back(sfcPoint.getHeight() );

  for (size_t i = nearSurfaceGridPoint; i < gridPoints.size(); ++i) {
    if ((gridPoints[i].getUWind() != Constants::MissingData) &&
      (gridPoints[i].getVWind() != Constants::MissingData) &&
      (gridPoints[i].getHeight() != Constants::MissingData) )
    {
      uwind.push_back(gridPoints[i].getUWind());
      vwind.push_back(gridPoints[i].getVWind());
      height.push_back(gridPoints[i].getHeight());
    }
  }
  if (uwind.size() == 0) { return Constants::MissingData; }

  double botU = getMeanValueLayer(sfcPoint.getHeight(),
      sfcPoint.getHeight() + 2000, uwind, height);
  double botV = getMeanValueLayer(sfcPoint.getHeight(),
      sfcPoint.getHeight() + 2000, vwind, height);
  double topU = getMeanValueLayer(sfcPoint.getHeight() + 9000,
      sfcPoint.getHeight() + 11000, uwind, height);
  double topV = getMeanValueLayer(sfcPoint.getHeight() + 9000,
      sfcPoint.getHeight() + 11000, vwind, height);

  double shear = pow(pow(topU - botU, 2) + pow(topV - botV, 2), 0.5);

  return shear;
} // nseProfile::getDeepShearVector

double
nseProfile::getStormRelativeFlow(double height_bot_agl,
  double height_top_agl, double stormU, double stormV)
{
  // this is simply the mean flow minus the storm motion vector

  if ((height_bot_agl == Constants::MissingData) ||
    (height_top_agl == Constants::MissingData) ||
    (stormU == Constants::MissingData) ||
    (stormV == Constants::MissingData) ||
    (sfcPoint.getHeight() == Constants::MissingData) ||
    (height_top_agl <= 0) ||
    (height_top_agl <= height_bot_agl) )
  {
    return Constants::MissingData;
  }

  if (height_bot_agl < 0) { height_bot_agl = 0; }

  std::vector<double> uwind;
  std::vector<double> vwind;
  std::vector<double> height;

  if ((sfcPoint.getUWind() != Constants::MissingData) &&
    (sfcPoint.getUWind() != Constants::MissingData) &&
    (sfcPoint.getHeight() != Constants::MissingData) )
  {
    uwind.push_back(sfcPoint.getUWind());
    vwind.push_back(sfcPoint.getUWind());
    height.push_back(sfcPoint.getHeight());
  }

  for (size_t i = 0; i < gridPoints.size(); ++i) {
    if ((gridPoints[i].getUWind() != Constants::MissingData) &&
      (gridPoints[i].getVWind() != Constants::MissingData) &&
      (gridPoints[i].getHeight() != Constants::MissingData) )
    {
      uwind.push_back(gridPoints[i].getUWind());
      vwind.push_back(gridPoints[i].getVWind());
      height.push_back(gridPoints[i].getHeight());
    }
  }

  if (uwind.size() == 0) { return Constants::MissingData; }

  double height_top = height_top_agl + sfcPoint.getHeight();

  // test:  this might work to get rid of nasty big values:
  if (height[height.size() - 1] < height_top) { return Constants::MissingData; }
  double height_bot = height_bot_agl + sfcPoint.getHeight();
  double uMean      = getMeanValueLayer(height_bot, height_top, uwind, height);
  double vMean      = getMeanValueLayer(height_bot, height_top, vwind, height);

  if ((uMean == Constants::MissingData) || (vMean == Constants::MissingData)) {
    return Constants::MissingData;
  }

  double uFlow = uMean - stormU;
  double vFlow = vMean - stormV;
  double Flow  = pow(uFlow * uFlow + vFlow * vFlow, 0.5);

  return Flow;
} // nseProfile::getStormRelativeFlow

double
nseProfile::getBRNshear()
{
  // FIXME: needs to be the pressure-weighted wind speed, methinks

  if (sfcPoint.getHeight() == Constants::MissingData) {
    return Constants::MissingData;
  }

  std::vector<double> uwind;
  std::vector<double> vwind;
  std::vector<double> height;

  if ((sfcPoint.getHeight() != Constants::MissingData) &&
    (sfcPoint.getUWind() != Constants::MissingData) &&
    (sfcPoint.getVWind() != Constants::MissingData) )
  {
    height.push_back(sfcPoint.getHeight());
    uwind.push_back(sfcPoint.getUWind());
    vwind.push_back(sfcPoint.getVWind());
  }

  for (size_t i = 0; i < gridPoints.size(); ++i) {
    if ((gridPoints[i].getHeight() != Constants::MissingData) &&
      (gridPoints[i].getUWind() != Constants::MissingData) &&
      (gridPoints[i].getVWind() != Constants::MissingData) )
    {
      height.push_back(gridPoints[i].getHeight());
      uwind.push_back(gridPoints[i].getUWind());
      vwind.push_back(gridPoints[i].getVWind());
    }
  }
  double u500 = getMeanValueLayer(sfcPoint.getHeight(),
      sfcPoint.getHeight() + 500, uwind, height);
  double v500 = getMeanValueLayer(sfcPoint.getHeight(),
      sfcPoint.getHeight() + 500, vwind, height);
  double u6000 = getMeanValueLayer(sfcPoint.getHeight(),
      sfcPoint.getHeight() + 6000, uwind, height);
  double v6000 = getMeanValueLayer(sfcPoint.getHeight(),
      sfcPoint.getHeight() + 6000, vwind, height);

  if ((u500 == Constants::MissingData) ||
    (v500 == Constants::MissingData) ||
    (u6000 == Constants::MissingData) ||
    (v6000 == Constants::MissingData) )
  {
    return Constants::MissingData;
  }

  double u500speed  = pow(u500 * u500 + v500 * v500, 0.5);
  double u6000speed = pow(u6000 * u6000 + v6000 * v6000, 0.5);
  double s2         = 0.5 * pow(u6000speed - u500speed, 2.0);

  return s2;
} // nseProfile::getBRNshear

double
nseProfile::getBRN(double CAPE, double ShearSquared)
{
  if ((CAPE != Constants::MissingData) &&
    (ShearSquared != Constants::MissingData) )
  {
    return (CAPE / ShearSquared);
  } else {
    return Constants::MissingData;
  }
}

double
nseProfile::getTempDifferencePressureLayer(double topmb, double botmb)
{
  // return the difference in temperature between two pressure levels

  std::vector<double> param;
  std::vector<double> pres;

  for (size_t i = 0; i < gridPoints.size(); ++i) {
    param.push_back(gridPoints[i].getTempC());
    pres.push_back(gridPoints[i].getPressure());
  }
  double diff = getParamDifferencePressureLayer(topmb, botmb, param, pres);

  return diff;
}

double
nseProfile::getParamDifferencePressureLayer(double topmb, double botmb,
  const std::vector<double> &param, const std::vector<double> &pres)
{
  if (param.size() <= 1) { return Constants::MissingData; }

  double botval = Constants::MissingData;
  double topval = Constants::MissingData;

  // FIXME: use a logarithmic interpolation scheme
  for (size_t i = 0; i < (param.size() - 1); ++i) {
    if (pres[i] == botmb) {
      botval = param[i];
    } else if ((pres[i] < botmb) && (pres[i + 1] > botmb)) {
      botval = interpVal(param[i], param[i + 1], pres[i], pres[i + 1], botmb);
    }
    if (pres[i] == topmb) {
      topval = param[i];
    } else if ((pres[i] < topmb) && (pres[i + 1] > topmb)) {
      topval = interpVal(param[i], param[i + 1], pres[i], pres[i + 1], botmb);
    }
  }
  if ((botval != Constants::MissingData) && (topval != Constants::MissingData)) {
    return (topval - botval);
  } else { return Constants::MissingData; }
}

double
nseProfile::getWindSpeedAtHeightAGL(double heightAGL,
  std::string                              WindType)
{
  // Windtype info:
  // "U" returns the U component of the wind at this level
  // "V" returns the V component of the wind at this level
  // "Speed" returns the speed of the wind at this level
  //
  double height;

  if (sfcPoint.getHeight() != Constants::MissingData) {
    height = heightAGL + sfcPoint.getHeight();
  } else {
    height = heightAGL;
  }

  double U     = Constants::MissingData;
  double V     = Constants::MissingData;
  double speed = Constants::MissingData;

  for (size_t i = 0; i < (gridPoints.size() - 1); ++i) {
    if ((gridPoints[i].getHeight() <= height) &&
      (gridPoints[i + 1].getHeight() > height) )
    {
      U = interpVal(gridPoints[i].getUWind(), gridPoints[i + 1].getUWind(),
          gridPoints[i].getHeight(), gridPoints[i + 1].getHeight(), height);
      if (WindType == "U") { return U; }
      V = interpVal(gridPoints[i].getUWind(), gridPoints[i + 1].getUWind(),
          gridPoints[i].getHeight(), gridPoints[i + 1].getHeight(), height);
      if (WindType == "V") { return V; }
      if ((U != Constants::MissingData) && (V != Constants::MissingData)) {
        speed = pow(U * U + V * V, 0.5);
      } else { speed = Constants::MissingData; }
      if (WindType == "Speed") { return speed; } else { return Constants::MissingData; }
    }
  }
  return Constants::MissingData;
} // nseProfile::getWindSpeedAtHeightAGL

double
nseProfile::getSpeedShear(double topHeightAGL)
{
  // return the value of the speed shear for the layer from the
  // surface to height topHeightAGL
  double topU = getWindSpeedAtHeightAGL(topHeightAGL, "U");
  double topV = getWindSpeedAtHeightAGL(topHeightAGL, "V");

  if ((topU == Constants::MissingData) ||
    (topV == Constants::MissingData) ||
    (sfcPoint.getUWind() == Constants::MissingData) ||
    (sfcPoint.getVWind() == Constants::MissingData) )
  {
    return Constants::MissingData;
  }

  double shear = pow(topU * topU + topV * topV, 0.5)
    - pow((sfcPoint.getUWind() * sfcPoint.getUWind()
      + sfcPoint.getVWind() * sfcPoint.getVWind()), 0.5);

  return shear;
}

double
nseProfile::getShearVectorMagnitude(double topHeightAGL)
{
  // return the value of the shear vector for the layer from the
  // surface to height topHeightAGL

  if (topHeightAGL == Constants::MissingData) {
    return Constants::MissingData;
  }

  double topU = getWindSpeedAtHeightAGL(topHeightAGL, "U");
  double topV = getWindSpeedAtHeightAGL(topHeightAGL, "V");

  if ((topU == Constants::MissingData) ||
    (topV == Constants::MissingData) ||
    (sfcPoint.getUWind() == Constants::MissingData) ||
    (sfcPoint.getVWind() == Constants::MissingData) )
  {
    return Constants::MissingData;
  }

  double shear = pow(pow(topU - sfcPoint.getUWind(), 2)
      + pow(topV - sfcPoint.getVWind(), 2), 0.5);

  return shear;
}

void
nseProfile::getJunkIndices(double LI, double& VerticalTotals,
  double& CrossTotals, double& TotalTotals, double& KIndex,
  double& SWEAT, double& SSI)
{
  // For historical reasons only, I hope!
  // Lifted Index is the only input value, here.

  VerticalTotals = Constants::MissingData;
  CrossTotals    = Constants::MissingData;
  TotalTotals    = Constants::MissingData;
  SWEAT  = Constants::MissingData;
  SSI    = Constants::MissingData;
  KIndex = Constants::MissingData;

  double TempC_850     = Constants::MissingData;
  double TempC_500     = Constants::MissingData;
  double DewPointC_850 = Constants::MissingData;
  double TempC_700     = Constants::MissingData;
  double DewPointC_700 = Constants::MissingData;
  double DD_700        = Constants::MissingData;
  double WindSpeed500  = Constants::MissingData;
  double WindSpeed850  = Constants::MissingData;
  double WindDir500    = Constants::MissingData;
  double WindDir850    = Constants::MissingData;

  for (size_t i = 0; i < (gridPoints.size() - 1); ++i) {
    if ((gridPoints[i].getPressure() >= 850) &&
      (gridPoints[i + 1].getPressure() < 850) )
    {
      TempC_850 = interpVal(gridPoints[i].getTempC(),
          gridPoints[i + 1].getTempC(), gridPoints[i].getPressure(),
          gridPoints[i + 1].getPressure(), 850);
      DewPointC_850 = interpVal(gridPoints[i].getDewPointC(),
          gridPoints[i + 1].getDewPointC(), gridPoints[i].getPressure(),
          gridPoints[i + 1].getPressure(), 850);
      double U = interpVal(gridPoints[i].getUWind(),
          gridPoints[i + 1].getUWind(), gridPoints[i].getPressure(),
          gridPoints[i + 1].getPressure(), 850);
      double V = interpVal(gridPoints[i].getVWind(),
          gridPoints[i + 1].getVWind(), gridPoints[i].getPressure(),
          gridPoints[i + 1].getPressure(), 850);
      if ((U != Constants::MissingData) && (V != Constants::MissingData)) {
        WindSpeed850 = pow(U * U + V * V, 0.5);
        WindDir850   = atan2(U, V) * (180 / 3.14159) - 180;
        if (WindDir850 < 0) { WindDir850 = 360 + WindDir850; }
      }
    } else if ((gridPoints[i].getPressure() >= 700) &&
      (gridPoints[i + 1].getPressure() < 700) )
    {
      TempC_700 = interpVal(gridPoints[i].getTempC(),
          gridPoints[i + 1].getTempC(), gridPoints[i].getPressure(),
          gridPoints[i + 1].getPressure(), 700);
      DewPointC_700 = interpVal(gridPoints[i].getDewPointC(),
          gridPoints[i + 1].getDewPointC(), gridPoints[i].getPressure(),
          gridPoints[i + 1].getPressure(), 700);
      if ((TempC_700 != Constants::MissingData) &&
        (DewPointC_700 != Constants::MissingData) )
      {
        DD_700 = TempC_700 - DewPointC_700;
      }
    } else if ((gridPoints[i].getPressure() >= 500) &&
      (gridPoints[i + 1].getPressure() < 500) )
    {
      TempC_500 = interpVal(gridPoints[i].getTempC(),
          gridPoints[i + 1].getTempC(), gridPoints[i].getPressure(),
          gridPoints[i + 1].getPressure(), 500);
      double U = interpVal(gridPoints[i].getUWind(),
          gridPoints[i + 1].getUWind(), gridPoints[i].getPressure(),
          gridPoints[i + 1].getPressure(), 500);
      double V = interpVal(gridPoints[i].getVWind(),
          gridPoints[i + 1].getVWind(), gridPoints[i].getPressure(),
          gridPoints[i + 1].getPressure(), 500);
      if ((U != Constants::MissingData) && (V != Constants::MissingData)) {
        WindSpeed500 = pow(U * U + V * V, 0.5);
        WindDir500   = atan2(U, V) * (180 / 3.14159) - 180;
        if (WindDir500 < 0) { WindDir500 = 360 + WindDir500; }
      }
    }
  }
  if ((TempC_850 != Constants::MissingData) &&
    (TempC_500 != Constants::MissingData) )
  {
    VerticalTotals = TempC_850 - TempC_500;
  }
  if ((DewPointC_850 != Constants::MissingData) &&
    (TempC_500 != Constants::MissingData) )
  {
    CrossTotals = DewPointC_850 - TempC_500;
  }
  if ( (CrossTotals != Constants::MissingData) &&
    (VerticalTotals != Constants::MissingData) )
  {
    TotalTotals = VerticalTotals + CrossTotals;
  }
  if ((TempC_850 != Constants::MissingData) &&
    (DewPointC_850 != Constants::MissingData) &&
    (TempC_500 != Constants::MissingData) &&
    (DD_700 != Constants::MissingData) )
  {
    KIndex = TempC_850 + DewPointC_850 + TempC_500 + DD_700;
  }
  if ( (LI != Constants::MissingData) && (WindSpeed500 != Constants::MissingData)) {
    SSI = 3.778 * LI * WindSpeed500 * WindSpeed500;
  }

  if ((TempC_850 != Constants::MissingData) &&
    (TotalTotals != Constants::MissingData) &&
    (WindSpeed500 != Constants::MissingData) &&
    (WindSpeed850 != Constants::MissingData) &&
    (WindDir500 != Constants::MissingData) &&
    (WindDir850 != Constants::MissingData) )
  {
    double tempTerm = 0;
    if (TotalTotals > 49) { tempTerm = 20. * (TotalTotals - 49); }

    SWEAT = (12.0 * TempC_850) + tempTerm + (2. * WindSpeed850 * 1.94 )
      + (WindSpeed500 * 1.94)
      + ( 125. * (sin((WindDir500 - WindDir850) * 3.14159 / 180.) + 0.2));
    //    std::cout << "T850, term1, WS850, WS500, WD850, WD500"
    //      << TempC_850 << " " << tempTerm << " " << WindSpeed850 << " "
    //      << WindSpeed500 << " " << WindDir850 << " " << WindDir500
    //      << " SWEAT = " << SWEAT << "\n";
  }
} // nseProfile::getJunkIndices

double
nseProfile::getMaxThetaEBelowPressureLevel(double topPres, double& heightOfMax)
{
  // return the maximum thetaE below the given pressure level

  double ThetaE = sfcPoint.getThetaE();

  heightOfMax = sfcPoint.getHeight();

  for (size_t i = nearSurfaceGridPoint; i < gridPoints.size(); ++i) {
    if ((gridPoints[i].getThetaE() > ThetaE) &&
      (gridPoints[i].getPressure() >= topPres) )
    {
      ThetaE      = gridPoints[i].getThetaE();
      heightOfMax = gridPoints[i].getHeight();
    }
  }
  return ThetaE;
}

double
nseProfile::getLapseRateInHeightLayer(double botHeight,
  double                                     topHeight)
{
  // these heights are MSL, not AGL

  double botPres = Constants::MissingData;
  double topPres = Constants::MissingData;

  for (size_t i = 0; i < (gridPoints.size() - 1); ++i) {
    if ((botHeight >= gridPoints[i].getHeight()) &&
      (botHeight < gridPoints[i + 1].getHeight()) )
    {
      // FIXME: needs logrithmic interpolation, here!
      botPres = interpVal(gridPoints[i].getPressure(),
          gridPoints[i + 1].getPressure(),
          gridPoints[i].getHeight(),
          gridPoints[i + 1].getHeight(),
          botHeight);
    }
    if ((topHeight >= gridPoints[i].getHeight()) &&
      (topHeight < gridPoints[i + 1].getHeight()) )
    {
      // FIXME: needs logrithmic interpolation, here!
      topPres = interpVal(gridPoints[i].getPressure(),
          gridPoints[i + 1].getPressure(),
          gridPoints[i].getHeight(),
          gridPoints[i + 1].getHeight(),
          topHeight);
    }
  }
  if ((botPres != Constants::MissingData) &&
    (topPres != Constants::MissingData) )
  {
    return getLapseRate(botPres, topPres);
  } else { return Constants::MissingData; }
} // nseProfile::getLapseRateInHeightLayer

double
nseProfile::getLapseRate(double botPres, double topPres)
{
  // return the average lapse rate for a layer.  If botPres == 9999,
  // then use the surface parcel for the bottom

  double sum = 0;

  double botHeight = Constants::MissingData;
  double topHeight = Constants::MissingData;

  // find the bottom and top heights
  for (size_t i = nearSurfaceGridPoint; i < (gridPoints.size() - 1); ++i) {
    if ((botPres <= gridPoints[i].getPressure()) &&
      (botPres > gridPoints[i + 1].getPressure()) &&
      (botPres != 9999) )
    {
      botHeight = interpVal(gridPoints[i].getHeight(),
          gridPoints[i + 1].getHeight(),
          gridPoints[i].getPressure(),
          gridPoints[i + 1].getPressure(),
          botPres);
    }
    if ((topPres <= gridPoints[i].getPressure()) &&
      (topPres > gridPoints[i + 1].getPressure()) )
    {
      topHeight = interpVal(gridPoints[i].getHeight(),
          gridPoints[i + 1].getHeight(),
          gridPoints[i].getPressure(),
          gridPoints[i + 1].getPressure(),
          topPres);
    }
  }
  bool useSurface = false;

  if (botPres == 9999) {
    botPres    = sfcPoint.getPressure();
    botHeight  = sfcPoint.getHeight();
    useSurface = true;
  }
  if ((botPres == Constants::MissingData) ||
    (botHeight == Constants::MissingData) ||
    (topPres == Constants::MissingData) ||
    (topHeight == Constants::MissingData) )
  {
    return Constants::MissingData;
  }

  double totalDepth = topHeight - botHeight;

  // if we are using the surface point, then add the information for the
  // first layer

  if (useSurface) {
    double deltaTemp = gridPoints[nearSurfaceGridPoint].getTempC()
      - sfcPoint.getTempC();
    // double depth = gridPoints[nearSurfaceGridPoint].getHeight()
    //  - sfcPoint.getHeight();
    sum += deltaTemp / totalDepth;
  }

  // FIXME:  there might be a problem, here, if the bottom pressure is
  // not on a grid point or on a surface (that is, we need to interpolate
  // to get the lower small "chunk" of the profile -- as is, it gets
  // dropped).

  // now do the calculations for all the intervening levels

  for (size_t i = nearSurfaceGridPoint; i < (gridPoints.size() - 1); ++i) {
    if ((gridPoints[i].getPressure() <= botPres) &&
      (gridPoints[i + 1].getPressure() >= topPres) )
    {
      double deltaTemp = gridPoints[i + 1].getTempC() - gridPoints[i].getTempC();
      // double depth = gridPoints[i+1].getHeight() - gridPoints[i].getHeight();
      // sum += ((depth / totalDepth) * (deltaTemp / depth ));
      sum += (deltaTemp / totalDepth );
    }
    // the last layer, but only if the topPres falls between two grid points
    // (otherwise it is already taken care of)
    if ((gridPoints[i].getPressure() <= botPres) &&
      (gridPoints[i].getPressure() > topPres) &&
      (gridPoints[i + 1].getPressure() < topPres) )
    {
      double topTempC = interpVal(gridPoints[i].getTempC(),
          gridPoints[i + 1].getTempC(),
          gridPoints[i].getHeight(),
          gridPoints[i + 1].getHeight(),
          topHeight);
      double deltaTemp = topTempC - gridPoints[i].getTempC();
      // double depth = topHeight - gridPoints[i].getHeight();
      sum += (deltaTemp / totalDepth );
    }
  }

  sum = sum * 1000; // convert to degreeC per km
  return sum;
} // nseProfile::getLapseRate

double
nseProfile::NormalizedCAPE(double CAPE,
  double ELHeight, double LCLHeight)
{
  if ((CAPE != Constants::MissingData) &&
    (ELHeight != Constants::MissingData) &&
    (LCLHeight != Constants::MissingData) )
  {
    return (CAPE / (ELHeight - LCLHeight ) );
  } else { return Constants::MissingData; }
}

double
nseProfile::Percentage(double partialCAPE, double CAPE)
{
  if ((partialCAPE == Constants::MissingData) ||
    (CAPE == Constants::MissingData) )
  {
    return Constants::MissingData;
  } else {
    if (CAPE != 0) {
      return (partialCAPE / CAPE );
    } else { return Constants::MissingData; }
  }
}

double
nseProfile::Difference(double first, double second)
{
  if ((first == Constants::MissingData) ||
    (second == Constants::MissingData) )
  {
    return Constants::MissingData;
  }

  return (first - second);
}

double
nseProfile::MeanShear(double depthAGL)
{
  if (depthAGL == Constants::MissingData) {
    return Constants::MissingData;
  }
  double HL = HodographLength(depthAGL);

  if (HL == Constants::MissingData) {
    return Constants::MissingData;
  }
  double shear = HL / (depthAGL / 1000);

  return shear; // in m/s/km
}

double
nseProfile::HodographLength(const double depthAGL)
{
  // determine the length of the hodograph
  //
  if ((sfcPoint.getUWind() == Constants::MissingData) ||
    (sfcPoint.getVWind() == Constants::MissingData) ||
    (sfcPoint.getHeight() == Constants::MissingData) ||
    (gridPoints.size() == 0))
  {
    return Constants::MissingData;
  }

  double length = 0;

  // initialize with surface-to-first-point value

  if ((gridPoints[nearSurfaceGridPoint].getUWind() != Constants::MissingData) &&
    (gridPoints[nearSurfaceGridPoint].getVWind() != Constants::MissingData) &&
    (gridPoints[nearSurfaceGridPoint].getHeight() != Constants::MissingData) &&
    ((gridPoints[nearSurfaceGridPoint].getHeight() - sfcPoint.getHeight())
    <= depthAGL) )
  {
    double uDiff = gridPoints[nearSurfaceGridPoint].getUWind()
      - sfcPoint.getUWind();
    double vDiff = gridPoints[nearSurfaceGridPoint].getVWind()
      - sfcPoint.getVWind();
    length += pow(uDiff * uDiff + vDiff * vDiff, 0.5);
  }

  for (size_t i = nearSurfaceGridPoint; i < (gridPoints.size() - 1); ++i) {
    if (!Constants::isGood(gridPoints[i].getHeight()) ||
      !Constants::isGood(gridPoints[i].getUWind()) ||
      !Constants::isGood(gridPoints[i].getVWind()) ||
      !Constants::isGood(gridPoints[i + 1].getHeight()) ||
      !Constants::isGood(gridPoints[i + 1].getUWind()) ||
      !Constants::isGood(gridPoints[i + 1].getVWind()) )
    {
      continue;
    }

    if ((gridPoints[i].getHeight() - sfcPoint.getHeight()) <= depthAGL) {
      double uTop = gridPoints[i + 1].getUWind();
      double vTop = gridPoints[i + 1].getVWind();
      if ((gridPoints[i + 1].getHeight() - sfcPoint.getHeight()) > depthAGL) {
        uTop = interpVal(gridPoints[i].getUWind(), gridPoints[i + 1].getUWind(),
            gridPoints[i].getHeight(), gridPoints[i + 1].getHeight(),
            sfcPoint.getHeight() + depthAGL);
        vTop = interpVal(gridPoints[i].getVWind(), gridPoints[i + 1].getVWind(),
            gridPoints[i].getHeight(), gridPoints[i + 1].getHeight(),
            sfcPoint.getHeight() + depthAGL);
      }
      double uDiff = uTop - gridPoints[i].getUWind();
      double vDiff = vTop - gridPoints[i].getVWind();
      length += pow(uDiff * uDiff + vDiff * vDiff, 0.5);
    }
  }
  return length;
} // nseProfile::HodographLength

double
nseProfile::HodographCurvature(double depthAGL)
{
  // Determine the curvature of the hodograph curve for this layer.
  //   0 = straight, >0 = cyclonic, <0 = anticyclonic
  //
  //   This is copied basically verbatim (with small changes) from the
  //   original NSE fortran code, which had no documentation or references
  //   to how this parameter is calculated.  So, you are on your own!
  //
  if ((sfcPoint.getUWind() == Constants::MissingData) ||
    (sfcPoint.getVWind() == Constants::MissingData) ||
    (sfcPoint.getHeight() == Constants::MissingData) )
  {
    return Constants::MissingData;
  }

  std::vector<double> du;
  std::vector<double> dv;
  std::vector<double> dz;

  double dz_tmp = gridPoints[nearSurfaceGridPoint].getHeight()
    - sfcPoint.getHeight();

  if ((dz_tmp != 0) &&
    (gridPoints[nearSurfaceGridPoint].getUWind() != Constants::MissingData) &&
    (gridPoints[nearSurfaceGridPoint].getVWind() != Constants::MissingData) )
  {
    double du_tmp = (gridPoints[nearSurfaceGridPoint].getUWind()
      - sfcPoint.getUWind())
      / (gridPoints[nearSurfaceGridPoint].getHeight()
      - sfcPoint.getHeight());
    double dv_tmp = (gridPoints[nearSurfaceGridPoint].getVWind()
      - sfcPoint.getVWind())
      / (gridPoints[nearSurfaceGridPoint].getHeight()
      - sfcPoint.getHeight());
    du.push_back(du_tmp);
    dv.push_back(dv_tmp);
    dz.push_back(dz_tmp);
  }
  //  double sumdu = 0;
  //  double sumdv = 0;
  //  double sumdz = 0;
  //  double sumd2u = 0;
  //  double sumd2v = 0;
  //  double sumd2z = 0;
  double sumCurve = 0;

  for (size_t i = nearSurfaceGridPoint; i < (gridPoints.size() - 1); ++i) {
    if ((gridPoints[i].getHeight() - sfcPoint.getHeight()) < depthAGL) {
      dz_tmp = gridPoints[i + 1].getHeight() - gridPoints[i].getHeight();
      if ((dz_tmp != 0) && (gridPoints[i + 1].getUWind() != Constants::MissingData) &&
        (gridPoints[i + 1].getVWind() != Constants::MissingData) &&
        (gridPoints[i].getUWind() != Constants::MissingData) &&
        (gridPoints[i].getVWind() != Constants::MissingData) )
      {
        double du_tmp = ( gridPoints[i + 1].getUWind()
          - gridPoints[i].getUWind() )
          / dz_tmp;
        double dv_tmp = ( gridPoints[i + 1].getVWind()
          - gridPoints[i].getVWind() )
          / dz_tmp;
        du.push_back(du_tmp);
        dv.push_back(dv_tmp);
        dz.push_back(dz_tmp);
      } else {
        du.push_back(0);
        dv.push_back(0);
        dz.push_back(0);
      }
      if (i > nearSurfaceGridPoint) {
        double dz2 = (gridPoints[i + 1].getHeight()
          + gridPoints[i].getHeight()) / 2
          - (gridPoints[i].getHeight()
          + gridPoints[i - 1].getHeight()) / 2;
        double d2u = 0;
        double d2v = 0;
        if ((dz2 != 0) && (du.size() > 1) && (dv.size() > 1) && (dz.size() > 1)) {
          d2u = ( du[du.size() - 1] - du[du.size() - 2] ) / dz2;
          d2v = ( dv[dv.size() - 1] - dv[dv.size() - 2] ) / dz2;
        }
        //	sumd2u += d2u;
        //	sumd2v += d2v;
        //	sumd2z += d2z;
        double denom = pow(du[du.size() - 1] * du[du.size() - 1]
            + dv[dv.size() - 1] * dv[dv.size() - 1], 1.5);
        if (denom != 0) {
          double curve = -(du[du.size() - 1] * d2v - dv[dv.size() - 1] * d2u )
            / denom;
          sumCurve += curve;
        }
      }
    } else { break; }
  }
  //  for (size_t i = 0; i < du.size(); ++i) sumdu += du[i];
  //  for (size_t i = 0; i < dv.size(); ++i) sumdv += dv[i];
  //  for (size_t i = 0; i < dz.size(); ++i) sumdz += dz[i];
  //  double avg_du = sumdu /  du.size();
  //  double avg_dv = sumdv /  dv.size();
  //  double avg_dz = sumdz /  dz.size();

  if (du.size() > 2) {
    return (sumCurve / du.size() );
    // we leave out a bunch of stuff from the original, here, that
    // doesn't seem to be used for anything...
  } else { return Constants::MissingData; }
} // nseProfile::HodographCurvature

double
nseProfile::getHeightOfMinValue(double presLevel,
  const std::vector<double> &param, const std::vector<double> &height,
  const std::vector<double> &pres, double& minval)
{
  // return the height of the minimum value of the parameter
  // below the given pressure level (for the entire sounding,
  // set presLevel to 100)

  assert(param.size() == height.size());
  assert(param.size() == pres.size() );

  if ((param.size() != height.size()) || (param.size() != pres.size())) {
    minval = Constants::MissingData;
    return Constants::MissingData;
  }

  minval = 9999999999999.0;

  double ht = Constants::MissingData;

  for (size_t i = 0; i < param.size(); ++i) {
    if ((param[i] < minval) && (pres[i] >= presLevel)) {
      minval = param[i];
      ht     = height[i];
    }
  }
  if (minval == 9999999999999.0) { minval = Constants::MissingData; }
  return ht;
}

double
nseProfile::getDewpointDepressionAtPressureLevel(double pres)
{
  // FIXME: assumes that we are on a pressure grid of equal spacing,
  // needs to do interpolation
  for (size_t i = 0; i < gridPoints.size(); ++i) {
    if (gridPoints[i].getPressure() == pres) {
      return (gridPoints[i].getDewPointDepression() );
    }
  }
  return Constants::MissingData;
}

double
nseProfile::getMaxDewpointDepressionInPressureLayer(double botPres,
  double                                                   topPres)
{
  double max = 0;

  for (size_t i = 0; i < gridPoints.size(); ++i) {
    if ((gridPoints[i].getPressure() <= botPres) &&
      (gridPoints[i].getPressure() >= topPres) )
    {
      if (gridPoints[i].getDewPointDepression() > max) {
        max = gridPoints[i].getDewPointDepression();
      }
    }
  }
  return max;
}

double
nseProfile::getWINDEX()
{
  double htOf253 = getHeightOfIsotherm(-20);

  if (htOf253 <= sfcPoint.getHeight() ) { return 0; } else if (htOf253 == Constants::MissingData) {
    return Constants::MissingData;
  } else if (gridPoints.size() == 0) {
    return Constants::MissingData;
  } else if ((sfcPoint.getHeight() == Constants::MissingData) ||
    (sfcPoint.getMixingRatio() == Constants::MissingData) )
  {
    return Constants::MissingData;
  }

  double lapseRate_SfcTo253K = Constants::MissingData;
  double MixingRatioAt253K   = Constants::MissingData;
  double PresAt253K = Constants::MissingData;
  std::vector<double> MR_profile; // MixingRatio profile, used to calc. mean
  std::vector<double> Ht_profile;

  MR_profile.push_back(sfcPoint.getMixingRatio() );
  Ht_profile.push_back(sfcPoint.getHeight() );
  if ((htOf253 > sfcPoint.getHeight()) && (htOf253 < gridPoints[0].getHeight())) {
    MixingRatioAt253K = interpVal(sfcPoint.getMixingRatio(),
        gridPoints[0].getMixingRatio(), sfcPoint.getHeight(),
        gridPoints[0].getHeight(), htOf253);
    PresAt253K = interpVal(sfcPoint.getPressure(),
        gridPoints[0].getPressure(), sfcPoint.getHeight(),
        gridPoints[0].getHeight(), htOf253);
  }
  for (size_t i = nearSurfaceGridPoint; i < (gridPoints.size() - 1); ++i) {
    // find mixing ratio and pressure at htOf253
    if ((htOf253 >= gridPoints[i].getHeight()) &&
      (htOf253 < gridPoints[i + 1].getHeight()) )
    {
      MixingRatioAt253K = interpVal(gridPoints[i].getMixingRatio(),
          gridPoints[i + 1].getMixingRatio(), gridPoints[i].getHeight(),
          gridPoints[i + 1].getHeight(), htOf253);
      // FIXME: log interp?
      PresAt253K = interpVal(gridPoints[i].getPressure(),
          gridPoints[i + 1].getPressure(), gridPoints[i].getHeight(),
          gridPoints[i + 1].getHeight(), htOf253);
    }

    // populate a vector so we can find mean layer values

    MR_profile.push_back(gridPoints[i].getMixingRatio());
    Ht_profile.push_back(gridPoints[i].getHeight());
  }
  assert(MR_profile.size() == Ht_profile.size() );

  lapseRate_SfcTo253K = getLapseRate(9999, PresAt253K);

  double MeanMR_SfcTo1km = getMeanValueLayer(sfcPoint.getHeight(),
      sfcPoint.getHeight() + 1000, MR_profile, Ht_profile);

  double RQ = MeanMR_SfcTo1km / 12.0;

  if (RQ > 1.0) { RQ = 1.0; }

  if ((lapseRate_SfcTo253K == Constants::MissingData) ||
    (MeanMR_SfcTo1km == Constants::MissingData) ||
    (MixingRatioAt253K == Constants::MissingData) ||
    (htOf253 == Constants::MissingData) )
  {
    return Constants::MissingData;
  }
  // lapse rate too small
  if (lapseRate_SfcTo253K > -5.5) { return 0; }

  double term1 = lapseRate_SfcTo253K * lapseRate_SfcTo253K - 30.0
    + MeanMR_SfcTo1km - 2.0 * MixingRatioAt253K;

  if (term1 < 0) { return 0; }

  double windex = 5.0 * pow( ( htOf253 / 1000 * RQ * term1 ), 0.5);

  return windex;
} // nseProfile::getWINDEX

double
nseProfile::ConvectiveTemperature(nsePoint parcel)
{
  // original comments from fortan code follow:

  /*
   * c-------------------------------------------------------------------------
   * c       This subroutine calculates convective temperature from a
   * c       sounding. The procedure is as follows:
   * c
   * c       Given the mixing ratio (kg/kg) of the chosen parcel, I follow
   * c       that mixing ratio line up to a very low pressure (e.g., 200 mb).
   * c       At that point on the sounding, I find the vapor pressure (e) using
   * c       the equation on p.59 of Hess. Given this vapor pressure, I
   * c       calculate the temperature using equation 11 of Bolton (MWR JUL80).
   * c       I then compare this temperature to the temperature of the
   * c       sounding (at that pressure). The first time through (i.e., at
   * c       the very low initial pressure), the temperature of the sounding
   * c       should be lower than the calculated temperature. Since this is
   * c       true, I then add a delta p (the vertical resolution of the model
   * c       grids) to the original pressure level that I chose (i.e., I start
   * c       to work my way towards the ground) and calculate a new vapor
   * c       pressure and temperature (given the same mixing ratio). If this
   * c       temperature is warmer than the sounding at the new pressure
   * c       level, then I add delta p and keep repeating the process until
   * c       the calculated temperature is cooler than the sounding temperature.
   * c       When this happens, I know that I've crossed the temperature
   * c       sounding. I then estimate the pressure (and temperature) at which
   * c       the cross-over occurred. Given this information, I calculate
   * c       the potential temperature at that point. Given the potential
   * c       temperature at the cross-over point and surface pressure, I
   * c       calculate the convective temperature.
   * c
   * c       The "cross-over" point that I discuss is the point at which the
   * c       mixing ratio line of the given parcel intersects the temperature
   * c       curve (i.e., the CCL).
   * c
   * c       Phillip Spencer  NSSL
   * c
   */

  double pres = 200; // starting pressure

  // find the index of the starting pressure

  int topI   = 0;
  int startI = (int) gridPoints.size() - 1;

  if (startI < (int) nearSurfaceGridPoint) { return Constants::MissingData; }

  assert(startI < (int) gridPoints.size() && "Out of range");
  for (int i = startI; i >= (int) nearSurfaceGridPoint; i--) {
    if (i < 0) {
      fLogSevere("index in convective temp calculation < 0!");
      return Constants::MissingData;
    }
    // FIXME: assumes that pres is found exactly -- not interpolated
    if (pres == gridPoints[i].getPressure()) { topI = i; }
  }
  if (topI == 0) { return Constants::MissingData; }

  assert(topI < (int) gridPoints.size() && "Out of range");
  for (int i = topI; i >= (int) nearSurfaceGridPoint; i--) {
    pres = gridPoints[i].getPressure();
    double e = nsePoint::calcVaporPressure(parcel.getMixingRatio(), pres);
    double t = nsePoint::calcTemperatureFromVaporPressure(e);

    if (t <= gridPoints[i].getTempC() ) {
      // in the case that we are cooler than the environment, then we
      // need to locate the point where "t" crossed the environment line

      double tdiff_bot = t - gridPoints[i].getTempC();
      double tdiff_top = t - gridPoints[i + 1].getTempC();
      double t_cross   = interpVal(gridPoints[i].getTempC(),
          gridPoints[i + 1].getTempC(), tdiff_bot, tdiff_top, 0);
      double p_cross = interpVal(gridPoints[i].getPressure(),
          gridPoints[i + 1].getPressure(), tdiff_bot, tdiff_top, 0);
      nsePoint pt(t_cross, 100, p_cross);
      double ct =
        nsePoint::calcTemperatureFromTheta(pt.getTheta(), sfcPoint.getPressure());
      return ct;
    }
  }
  return Constants::MissingData;
} // nseProfile::ConvectiveTemperature

double
nseProfile::PrecipitableWater()
{
  const double gravity = 9.81; // m/s^2

  // return the precipitable water for the sounding

  // bottom layer:

  if ((sfcPoint.getPressure() == Constants::MissingData) ||
    (gridPoints[nearSurfaceGridPoint].getPressure() == Constants::MissingData) ||
    (sfcPoint.getMixingRatio() == Constants::MissingData) ||
    (gridPoints[nearSurfaceGridPoint].getMixingRatio()
    == Constants::MissingData) )
  {
    return Constants::MissingData;
  }


  double deltaP = sfcPoint.getPressure()
    - gridPoints[nearSurfaceGridPoint].getPressure();
  double avgW = (sfcPoint.getMixingRatio()
    + gridPoints[nearSurfaceGridPoint].getMixingRatio()) / 2;

  double pw = (avgW / gravity) * (deltaP * 100);

  for (size_t i = nearSurfaceGridPoint; i < (gridPoints.size() - 1); ++i) {
    if ((gridPoints[i].getPressure() == Constants::MissingData) ||
      (gridPoints[i + 1].getPressure() == Constants::MissingData) ||
      (gridPoints[i].getMixingRatio() == Constants::MissingData) ||
      (gridPoints[i + 1].getMixingRatio() == Constants::MissingData) )
    {
      return Constants::MissingData;
    }
    deltaP = gridPoints[i].getPressure() - gridPoints[i + 1].getPressure();
    avgW   = (gridPoints[i].getMixingRatio() + gridPoints[i + 1].getMixingRatio()) / 2;
    pw    += ((avgW / gravity) * (deltaP * 100));
  }
  pw = pw / 10000; // cm
  return pw;
} // nseProfile::PrecipitableWater

double
nseProfile::VorticityGenerationPotential(double CAPE, double depth)
{
  // depth (m AGL)
  // CAPE = your CAPE of choice
  if ((depth == 0) || !Constants::isGood(CAPE)) {
    return Constants::MissingData;
  }
  double hodoLength = HodographLength(depth);

  if (!Constants::isGood(hodoLength) || (CAPE < 0)) {
    return Constants::MissingData;
  }
  double vgp = (hodoLength * pow(CAPE, 0.5)) / depth;

  return vgp;
}

double
nseProfile::getVerticalVelocityAtPressureLevel(double pres)
{
  for (size_t i = 0; i < gridPoints.size(); ++i) {
    if (pres == gridPoints[i].getPressure()) {
      return gridPoints[i].getVerticalVelocity();
    }
  }
  return Constants::MissingData;
}

// return an interpolated value for the target level
//
double
interpVal(double bot, double top, double botval,
  double topval, double targetval)
{
  if ((bot == Constants::MissingData) ||
    (top == Constants::MissingData) ||
    (botval == Constants::MissingData) ||
    (topval == Constants::MissingData) ||
    (targetval == Constants::MissingData) )
  {
    return Constants::MissingData;
  }

  double ratio  = (botval - targetval) / (botval - topval);
  double target = (bot + (ratio * (top - bot)));

  return target;
}

/*
 *   $temp1 = ($tmpk[$i] - 253)   / ($tmpk[$i] - $tmpk[$i+1]);
 *   $h253 = int ($hght[$i] + ($temp1 * ($hght[$i+1] - $hght[$i])));
 *   $h253 /= 1000;
 *   $found253 = 1;
 */
