#include "rNetcdfRadialSet.h"

#include "rRadialSet.h"
#include "rError.h"
#include "rUnit.h"
#include "rConstants.h"
#include "rProcessTimer.h"

#include <netcdf.h>

#include <rDataGrid.h> // macros

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
    // Stock global attributes
    LLH location;
    Time time;
    std::vector<std::string> all_attr;
    SentinelDouble FILE_MISSING_DATA, FILE_RANGE_FOLDED;
    IONetcdf::getGlobalAttr(ncid,
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
    NETCDF(IONetcdf::getAtt(ncid, "Elevation", &elev));
    const double elev_angle = elev;

    // Use global range to first gate if we have it
    Length rangeToFirstGate = Length();
    float tempFloat;
    int retval = IONetcdf::getAtt(ncid, "RangeToFirstGate", &tempFloat);

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

    IONetcdf::readDimensionInfo(ncid,
      aTypeName.c_str(), &data_var, &data_num_dims,
      "Gate", &gate_dim, &num_gates,
      "Azimuth", &az_dim, &num_radials);

    // -----------------------------------------------------------------------
    // Create a new radial set object
    std::shared_ptr<RadialSet> radialSetSP = std::make_shared<RadialSet>(location, time,
        rangeToFirstGate);
    RadialSet& radialSet = *radialSetSP;
    radialSet.resize(num_radials, num_gates); // All added will use dimension size now
    radialSet.setTypeName(aTypeName);
    radialSet.setElevation(elev_angle);

    // -----------------------------------------------------------------------
    // Gather the variable info
    int ndimsp, nvarsp, nattsp, unlimdimidp;
    NETCDF(nc_inq(ncid, &ndimsp, &nvarsp, &nattsp, &unlimdimidp));

    char cname[1000];
    nc_type xtypep;
    int ndimsp2, dimidsp, nattsp2;

    // For each variable in the netcdf file...
    for (int i = 0; i < nvarsp; ++i) {
      // Get variable info
      NETCDF(nc_inq_var(ncid, i, cname, &xtypep, &ndimsp2, &dimidsp, &nattsp2));
      std::string name = std::string(cname);

      // Hunting each supported grid type
      int data_var1 = -1;
      NETCDF(nc_inq_varid(ncid, name.c_str(), &data_var1));

      // Read units
      std::string units = "Dimensionless";
      retval = IONetcdf::getAtt(ncid, "Units", units, data_var1);
      if (retval != NC_NOERR) {
        retval = IONetcdf::getAtt(ncid, "units", units, data_var1);
      }

      if (ndimsp2 == 1) { // 1D float
        if (xtypep == NC_FLOAT) {
          auto data1DF = radialSet.addFloat1D(name, units, num_radials);
          NETCDF(nc_get_var_float(ncid, data_var1, data1DF->data()));
        } else if (xtypep == NC_INT) {
          auto data1DI = radialSet.addInt1D(name, units, num_radials);
          NETCDF(nc_get_var_int(ncid, data_var1, data1DI->data()));
        }
      } else if (ndimsp2 == 2) { // 2D stuff
        if (xtypep == NC_FLOAT) {
          bool primary = false;
          if (aTypeName == name) { // Typename matches primary grid
            name    = "primary";
            primary = true;
          }
          auto data2DF = radialSet.addFloat2D(name, units, num_radials, num_gates);

          const bool sparse = (data_num_dims < 2);
          if (sparse && primary) { // Only allow primary to be old sparse format.
            IONetcdf::readSparse2D(ncid, data_var1, num_radials, num_gates,
              FILE_MISSING_DATA, FILE_RANGE_FOLDED, *data2DF);
          } else {
            // We use a grid per moment
            const size_t start2[] = { 0, 0 };
            const size_t count2[] = { num_radials, num_gates };

            NETCDF(nc_get_vara_float(ncid, data_var1, start2, count2,
              data2DF->data()));
            // Do we need this?  FIXME: should take any grid right?
            radialSet.replaceMissing(data2DF, FILE_MISSING_DATA, FILE_RANGE_FOLDED);
          }
        }
      }
    }
    // -----------------------------------------------------------------------
    IONetcdf::readUnitValueList(ncid, radialSet); // Can read now with value
                                                  // object...
    return (radialSetSP);
  } catch (NetcdfException& ex) {
    LogSevere("Netcdf read error with radial set: " << ex.getNetcdfStr() << "\n");
    return (0);
  }
  return (0);
} // NetcdfRadialSet::read

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
    // Typename of primary 2D float grid
    const std::string typeName = radialSet.getTypeName();

    // ------------------------------------------------------------
    // DIMENSIONS
    //
    const size_t num_radials = radialSet.getNumRadials();
    const size_t num_gates = radialSet.getNumGates();
    int az_dim, rn_dim;
    NETCDF(nc_def_dim(ncid, "Azimuth", num_radials, &az_dim));
    NETCDF(nc_def_dim(ncid, "Gate", num_gates, &rn_dim));

    // ------------------------------------------------------------
    // VARIABLES
    //
    DataGrid& grid = dynamic_cast<DataGrid&>(radialSet);
    auto list      = grid.getArrays();

    // gotta be careful to write in same order as declare...
    std::vector<int> datavars;
    for (auto l:list) {
      auto theType  = l->getName();
      auto theUnits = l->getUnits();
      // Primary data is the data type of the file
      if (theType == "primary") {
        theType = typeName;
      }
      int var        = -1;
      const size_t s = l->getDims().size();

      // FIXME: more general method, pass dimensions/type?
      // Also we're writing int as float here..no type yet
      if (s == 2) {
        NETCDF(IONetcdf::addVar2D(ncid, theType.c_str(),
          theUnits.c_str(),
          NC_FLOAT, az_dim, rn_dim, &var));
      } else if (s == 1) {
        NETCDF(IONetcdf::addVar1D(ncid, theType.c_str(),
          theUnits.c_str(),
          NC_FLOAT, az_dim, &var));
      }

      datavars.push_back(var);
    }
    // Declare 1D int variables
    //  int  radialtimevar;
    //  NETCDF(IONetcdf::addVar1D(ncid, "RadialTime", "Milliseconds", NC_INT,
    //    az_dim, &radialtimevar));

    // ------------------------------------------------------------
    // GLOBAL ATTRIBUTES
    //
    const std::string dataType = "RadialSet";
    if (!IONetcdf::addGlobalAttr(ncid, radialSet, dataType)) {
      return (false);
    }

    // Nyquist
    float default_nyquist = Constants::MissingData;
    radialSet.getDataAttributeFloat("Nyquist_Vel",
      default_nyquist,
      "MetersPerSecond");

    const double elevDegrees = radialSet.getElevation();
    const double firstGateM  = radialSet.getDistanceToFirstGate().meters();
    NETCDF(IONetcdf::addAtt(
        ncid,
        "Elevation",
        elevDegrees));
    NETCDF(IONetcdf::addAtt(ncid, "ElevationUnits", "Degrees"));
    NETCDF(IONetcdf::addAtt(ncid, "RangeToFirstGate", firstGateM));
    NETCDF(IONetcdf::addAtt(ncid, "RangeToFirstGateUnits", "Meters"));
    NETCDF(IONetcdf::addAtt(ncid, "MissingData", missing));
    NETCDF(IONetcdf::addAtt(ncid, "RangeFolded", rangeFolded));

    // Non netcdf-4/hdf5 require separation between define and data...
    // netcdf-4 doesn't care though
    NETCDF(nc_enddef(ncid));

    // Write pass using datavars
    const size_t start2[] = { 0, 0 };
    const size_t count2[] = { num_radials, num_gates };
    size_t count = 0;
    for (auto l:list) {
      const size_t s = l->getDims().size();

      if (s == 2) {
        auto theData = l->get<RAPIO_2DF>();
        NETCDF(nc_put_vara_float(ncid, datavars[count], start2, count2,
          theData->data()));
      } else if (s == 1) {
        auto theData = l->get<RAPIO_1DF>();
        NETCDF(nc_put_var_float(ncid, datavars[count], theData->data()));
      }
      count++;
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

  // float radial_time        = 1000.0; // Add a second to radial time for each
  //                                   // radial...

  std::string nyq_unit("MetersPerSecond");

  std::shared_ptr<RadialSet> radialSetSP = std::make_shared<RadialSet>(location, time, Length());
  RadialSet& radialSet = *radialSetSP;

  radialSet.setTypeName("Reflectivity");
  radialSet.setElevation(elev_angle);

  // Allow in-line vector storage to work and not crash
  radialSet.resize(num_radials, num_gates);
  radialSet.setNyquistVelocityUnit(nyq_unit);

  auto azimuths   = radialSet.getFloat1D("Azimuth");
  auto beamwidths = radialSet.getFloat1D("BeamWidth");
  auto gatewidths = radialSet.getFloat1D("GateWidth");

  auto data = radialSet.getFloat2D("primary");
  for (size_t i = 0; i < num_radials; ++i) {
    float start_az = i; // Each degree
    (*azimuths)[i]   = start_az;
    (*beamwidths)[i] = beam_width;
    (*gatewidths)[i] = gate_width;
    for (size_t j = 0; j < num_gates; ++j) {
      #ifdef BOOST_ARRAY
      (*data)[i][j] = i;
      #else
      (*data).set(i, j, i);
      #endif
    }
  }

  radialSet.setDataAttributeValue("Unit", "dimensionless", "MetersPerSecond");
  return (radialSetSP);
} // NetcdfRadialSet::getTestObject
