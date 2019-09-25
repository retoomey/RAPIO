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
  std::shared_ptr<DataType> newWay = read(ncid, params);
  return (newWay);
}

std::shared_ptr<DataType>
NetcdfLatLonGrid::read(const int ncid, const vector<string>& params)
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
    float lat_spacing, lon_spacing;                                 //
    NETCDF(IONetcdf::getAtt(ncid, "LatGridSpacing", &lat_spacing)); // diff
    NETCDF(IONetcdf::getAtt(ncid, "LonGridSpacing", &lon_spacing)); // diff

    // Read our first two dimensions and sizes (sparse will have three)
    int data_var, data_num_dims, lat_dim, lon_dim;
    size_t num_lats, num_lons;

    IONetcdf::readDimensionInfo(ncid,
      aTypeName.c_str(), &data_var, &data_num_dims,
      "Lat", &lat_dim, &num_lats,
      "Lon", &lon_dim, &num_lons);

    // Create a new lat lon grid object
    std::shared_ptr<LatLonGrid> LatLonGridSP = std::make_shared<LatLonGrid>(

      // stref,
      location, time,
      lat_spacing,
      lon_spacing);

    LatLonGrid& llgrid = *LatLonGridSP;
    llgrid.setTypeName(aTypeName);

    // Preallocate memory in object
    // FIXME: Humm this fills in the memory.  Maybe the reserve(rows, cols)
    // would work...TEST
    llgrid.resize(num_lats, num_lons, 0.0f);

    // Check dimensions of the data array.  Either it's 2D for azimuth/range
    // or it's 1D for sparse array
    const bool sparse = (data_num_dims < 2);

    auto data = llgrid.getFloat2D("primary");
    if (sparse) {
      // LogSevere("LAT LON GRID IS SPARSE!!!\n");
      IONetcdf::readSparse2D(ncid, data_var, num_lats, num_lons,
        FILE_MISSING_DATA, FILE_RANGE_FOLDED, *data);
    } else {
      // LogSevere("LAT LON GRID IS ___NOT___ SPARSE!!!\n");

      // See if we have the compliance stuff unlimited time var...changes how we
      // read in the grid...
      int time_dim;
      int retval = nc_inq_dimid(ncid, "time", &time_dim);

      if (retval != NC_NOERR) { retval = nc_inq_dimid(ncid, "Time", &time_dim); }
      bool haveTimeDim = (retval == NC_NOERR);

      if (haveTimeDim) {
        const size_t start[] = { 0, 0, 0 };               // time, lat, lon
        const size_t count[] = { 1, num_lats, num_lons }; // 1 time, lat_count,
                                                          // lon_count
        NETCDF(nc_get_vara_float(ncid, data_var, start, count,
          data->data()));
      } else {
        const size_t start[] = { 0, 0 };               // lat, lon
        const size_t count[] = { num_lats, num_lons }; // lat_count, lon_count
        NETCDF(nc_get_vara_float(ncid, data_var, start, count,
          data->data()));
      }
    }

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
  ProcessTimer("Writing LatLonGrid");

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
  try {
    // LatLonGrid is a 2D field, so 2 dimensions
    const size_t lon_size = llgrid.getNumLons();

    if (lon_size == 0) {
      LogSevere
        ("NetcdfEncoder: There are no points in this lat-lon-grid.\n");
      return (false);
    }

    // DEFINE ----------------------------------------------------------
    // Declare our dimensions
    int lat_dim, lon_dim, time_dim;
    const size_t lat_size = llgrid.getNumLats(); // used again later
    NETCDF(nc_def_dim(ncid, "Lat", lat_size, &lat_dim));
    NETCDF(nc_def_dim(ncid, "Lon", lon_size, &lon_dim));

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
    std::string type  = llgrid.getTypeName();

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
    NETCDF(IONetcdf::addAtt(ncid, "NumValidRuns", -1, data_var));

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

  llgrid.resize(num_lats, num_lons, 7.0f);

  llgrid.setTypeName("MergedReflectivityQC");

  // RAPIO: Direct access into lat lon grid...
  llgrid.setDataAttributeValue("Unit", "dimensionless", "MetersPerSecond");

  return (llgridsp);
}
