#include "rNetcdfLatLonGrid.h"

#include "rLatLonGrid.h"
#include "rError.h"
#include "rConstants.h"
#include "rProcessTimer.h"

using namespace rapio;
using namespace std;

NetcdfLatLonGrid::~NetcdfLatLonGrid()
{ }

void
NetcdfLatLonGrid::introduceSelf()
{
  std::shared_ptr<NetcdfType> io = std::make_shared<NetcdfLatLonGrid>();
  IONetcdf::introduce("LatLonGrid", io);
  IONetcdf::introduce("SparseLatLonGrid", io);
}

std::shared_ptr<DataType>
NetcdfLatLonGrid::read(const int ncid, const URL& loc,
  const vector<string>& params)
{
  // FIXME: Tons of shared code now with rNetcdfRadialSet
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
    // LatLonGrid only ------------------------------------------------
    float lat_spacing, lon_spacing;                                 //
    NETCDF(IONetcdf::getAtt(ncid, "LatGridSpacing", &lat_spacing)); // diff
    NETCDF(IONetcdf::getAtt(ncid, "LonGridSpacing", &lon_spacing)); // diff
    // End LatLonGrid only ------------------------------------------------

    // Read our first two dimensions and sizes (sparse will have three)
    int data_var, data_num_dims, lat_dim, lon_dim;
    size_t num_lats, num_lons;

    IONetcdf::readDimensionInfo(ncid,
      aTypeName.c_str(), &data_var, &data_num_dims,
      "Lat", &lat_dim, &num_lats,
      "Lon", &lon_dim, &num_lons);

    // -----------------------------------------------------------------------
    // Gather the variable info
    int ndimsp, nvarsp, nattsp, unlimdimidp;
    NETCDF(nc_inq(ncid, &ndimsp, &nvarsp, &nattsp, &unlimdimidp));

    std::vector<int> dimids;
    std::vector<size_t> dimsizes;
    std::vector<std::string> dimnames;
    ndimsp = IONetcdf::getDimensions(ncid, dimids, dimnames, dimsizes);

    // -----------------------------------------------------------------------
    // Create a new lat lon grid object
    std::shared_ptr<LatLonGrid> LatLonGridSP = std::make_shared<LatLonGrid>(
      location, time,
      lat_spacing,
      lon_spacing);
    LatLonGrid& llgrid = *LatLonGridSP;
    DataGrid& dataGrid = *LatLonGridSP;

    // Data Grid generic?
    // dataGrid.declareDims({ num_lats, num_lons });
    dataGrid.declareDims(dimsizes, dimnames); // Gotta get rid of pixel I think
    dataGrid.setTypeName(aTypeName);

    char cname[1000];
    nc_type xtypep;
    int ndimsp2, dimidsp, nattsp2;

    // 2D grid stuff for netcdf
    const size_t start2[] = { 0, 0 };
    const size_t count2[] = { num_lats, num_lons };
    // Time variable
    const size_t start[] = { 0, 0, 0 };               // time, lat, lon
    const size_t count[] = { 1, num_lats, num_lons }; // 1 time, lat_count,
    // lon_count
    // Extra Lat lon grid time variable
    // See if we have the compliance stuff unlimited time var...changes how we
    // read in the grid...
    int time_dim;
    int retval = nc_inq_dimid(ncid, "time", &time_dim);
    if (retval != NC_NOERR) { retval = nc_inq_dimid(ncid, "Time", &time_dim); }
    bool haveTimeDim = (retval == NC_NOERR);
    // ---- end Lat lon grid time variable

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
      // We store data type as primary, not sure we need to do this...
      bool primary = false;
      if (aTypeName == name) {
        name    = "primary";
        primary = true;
      }

      if (ndimsp2 == 1) { // 1D float
        if (xtypep == NC_FLOAT) {
          // If it's the primary grid and ONE dimension, this means
          // we have the old sparse data which is wrapped around pixel dimension
          if (primary) {
            // Expand it to 2D float array from sparse...
            auto data2DF = llgrid.addFloat2D(name, units, { 0, 1 });
            IONetcdf::readSparse2D(ncid, data_var1, num_lats, num_lons,
              FILE_MISSING_DATA, FILE_RANGE_FOLDED, *data2DF);
          } else {
            auto data1DF = llgrid.addFloat1D(name, units, { 0 });
            NETCDF(nc_get_var_float(ncid, data_var1, data1DF->data()));
          }
        } else if (xtypep == NC_INT) {
          // FIXME: we're gonna crash on anything using different dimension than
          // what we're forcing.  Need to handle dimension storage I think.
          if (name == "pixel_count") { // Sparse has it's own dimension
          } else {
            auto data1DI = llgrid.addInt1D(name, units, { 0 });
            NETCDF(nc_get_var_int(ncid, data_var1, data1DI->data()));
          }
        }
      } else if (ndimsp2 == 2) { // 2D stuff
        if (xtypep == NC_FLOAT) {
          auto data2DF = llgrid.addFloat2D(name, units, { 0, 1 });
          if (haveTimeDim) {
            NETCDF(nc_get_vara_float(ncid, data_var, start, count, data2DF->data()));
          } else {
            NETCDF(nc_get_vara_float(ncid, data_var1, start2, count2,
              data2DF->data()));
          }
          // Do we need this?  FIXME: should take any grid right?
          llgrid.replaceMissing(data2DF, FILE_MISSING_DATA, FILE_RANGE_FOLDED);
        }
      }
    }

    // -----------------------------------------------------------------------
    IONetcdf::readUnitValueList(ncid, llgrid); // Can read now with value
                                               // object...

    return (LatLonGridSP);
  } catch (NetcdfException& ex) {
    LogSevere("Netcdf read error with LatLonGrid: " << ex.getNetcdfStr() << "\n");
    return (0);
  }

  return (0);
} // NetcdfLatLonGrid::read

void
NetcdfLatLonGrid::writeCFHeader(
  int   ncid,
  int   lat_dim,
  int   lon_dim,
  int * latvar,
  int * lonvar,
  int * time_dim, // New time dimension
  int * time_var  // New time variable
)
{
  // Add an extra dimension of time iff when in compliance.
  NETCDF(nc_def_dim(ncid, "time", 1, time_dim));

  // ### Set up the Dimension Variables and add them to the ncfile ###
  NETCDF(IONetcdf::addVar1D(ncid, "Lat", "degrees_north", NC_FLOAT, lat_dim,
    latvar));
  NETCDF(IONetcdf::addAtt(ncid, "long_name", "latitude", *latvar));
  NETCDF(IONetcdf::addAtt(ncid, "standard_name", "latitude", *latvar));

  // FIXME :: should call name of dim (broken in old code too)
  // latvar->add_att("_CoordinateAxisType", lat_dim->name() );
  // latvar->add_att("_CoordinateAxisType", "Lat");
  NETCDF(IONetcdf::addVar1D(ncid, "Lon", "degrees_east", NC_FLOAT, lon_dim,
    lonvar));
  NETCDF(IONetcdf::addAtt(ncid, "long_name", "longitude", *lonvar));
  NETCDF(IONetcdf::addAtt(ncid, "standard_name", "longitude", *lonvar));

  // FIXME :: should call name of dim (broken in old code too)
  // lonvar->add_att("_CoordinateAxisType", "Lon");
  NETCDF(IONetcdf::addVar1D(ncid, "time", "seconds since 1970-1-1 0:0:0",
    NC_DOUBLE, *time_dim, time_var));
  NETCDF(IONetcdf::addAtt(ncid, "long_name", "time", *time_var));
  NETCDF(IONetcdf::addAtt(ncid, "calendar", "standard", *time_var));

  // FIXME :: should call name of dim
  NETCDF(IONetcdf::addAtt(ncid, "_CoordinateAxisType", "Time", *time_var));

  // ### Add Conventions Global Attribute :: needed for compliance ###
  NETCDF(IONetcdf::addAtt(ncid, "title", "NMQ Product"));
  NETCDF(IONetcdf::addAtt(ncid, "institution", "NSSL"));
  NETCDF(IONetcdf::addAtt(ncid, "source", "MRMS/WDSS2"));

  // NETCDF(IONetcdf::addAtt(ncid, "history", "..."));
  // NETCDF(IONetcdf::addAtt(ncid, "references", "..."));
  // NETCDF(IONetcdf::addAtt(ncid, "comment", "..."));
  NETCDF(IONetcdf::addAtt(ncid, "Conventions", "CF-1.4"));
} // NetcdfLatLonGrid::writeCFHeader

void
NetcdfLatLonGrid::writeLatLonValues(
  int          ncid,
  const int    lat_var,  // Out lat var
  const int    lon_var,  // Out lon var
  const float  oLatDegs, // Origin Lat degrees
  const float  oLonDegs, // Origin Lon degrees
  const float  dLatDegs, // delta Lat
  const float  dLonDegs, // delta Lon
  const size_t lat_size, // Lat count
  const size_t lon_size  // Lon count
)
{
  float cLat = oLatDegs;

  for (size_t i = 0; i < lat_size; ++i) {
    NETCDF(nc_put_var1_float(ncid, lat_var, &i, &cLat));
    cLat -= dLatDegs; // less cpu
  }

  float cLon = oLonDegs;

  for (size_t i = 0; i < lon_size; ++i) {
    NETCDF(nc_put_var1_float(ncid, lon_var, &i, &cLon));
    cLon += dLonDegs; // less cpu
  }
}

bool
NetcdfLatLonGrid::write(int ncid, const DataType& dt,
  std::shared_ptr<DataFormatSetting> dfs)
{
  const LatLonGrid& llgc = dynamic_cast<const LatLonGrid&>(dt);
  LatLonGrid& llgrid     = const_cast<LatLonGrid&>(llgc);

  return (write(ncid, llgrid, IONetcdf::MISSING_DATA, IONetcdf::RANGE_FOLDED,
         dfs->cdmcompliance, dfs->faacompliance));
}

/** Write routine using c library */
bool
NetcdfLatLonGrid::write(int ncid, LatLonGrid& llgrid,
  const float missing,
  const float rangeFolded,
  const bool cdm_compliance,
  const bool faa_compliance)
{
  ProcessTimer("Writing LatLonGrid");

  try {
    // Typename of primary 2D float grid
    std::string type = llgrid.getTypeName();

    // ------------------------------------------------------------
    // DIMENSIONS
    //
    const size_t lat_size = llgrid.getNumLats(); // used again later
    const size_t lon_size = llgrid.getNumLons();
    int lat_dim, lon_dim, time_dim = -1;
    NETCDF(nc_def_dim(ncid, "Lat", lat_size, &lat_dim));
    NETCDF(nc_def_dim(ncid, "Lon", lon_size, &lon_dim));

    // ------------------------------------------------------------
    // VARIABLES
    //
    // DataGrid& grid = dynamic_cast<DataGrid&>(llgrid);

    int latvar, lonvar, timevar;

    const bool useCF = ((cdm_compliance) || (faa_compliance));

    if (useCF) {
      writeCFHeader(ncid, lat_dim, lon_dim, &latvar, &lonvar, &time_dim,
        &timevar);
    }

    // the main variable name is "Rflctvty" for example
    // The hack here we no longer want..
    // NetcdfDataVariable<LatLonGrid> datavar( llgrid, *ncfile, lat_dim, lon_dim
    // );

    // From constructor of NetcdfDataVariable, the 'meat' which is to either do
    // 2 or 3
    // dimensions depending on compliance
    int data_var;
    std::string units = llgrid.getUnits();

    if (faa_compliance) {
      NETCDF(IONetcdf::addVar3D(ncid, type.c_str(), units.c_str(), NC_FLOAT,
        time_dim, lat_dim, lon_dim, &data_var));
    } else {
      NETCDF(IONetcdf::addVar2D(ncid, type.c_str(), units.c_str(), NC_FLOAT,
        lat_dim, lon_dim, &data_var));
    }

    if ((cdm_compliance) || (faa_compliance)) {
      NETCDF(IONetcdf::addAtt(ncid, "long_name", llgrid.getTypeName().c_str(),
        data_var));
      NETCDF(IONetcdf::addAtt(ncid, "_FillValue", (float) Constants::MissingData,
        data_var));
    }

    // old code always adds this..probably not needed but we will keep to match
    // for now
    // NETCDF(IONetcdf::addAtt(ncid, "NumValidRuns", -1, data_var));

    // End constructor NetcdfDataVariable

    // All the attributes of this file are added here
    if (!IONetcdf::addGlobalAttr(ncid, llgrid, "LatLonGrid")) {
      return (false);
    }

    const float latgrid_spacing = llgrid.getLatSpacing();
    NETCDF(IONetcdf::addAtt(ncid, "LatGridSpacing", (double) latgrid_spacing));

    // ^^^^^ Noticed old code writes as a double even with float precision

    // const float longrid_spacing = llgrid.getGridSpacing().second;
    const float longrid_spacing = llgrid.getLonSpacing();
    NETCDF(IONetcdf::addAtt(ncid, "LonGridSpacing", (double) longrid_spacing));

    NETCDF(IONetcdf::addAtt(ncid, "MissingData", missing));

    NETCDF(IONetcdf::addAtt(ncid, "RangeFolded", rangeFolded));

    // Non netcdf-4/hdf5 require separation between define and data...
    // netcdf-4 doesn't care though
    NETCDF(nc_enddef(ncid));

    // FILL DATA -------------------------------------------------------
    // Now complete the CDM/FAA compliance work
    if (useCF) {
      // ### Get dimension sizes ###
      // These were already declared above and don't change, lol.. yep I checked
      // size_t lat_dimsize = lat_dim->size();
      // size_t lon_dimsize = lon_dim->size();
      // ### Get grid spacing ###
      // These were set in the addGlobalAttr..Just get from the llg.
      const Time aTime     = llgrid.getTime();
      const LLH aLocation  = llgrid.getLocation();
      float latgrid_origin = aLocation.getLatitudeDeg();
      float longrid_origin = aLocation.getLongitudeDeg();

      // Write out array of lat/lon output values
      writeLatLonValues(ncid, latvar, lonvar, latgrid_origin, longrid_origin,
        latgrid_spacing, longrid_spacing, lat_size, lon_size);

      int timestamp = aTime.getSecondsSinceEpoch();
      double times[1];
      times[0] = timestamp; // old code is clipping..should we change this?
                            //  double = int
      NETCDF(nc_put_var_double(ncid, timevar, &times[0]));
    }

    // Start putting data in
    // datavar.writeData( llgrid );
    // I think this is bugged in old code.  With CDM and not FAA we create a
    // time dimension, but then
    // we don't store in it..I matched/compared old and new netcdf but I'm not
    // liking this.
    auto data = llgrid.getFloat2D("primary");
    if (faa_compliance) {
      // Or if we HAVE A time dimension basically.  I think this is bugged in
      // old code
      const size_t start[] = { 0, 0, 0 };               // time, lat, lon
      const size_t count[] = { 1, lat_size, lon_size }; // 1 time, lat_count,
                                                        // lon_count
      NETCDF(nc_put_vara_float(ncid, data_var, start, count,
        data->data()));
    } else {
      const size_t start[] = { 0, 0 };               // lat, lon
      const size_t count[] = { lat_size, lon_size }; // lat_count, lon_count
      NETCDF(nc_put_vara_float(ncid, data_var, start, count,
        data->data()));
    }
  } catch (NetcdfException& ex) {
    LogSevere("Netcdf write error with LatLonGrid: " << ex.getNetcdfStr()
                                                     << "\n");
    return (false);
  }

  return (true);
} // NetcdfLatLonGrid::write

std::shared_ptr<DataType>
NetcdfLatLonGrid::getTestObject(
  LLH    location,
  Time   time,
  size_t objectNumber)
{
  // Ignore object number we only have 1
  const size_t num_lats = 10;
  const size_t num_lons = 20;
  float lat_spacing     = .05;
  float lon_spacing     = .05;

  std::shared_ptr<LatLonGrid> llgridsp = std::make_shared<LatLonGrid>(
    location, time,
    lat_spacing,
    lon_spacing);
  LatLonGrid& llgrid = *llgridsp;

  // llgrid.resize(num_lats, num_lons, 7.0f);
  llgrid.declareDims({ num_lats, num_lons }, { "Lat", "Lon" });

  llgrid.setTypeName("MergedReflectivityQC");

  // RAPIO: Direct access into lat lon grid...
  llgrid.setDataAttributeValue("Unit", "dimensionless", "MetersPerSecond");

  return (llgridsp);
}
