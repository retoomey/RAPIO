#include "rNetcdfRadialSet.h"

#include "rRadialSet.h"
#include "rError.h"
#include "rUnit.h"
#include "rConstants.h"
#include "rNetcdfUtil.h"
#include "rProcessTimer.h"

#include <netcdf.h>

using namespace rapio;
using namespace std;

NetcdfRadialSet::~NetcdfRadialSet()
{ }

void
NetcdfRadialSet::introduceSelf()
{
  std::shared_ptr<NetcdfType> io = std::make_shared<NetcdfRadialSet>();

  // NOTE: We read in polar grids and turn them into full radialsets
  IONetcdf::introduce("RadialSet", io);
  IONetcdf::introduce("PolarGrid", io);
  IONetcdf::introduce("SparseRadialSet", io);
  IONetcdf::introduce("SparsePolarGrid", io);
}

std::shared_ptr<DataType>
NetcdfRadialSet::read(const int ncid, const URL& loc,
  const vector<string>& params)
{
  try {
    int retval;

    // Stock global attributes
    LLH location;
    Time time;
    std::vector<std::string> all_attr;
    SentinelDouble FILE_MISSING_DATA, FILE_RANGE_FOLDED;
    NetcdfUtil::getGlobalAttr(ncid,
      all_attr,
      &location,
      &time,
      &FILE_MISSING_DATA,
      &FILE_RANGE_FOLDED);

    // Pull typename and datatype out...
    const std::string aTypeName = all_attr[IONetcdf::ncTypeName];
    const std::string aDataType = all_attr[IONetcdf::ncDataType];

    // Some other global attributes that are here
    // Get Elevation of the RadialSet
    float elev;
    NETCDF(NetcdfUtil::getAtt(ncid, "Elevation", &elev));
    const double elev_angle = elev;

    // Use global range to first gate if we have it
    Length rangeToFirstGate = Length();
    float tempFloat;
    retval = NetcdfUtil::getAtt(ncid, "RangeToFirstGate", &tempFloat);

    if (retval == NC_NOERR) {
      if ((!isinff(tempFloat)) && (!isnanf(tempFloat))) {
        rangeToFirstGate = Length::Meters(tempFloat);
      } else {
        LogInfo("Invalid RangeToFirstGate attribute. \n");
      }
    } else {
      LogInfo("Missing RangeToFirstGate attribute. \n");
    }

    // Read our first two dimensions and sizes (sparse will have three)
    int data_var, data_num_dims, gate_dim, az_dim;
    size_t num_gates, num_radials;

    NetcdfUtil::readDimensionInfo(ncid,
      aTypeName.c_str(), &data_var, &data_num_dims,
      "Gate", &gate_dim, &num_gates,
      "Azimuth", &az_dim, &num_radials);

    // Required variables

    /*
     * // --------------------------------------------
     * //ALPHA new way. Pull the list
     * //
     * // How to read list without object?
     * // "Azimuth", "BeamWidth", "AzimuthalSpacing", "GateWidth", "Nyquist"
     * // also what if strings in there
     * // So basically there are 'known' fields such as Azimuth...but what
     * // if there's a field of something added?
     * // 1. Read list of names
     * // 2. Remove 'known' ones
     * // 3. What's left, check dimension and add to 1d or 2d ?
     * // 4. Difficult right
     * int ndimsp, nvarsp, nattsp, unlimdimidp;
     * nc_inq(ncid, &ndimsp, &nvarsp, &nattsp , &unlimdimidp);
     * LogSevere("--->VAR COUNT:  "<<nvarsp<<"\n");
     * char name[1000];
     * nc_type xtypep;
     * int ndimsp2, dimidsp, nattsp2;
     * for(int i=0; i < nvarsp; ++i){
     * nc_inq_var(ncid, i, name, &xtypep, &ndimsp2, &dimidsp, &nattsp2);
     * LogSevere(">>>" << name << " " << ndimsp2 << " ," << dimidsp <<"\n");
     * }
     * LogSevere("The typename is " << aTypeName.c_str() << "\n");
     * exit(1);
     * // --------------------------------------------
     */

    // Azimuth variable
    int az_var;
    NETCDF(nc_inq_varid(ncid, "Azimuth", &az_var));

    // Optional variables
    int bw_var = -1;
    nc_inq_varid(ncid, "BeamWidth", &bw_var);
    int azspacing_var = -1;
    nc_inq_varid(ncid, "AzimuthalSpacing", &azspacing_var);
    int gw_var = -1;
    nc_inq_varid(ncid, "GateWidth", &gw_var);
    int radialtime_var = -1;
    nc_inq_varid(ncid, "RadialTime", &radialtime_var);

    int nyq_var = -1;
    nc_inq_varid(ncid, "NyquistVelocity", &nyq_var);
    std::string nyq_unit("MetersPerSecond");

    if (nyq_var > -1) {
      // Ok to fail.  Old code bug: We were looking at "Units" not "units".
      retval = NetcdfUtil::getAtt(ncid, Constants::Units, nyq_unit, nyq_var);
    }
    int quality_var = -1;
    nc_inq_varid(ncid, "DataQuality", &quality_var);

    // Create a new radial set object
    std::shared_ptr<RadialSet> radialSetSP = std::make_shared<RadialSet>(location, time,
        rangeToFirstGate);

    RadialSet& radialSet = *radialSetSP;

    // FIXME: maybe constructor?
    radialSet.setTypeName(aTypeName);
    radialSet.setElevation(elev_angle);
    radialSet.setNyquistVelocityUnit(nyq_unit);
    radialSet.reserveRadials(num_gates, num_radials);

    // ---------------------------------------------------------
    // var1 calls are slow as snails.  Use memory buffer to read all at once

    // FIXME: What is vector not in the RadialSet?  We could add it now
    auto azvector = radialSet.getAzimuthVector();
    NETCDF(nc_get_var_float(ncid, az_var, &(*azvector)[0]));

    if (bw_var > -1) {
      auto bwvector = radialSet.getBeamWidthVector();
      NETCDF(nc_get_var_float(ncid, bw_var, &(*bwvector)[0]));
    }

    if (azspacing_var > -1) {
      auto azsvector = radialSet.getAzimuthSpacingVector();
      NETCDF(nc_get_var_float(ncid, azspacing_var, &(*azsvector)[0]));
    } else {
      // Azimuth spacing defaults to beamwidth if not there
      // and beamwidth is...
      if (bw_var > -1) {
        auto azsvector = radialSet.getAzimuthSpacingVector();
        NETCDF(nc_get_var_float(ncid, bw_var, &(*azsvector)[0]));
      }
    }

    if (gw_var > -1) {
      auto gwvector = radialSet.getGateWidthVector();
      NETCDF(nc_get_var_float(ncid, gw_var, &(*gwvector)[0]));
    }

    if (radialtime_var > -1) {
      auto rtvector = radialSet.getRadialTimeVector();
      NETCDF(nc_get_var_int(ncid, radialtime_var, &(*rtvector)[0]));
    } else {
      // Ok, What are we trying to be done with these times?

      /* FIXME: what do we want?
       * Time tempRT(time); // Start time of radial
       * for (size_t i = 0; i < num_radials; ++i) {
       * // radialtime
       * float& radial_time = radial_time_vals[i];
       * tempRT += rapio::TimeDuration(radial_time / 1000.0); // RadialTime is in
       *                                                    // milliseconds
       * }
       */
    }

    if (nyq_var > -1) {
      auto nqvector = radialSet.getNyquistVector();
      NETCDF(nc_get_var_float(ncid, radialtime_var, &(*nqvector)[0]));
    } else {
      // FIXME: Fill with default.  Humm if stored in attributes shouldn't need
      // a giant vector of it right?
    }

    // ---------------------------------------------------------

    // Check dimensions of the data array.  Either it's 2D for azimuth/range
    // or it's 1D for sparse array
    const bool sparse = (data_num_dims < 2);

    if (sparse) {
      NetcdfUtil::readSparse2D(ncid, data_var, num_radials, num_gates,
        FILE_MISSING_DATA, FILE_RANGE_FOLDED, radialSet);
    } else {
      /** We use a grid per moment */
      const size_t start2[] = { 0, 0 };
      const size_t count2[] = { num_radials, num_gates };
      auto data = radialSet.getFloat2D("primary");
      NETCDF(nc_get_vara_float(ncid, data_var, start2, count2,
        // radialSet.getDataVector()));
        &(*data)[0]));
      // Do we need this?
      radialSet.replaceMissing(FILE_MISSING_DATA, FILE_RANGE_FOLDED);
    }

    NetcdfUtil::readUnitValueList(ncid, radialSet); // Can read now with value
                                                    // object...
    return (radialSetSP);
  } catch (NetcdfException& ex) {
    LogSevere("Netcdf read error with radial set: " << ex.getNetcdfStr() << "\n");
    return (0);
  }
  return (0);
} // NetcdfRadialSet::read

// Shared common write code for PolarGrid and RadialSet.
// This is a bit messy, but it's better to have one copy of code to reduce
// chance of bugs.
// PolarGrid: Full 2D grid in RAM written out..well as 2D grid
// RadialSet: Range length vectors of Radials in RAM and editable.
//     RadialSets are 'padded' when writing to turn them into PolarGrids pretty
// much...
// We always read in and convert to RadialSets...this is so algorithms don't
// have to care
// about PolarGrids unless they explicitly want to store one.  Bleh should be
// subclassed
bool
NetcdfRadialSet::writePolarHeader(int ncid, DataType& data,

  // Inputs
  const size_t num_radials,
  const size_t num_gates,
  const double elevDegrees,
  const double firstGateM,
  const bool wantQuality,
  const bool wantNyquist,
  const bool wantRadialTime,
  const float missing,
  const float rangeFolded,

  // Outputs
  int * az_dim,
  int * rn_dim,
  int * datavar,
  int * qualityvar,
  int * azvar,
  int * bwvar,
  int * azspacingvar,
  int * gwvar,
  int * nyquistvar,
  int * radialtimevar
)
{
  int retval;

  const std::string typeName = data.getTypeName();

  if (num_gates == 0) {
    LogSevere
    (
      "NetcdfEncoder: There are no gates to write in this radialset/polargrid dataset.\n");
    return (false);
  }

  // ------------------------------------------------------------
  // DIMENSIONS
  //
  NETCDF(nc_def_dim(ncid, "Azimuth", num_radials, az_dim));
  NETCDF(nc_def_dim(ncid, "Gate", num_gates, rn_dim));

  // ------------------------------------------------------------
  // VARIABLES
  //
  NETCDF(NetcdfUtil::addVar2D(ncid, typeName.c_str(),
    // Constants::getCODEUnits(data).c_str(),
    data.getUnits().c_str(),
    NC_FLOAT, *az_dim, *rn_dim, datavar));

  if (wantQuality) {
    NETCDF(NetcdfUtil::addVar2D(ncid, "DataQuality", "dimensionless",
      NC_FLOAT, *az_dim, *rn_dim, qualityvar))
  }
  NETCDF(NetcdfUtil::addVar1D(ncid,
    "Azimuth",
    "Degrees",
    NC_FLOAT,
    *az_dim,
    azvar));
  NETCDF(NetcdfUtil::addVar1D(ncid, "BeamWidth", "Degrees", NC_FLOAT, *az_dim,
    bwvar));
  NETCDF(NetcdfUtil::addVar1D(ncid, "AzimuthalSpacing", "Degrees", NC_FLOAT,
    *az_dim, azspacingvar));
  NETCDF(NetcdfUtil::addVar1D(ncid, "GateWidth", "Meters", NC_FLOAT, *az_dim,
    gwvar));

  if (wantRadialTime) {
    NETCDF(NetcdfUtil::addVar1D(ncid, "RadialTime", "Milliseconds", NC_INT,
      *az_dim, radialtimevar));
  }

  if (wantNyquist) {
    NETCDF(NetcdfUtil::addVar1D(ncid, "NyquistVelocity", "MetersPerSecond",
      NC_FLOAT, *az_dim, nyquistvar));
  }

  // ------------------------------------------------------------
  // GLOBAL ATTRIBUTES
  //
  const std::string dataType = "RadialSet"; // Code as RadialSet.  It doesn't
                                            // matter we read in as one

  if (!NetcdfUtil::addGlobalAttr(ncid, data, dataType)) {
    return (false);
  }

  NETCDF(NetcdfUtil::addAtt(
      ncid,
      "Elevation",
      elevDegrees));
  NETCDF(NetcdfUtil::addAtt(ncid, "ElevationUnits", "Degrees"));
  NETCDF(NetcdfUtil::addAtt(ncid, "RangeToFirstGate", firstGateM));
  NETCDF(NetcdfUtil::addAtt(ncid, "RangeToFirstGateUnits", "Meters"));
  NETCDF(NetcdfUtil::addAtt(ncid, "MissingData", missing));
  NETCDF(NetcdfUtil::addAtt(ncid, "RangeFolded", rangeFolded));

  // old netcdf write always added this, I don't think we need it but I'll add
  // for now
  // FIXME: remove this if not used.  PolarGrid never used it.  Extra field
  // doesn't hurt.
  NETCDF(NetcdfUtil::addAtt(ncid, "NumValidRuns", (int) (-1), *datavar));

  return (true);
} // NetcdfRadialSet::writePolarHeader

bool
NetcdfRadialSet::write(int ncid, const DataType& dt,
  std::shared_ptr<DataFormatSetting> dfs)
{
  // FIXME: Maybe just pass a shared_ptr down the tree
  const RadialSet& cradialSet = dynamic_cast<const RadialSet&>(dt);
  RadialSet& radialSet        = const_cast<RadialSet&>(cradialSet);

  return (write(ncid, radialSet, IONetcdf::MISSING_DATA, IONetcdf::RANGE_FOLDED));
}

/** Write routine using c library */
bool
NetcdfRadialSet::write(int ncid, RadialSet& radialSet,
  const float missing, const float rangeFolded)
{
  ProcessTimer("Writing RadialSet");

  try {
    int retval;

    const size_t num_radials = radialSet.getNumRadials();
    const size_t num_gates   = radialSet.getNumGates();

    // Nyquist
    float default_nyquist = Constants::MissingData;

    radialSet.getDataAttributeFloat("Nyquist_Vel",
      default_nyquist,
      "MetersPerSecond");

    // Quality
    bool haveQuality    = radialSet.hasQuality();
    RadialSet * quality = 0;

    if (haveQuality) {
      // quality = dynamic_cast<RadialSet*>(radialSet.getQualityPtr().ptr);
      quality     = (RadialSet *) (radialSet.getQualityPtr().get());
      haveQuality = (quality != nullptr);
    }

    int az_dim, rn_dim;
    int datavar, qualityvar, azvar, bwvar, azspacingvar, gwvar, nyquistvar,
      radialtimevar;
    writePolarHeader(ncid, radialSet,
      num_radials, num_gates,
      radialSet.getElevation(),
      radialSet.getDistanceToFirstGate().meters(),
      haveQuality,
      true, // Nyquist array
      true, // Radial time array
      missing,
      rangeFolded,

      &az_dim, &rn_dim,

      &datavar, &qualityvar,
      &azvar, &bwvar, &azspacingvar, &gwvar,
      &nyquistvar,
      &radialtimevar
    );

    // Non netcdf-4/hdf5 require separation between define and data...
    // netcdf-4 doesn't care though
    NETCDF(nc_enddef(ncid));

    // put the data in to the variables
    // This might be faster to pull as a full array from radialset..though
    // that would have to copy it...
    rapio::Time baseTime = radialSet.getTime();

    // We can write all of them at once now. FIXME: Why not loop generic sets?
    auto azvector = radialSet.getAzimuthVector();
    auto bwvector = radialSet.getBeamWidthVector();
    auto asvector = radialSet.getAzimuthSpacingVector();
    auto gwvector = radialSet.getGateWidthVector();
    auto rtvector = radialSet.getRadialTimeVector();
    NETCDF(nc_put_var_float(ncid, azvar, &(*azvector)[0]));
    NETCDF(nc_put_var_float(ncid, bwvar, &(*bwvector)[0]));
    NETCDF(nc_put_var_float(ncid, azspacingvar, &(*asvector)[0]));
    NETCDF(nc_put_var_float(ncid, gwvar, &(*gwvector)[0]));
    NETCDF(nc_put_var_int(ncid, radialtimevar, &(*rtvector)[0]));

    // FIXME: Nyquist not right yet.  Works for raw copy, not if units changed...

    /*if (radialSet.getNyquestVelocity(nyquist_val, nyq_unit)) {
     * nyquist_val = Unit::value(nyq_unit, "MetersPerSecond",
     *    nyquist_val);
     * } else {
     * nyquist_val = default_nyquist;
     * }
     */
    std::string unit = radialSet.getNyquistVelocityUnit();
    if (unit == "") { // No nyquest was ever set
    } else {
      auto nvector = radialSet.getFloat1D("Nyquist");
      nvector->fill(default_nyquist);
      NETCDF(nc_put_var_float(ncid, nyquistvar, &(*nvector)[0]));
    }

    /** Does it go in correctly? lol */
    const size_t start2[] = { 0, 0 };                   // lat, lon
    const size_t count2[] = { num_radials, num_gates }; // lat_count, lon_count
    // NETCDF(nc_put_vara_float(ncid, datavar, start2, count2,
    //   radialSet.getDataVector()));
    auto data = radialSet.getFloat2D("primary");
    NETCDF(nc_put_vara_float(ncid, datavar, start2, count2,
      &(*data)[0]));

    /*  FIXME: disabling quality.  We _really_ want multiband
     *    // Write out current radial from any quality...
     *    if (haveQuality) {
     *      const Radial& q = (*quality).getRadial(i);
     *      NETCDF(nc_put_vara_float(ncid, qualityvar, start, count,
     *        q.getDataVector()));
     *    }
     */
  } catch (NetcdfException& ex) {
    LogSevere("Netcdf write error with RadialSet: " << ex.getNetcdfStr() << "\n");
    return (false);
  }

  return (true);
} // NetcdfRadialSet::write

std::shared_ptr<DataType>
NetcdfRadialSet::getTestObject(
  LLH    location,
  Time   time,
  size_t objectNumber)
{
  const size_t num_radials = 100;
  const size_t num_gates   = 20;
  const float elev         = 0.50;
  const double elev_angle  = elev;
  const float gate_width   = 1;
  const float beam_width   = 2;
  // float radial_time        = 1000.0; // Add a second to radial time for each
  //                                   // radial...
  float azspacing = 1.0;

  std::string nyq_unit("MetersPerSecond");

  std::shared_ptr<RadialSet> radialSetSP = std::make_shared<RadialSet>(location, time, Length());
  RadialSet& radialSet = *radialSetSP;

  radialSet.setTypeName("Reflectivity");
  radialSet.setElevation(elev_angle);

  // Allow in-line vector storage to work and not crash
  radialSet.reserveRadials(num_gates, num_radials);
  radialSet.setNyquistVelocityUnit(nyq_unit);

  auto azimuths   = radialSet.getFloat1D("Azimuth");
  auto beamwidths = radialSet.getFloat1D("BeamWidth");
  auto azspacings = radialSet.getFloat1D("AzimuthalSpacing");
  auto gatewidths = radialSet.getFloat1D("GateWidth");

  for (size_t i = 0; i < num_radials; ++i) {
    float start_az = i; // Each degree

    azimuths[i]   = start_az;
    beamwidths[i] = beam_width;
    azspacings[i] = azspacing;
    gatewidths[i] = gate_width;
    for (size_t j = 0; j < num_gates; ++j) {
      radialSet.set(i, j, i);
    }
  }

  radialSet.setDataAttributeValue("Unit", "dimensionless", "MetersPerSecond");
  return (radialSetSP);
} // NetcdfRadialSet::getTestObject
