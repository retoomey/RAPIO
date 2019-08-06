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
  std::shared_ptr<NetcdfType> io(new NetcdfRadialSet());

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

    // LogSevere("DIMS: Gate(" << num_gates << "), Azimuth(" << num_radials <<
    // ")\n");

    // Required variables

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
    // LogSevere("Create radialset with " << rangeToFirstGate << " range...\n");
    std::shared_ptr<RadialSet> radialSetSP(new RadialSet(location, time,
      rangeToFirstGate));

    // RadialSet radialSet(stref.location, rangeToFirstGate);
    RadialSet& radialSet = *radialSetSP;

    radialSet.setTypeName(aTypeName);
    radialSet.setElevation(elev_angle);

    // Preallocate memory in object
    //  radialSet.data().reserve (num_radials);

    // ---------------------------------------------------------
    // var1 calls are slow as snails.  Use memory buffer to read all at once
    std::vector<float> az_vals(num_radials);
    NETCDF(nc_get_var_float(ncid, az_var, &az_vals[0]));

    std::vector<float> bw_vals(num_radials, 1.0f);

    if (bw_var > -1) {
      NETCDF(nc_get_var_float(ncid, bw_var, &bw_vals[0]));
    }

    std::vector<float> azspace_vals(num_radials, 1.0f);

    if (azspacing_var > -1) {
      NETCDF(nc_get_var_float(ncid, azspacing_var, &azspace_vals[0]));
    } else {
      // Azimuth spacing defaults to beamwidth if not there
      // and beamwidth is...
      if (bw_var > -1) {
        NETCDF(nc_get_var_float(ncid, bw_var, &azspace_vals[0]));
      }
    }

    std::vector<float> gate_width_vals(num_radials, 1000.0f);

    if (gw_var > -1) {
      NETCDF(nc_get_var_float(ncid, gw_var, &gate_width_vals[0]));
    }

    std::vector<float> radial_time_vals(num_radials, 0.0f);

    if (radialtime_var > -1) {
      NETCDF(nc_get_var_float(ncid, radialtime_var, &radial_time_vals[0]));
    }

    std::vector<float> nyquist_vals(num_radials);

    if (nyq_var > -1) {
      NETCDF(nc_get_var_float(ncid, nyq_var, &nyquist_vals[0]));
    }

    // ---------------------------------------------------------

    for (size_t i = 0; i < num_radials; ++i) {
      // azimuth
      // float start_az;
      // NETCDF(nc_get_var1_float(ncid, az_var, &i, &start_az));
      float& start_az = az_vals[i];

      // beamwidth
      // float beam_width = 1.0f;
      // if (bw_var > -1){
      // NETCDF(nc_get_var1_float(ncid, bw_var, &i, &beam_width));
      // }
      float& beam_width = bw_vals[i];

      // azspacing
      // float azspacing = beam_width;
      // if (azspacing_var > -1){
      // NETCDF(nc_get_var1_float(ncid, azspacing_var, &i, &azspacing));
      // }
      float& azspacing = azspace_vals[i];

      // gatewidth
      // float gate_width = 1000.0f;
      // if (gw_var > -1){
      // NETCDF(nc_get_var1_float(ncid, gw_var, &i, &gate_width));
      // }
      float& gate_width = gate_width_vals[i];

      // radialtime
      float& radial_time = radial_time_vals[i];

      // float radial_time = 0; // no RadialTime, use start time of scan
      // if (radialtime_var > -1){
      // NETCDF(nc_get_var1_float(ncid, radialtime_var, &i, &radial_time));
      // }

      // Now create radial using gathering information
      double startAz = start_az;

      Time tempRT(time); // Start time of
      // radial.
      tempRT += rapio::TimeDuration(radial_time / 1000.0); // RadialTime is in
                                                           // milliseconds
      Radial radial(startAz,
        azspacing,
        elev_angle,
        location,
        tempRT,
        Length::Meters(gate_width),
        beam_width
      );

      // Nyquist velocity
      if (nyq_var > -1) {
        // float nyquist;
        // NETCDF(nc_get_var1_float(ncid, nyq_var, &i, &nyquist));
        float& nyquist = nyquist_vals[i];

        // radial.setAttributeValue("NyquistVelocity",nyquist, nyq_unit );
        radial.setNyquistVelocity(nyquist, nyq_unit);
      }

      // radialSet.data().push_back( radial );
      radial.setGateCount(num_gates);
      radialSet.addRadial(radial); // copies FIXME: Use pointers
    }

    // Check dimensions of the data array.  Either it's 2D for azimuth/range
    // or it's 1D for sparse array
    const bool sparse = (data_num_dims < 2);

    if (sparse) {
      NetcdfUtil::readSparse2D(ncid, data_var, num_radials, num_gates,
        FILE_MISSING_DATA, FILE_RANGE_FOLDED, radialSet);
    } else {
      // I'll just write from my own read function here...
      // ...it's pretty simple and easier to understand I think compared to old
      // one.
      const size_t count[] = { 1, num_gates }; // We're gonna read num_gates of
                                               // floats

      for (size_t i = 0; i < num_radials; ++i) {
        // Read the ENTIRE radial i (read size = 1*num_gates)
        const size_t start[] = { i, 0 }; // Start at az_var i, rn_var 0...
        // NETCDF(nc_get_vara_float(ncid, data_var, start, count,
        // &radialSet[i][0]));
        NETCDF(nc_get_vara_float(ncid, data_var, start, count,
          radialSet.getRadialVector(i)));

        // Replace any missing/range using the file version to our internal.
        for (size_t j = 0; j < num_gates; j++) {
          //  float& v = radialSet[i][j];
          // if (v == FILE_MISSING_DATA){ v = Constants::MissingData; }
          // else if (v == FILE_RANGE_FOLDED){ v = Constants::RangeFolded; }
          float * v = radialSet.getRadialVector(i, j);

          if (*v == FILE_MISSING_DATA) {
            *v = Constants::MissingData;
          } else if (*v == FILE_RANGE_FOLDED) {
            *v = Constants::RangeFolded;
          }
        }
      }
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
NetcdfRadialSet::write(int ncid, const DataType& dt)
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

    // RadialSet is a sparse 2D field which we'll pad for writing.
    size_t num_gates = 0;

    // const size_t num_radials = radialSet.data().size();
    const size_t num_radials = radialSet.size_d();

    for (size_t i = 0; i < num_radials; ++i) {
      // if ( radialSet[i].data().size() > num_gates ){
      //  num_gates = radialSet[i].data().size();
      // }
      if (radialSet.size_d(i) > num_gates) {
        num_gates = radialSet.size_d(i);
      }
    }

    // Make the radial set equal-dimensioned.
    if (num_gates > 0) {
      for (size_t i = 0; i < num_radials; ++i) {
        // while ( radialSet[i].data().size() != num_gates ){
        //  radialSet[i].data().push_back( missing );
        // }
        while (radialSet.size_d(i) != num_gates) {
          // radialSet[i].data().push_back( missing );
          radialSet.addGate(i, missing);
        }
      }
    }

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

    for (size_t i = 0; i < num_radials; ++i) {
      const Radial& r = radialSet.getRadial(i);

      // azimuth
      const float angle = r.getAzimuth();
      NETCDF(nc_put_var1_float(ncid, azvar, &i, &angle));

      // beamwidth
      const float beam_width = r.getPhysicalBeamWidth();
      NETCDF(nc_put_var1_float(ncid, bwvar, &i, &beam_width));

      // azspacing
      const float azspacing = r.getAzimuthalSpacing();
      NETCDF(nc_put_var1_float(ncid, azspacingvar, &i, &azspacing));

      // gatewidth
      const float gate_width = r.getGateWidth().meters();
      NETCDF(nc_put_var1_float(ncid, gwvar, &i, &gate_width));

      // radialtime
      // const TimeDuration time_delta = r.getStartPoint().time - baseTime;
      const TimeDuration time_delta = r.getStartTime() - baseTime;
      const int radial_time         = static_cast<int>(time_delta.milliseconds());
      NETCDF(nc_put_var1_int(ncid, radialtimevar, &i, &radial_time));

      // Nyquist velocity attribute
      string nyq_unit;
      float nyquist_val;

      if (r.getNyquestVelocity(nyquist_val, nyq_unit)) {
        nyquist_val = Unit::value(nyq_unit, "MetersPerSecond",
            nyquist_val);
      } else {
        nyquist_val = default_nyquist;
      }
      NETCDF(nc_put_var1_float(ncid, nyquistvar, &i, &nyquist_val));

      // Write out current radial from data...
      const size_t start[] = { i, 0 };         // az_var, rn_var...
      const size_t count[] = { 1, num_gates }; // 1 az, n gates
      NETCDF(nc_put_vara_float(ncid, datavar, start, count, r.getDataVector()));

      // Write out current radial from any quality...
      if (haveQuality) {
        const Radial& q = (*quality).getRadial(i);
        NETCDF(nc_put_vara_float(ncid, qualityvar, start, count,
          q.getDataVector()));
      }
    }
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
  float radial_time        = 1000.0; // Add a second to radial time for each
                                     // radial...
  float azspacing = 1.0;

  std::string nyq_unit("MetersPerSecond");

  std::shared_ptr<RadialSet> radialSetSP(new RadialSet(location, time, Length()));
  RadialSet& radialSet = *radialSetSP;

  radialSet.setTypeName("Reflectivity");
  radialSet.setElevation(elev_angle);

  // radialSet.data().reserve (num_radials);
  for (size_t i = 0; i < num_radials; ++i) {
    float start_az = i; // Each degree
    // Now create radial using gathering information
    double startAz = start_az;

    Time tempRT(time); // Start time of
    // radial.
    tempRT += rapio::TimeDuration(radial_time / 1000.0); // RadialTime is in
                                                         // milliseconds

    Radial radial(startAz,
      azspacing,
      elev_angle,
      location,
      tempRT,
      Length::Meters(gate_width),
      beam_width
    );

    radial.setNyquistVelocity(0.6, nyq_unit);
    radialSet.addRadial(radial);

    for (size_t j = 0; j < num_gates; ++j) {
      radialSet.addGate(i, j);
    }
  }

  /* for (size_t i=0; i < num_radials; ++i){
   * for (size_t j=0; j < num_gates; ++j){
   *   float& v = radialSet[i][j];
   *   v = j;
   * }
   * }
   */
  radialSet.setDataAttributeValue("Unit", "dimensionless", "MetersPerSecond");
  return (radialSetSP);
} // NetcdfRadialSet::getTestObject
