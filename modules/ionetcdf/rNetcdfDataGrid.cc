#include "rNetcdfDataGrid.h"

#include "rDataGrid.h"
#include "rError.h"
#include "rUnit.h"
#include "rConstants.h"

#include "rSignals.h"
#include <netcdf.h>

using namespace rapio;
using namespace std;

NetcdfDataGrid::~NetcdfDataGrid()
{ }

void
NetcdfDataGrid::introduceSelf(IONetcdf * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<NetcdfDataGrid>();

  owner->introduce("DataGrid", io);
}

std::shared_ptr<DataType>
NetcdfDataGrid::read(std::map<std::string, std::string>& keys,
  std::shared_ptr<DataType>                            dt)
{
  // Generic make DataGrid type
  std::shared_ptr<DataGrid> dataGridSP = std::make_shared<DataGrid>();

  if (readDataGrid(dataGridSP, keys)) {
    return dataGridSP;
  }
  return nullptr;
}

bool
NetcdfDataGrid::readDataGrid(std::shared_ptr<DataGrid> dataGridSP,
  std::map<std::string, std::string>                   & keys)
{
  try {
    DataGrid& dataGrid = *dataGridSP;
    const int ncid     = std::stoi(keys["NETCDF_NCID"]);
    const URL loc      = URL(keys["NETCDF_URL"]);

    // ------------------------------------------------------------
    // GLOBAL ATTRIBUTES
    // Do this first to allow subclasses to check/validate format
    IONetcdf::getAttributes(ncid, NC_GLOBAL, dataGrid.getGlobalAttributes());

    // DataGrid doesn't care, but other classes might be pickier
    if (!dataGrid.initFromGlobalAttributes()) {
      fLogSevere("Bad or missing global attributes in data.");
      return false;
    }
    // TypeName from WDSSII type data
    auto aTypeName = dataGrid.getTypeName();

    // ------------------------------------------------------------
    // DIMENSIONS
    //
    std::vector<int> dimids;
    std::vector<std::string> dimnames;
    std::vector<size_t> dimsizes;
    auto s = IONetcdf::getDimensions(ncid, dimids, dimnames, dimsizes);

    // Declare dimensions in data structure
    dataGridSP->setDims(dimsizes, dimnames);

    auto dims = dataGridSP->getDims();

    // ------------------------------------------------------------
    // VARIABLES (DataArrays)
    //
    int varcount = -1;
    NETCDF(nc_inq_nvars(ncid, &varcount));
    const size_t vsize = varcount > 0 ? varcount : 0;
    for (size_t i = 0; i < vsize; ++i) {
      // ... get name
      char name_in[NC_MAX_NAME + 1];
      std::string name;
      NETCDF(nc_inq_varname(ncid, i, &name_in[0]));
      name = std::string(name_in);

      // Now for this variable with given name
      // ... get number of dimensions
      int varndims = -1;
      NETCDF(nc_inq_varndims(ncid, i, &varndims));

      // ... get the varid (pass in name)
      // This 'should' be the same as i, but we'll check
      int varid = -1;
      NETCDF(nc_inq_varid(ncid, name.c_str(), &varid));

      // Get variable info FIXME: do we need all these functions?
      nc_type xtypep;
      int ndimsp2;
      int dimidsp[NC_MAX_VAR_DIMS];
      int nattsp2;
      NETCDF(nc_inq_var(ncid, i, name_in, &xtypep, &ndimsp2, dimidsp, &nattsp2));

      // Convert dims/sizes to indexes for the variable
      std::vector<int> dimindexesINT;
      dimindexesINT.resize(varndims);
      NETCDF(nc_inq_vardimid(ncid, varid, &dimindexesINT[0]));

      std::vector<size_t> dimindexes; // BLEH
      for (auto& dd:dimindexesINT) {
        dimindexes.push_back(dd);
      }

      // --------------------------------------------------
      // WDSS2, we store the TypeName array as 'primary'
      // I'm not sure we still need to do this.  The issue could
      // happen if TypeName is changed...need to handle that properly
      std::string arrayName = name;
      if (aTypeName == name) {
        // Map to primary in internal structure in case TypeName
        // changes.
        arrayName = Constants::PrimaryDataName;
      }

      // If there's a units attribute it will replace later in general pull
      const std::string units = "dimensionless";

      void * data = nullptr;

      // Read all the arrays we support at the moment

      // Handle the special sparse data MRMS format first...
      bool handled = false;

      // All others, generically try to create array using DataGrid factory
      if (!handled) {
        DataArrayType theDataArrayType;
        if (IONetcdf::netcdfToDataArrayType(xtypep, theDataArrayType)) {
          data    = dataGrid.factoryGetRawDataPointer(arrayName, units, theDataArrayType, dimindexes);
          handled = (data != nullptr);
        }
        if (!handled) {
          fLogSevere("Skipping netcdf array '{}' since type not yet handled.", arrayName);
        }
      }
      if (handled) {
        // Read generally into array
        if (data != nullptr) {
          NETCDF(nc_get_var(ncid, varid, data));
        }

        // This should fill in attributes per array, such as the stored Units in wdssii
        auto theList = dataGridSP->getAttributes(arrayName);
        if (theList != nullptr) {
          IONetcdf::getAttributes(ncid, varid, theList);
        }
      }
    }
    // End generic read 2D?

    return (true);
  } catch (const NetcdfException& ex) {
    fLogSevere("Netcdf read error with data grid: {}", ex.getNetcdfStr());
    return (false);
  }
  return (false);
} // NetcdfDataGrid::read

bool
NetcdfDataGrid::write(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>            & keys)
{
  std::shared_ptr<DataGrid> dataGrid = std::dynamic_pointer_cast<DataGrid>(dt);
  auto dataType = dataGrid->getDataType();

  try {
    const int ncid = std::stoi(keys["NETCDF_NCID"]);

    // ------------------------------------------------------------
    // DIMENSIONS
    //
    auto dims = dataGrid->getDims();
    std::vector<int> dimvars;
    dimvars.resize(dims.size());
    for (size_t i = 0; i < dims.size(); ++i) {
      NETCDF(nc_def_dim(ncid, dims[i].name().c_str(), dims[i].size(), &dimvars[i]));
    }
    // IONetcdf::declareGridDims(dataGrid); // method maybe??
    // dataGrid->declareGridDims()... virtual?
    // Might be better design for subclasses like radial set

    // ------------------------------------------------------------
    // VARIABLES
    //
    auto typeName = dataGrid->getTypeName(); // FIXME: can't routine get it?
    std::vector<int> datavars = IONetcdf::declareGridVars(*dataGrid, typeName, dimvars, ncid);

    // ------------------------------------------------------------
    // GLOBAL ATTRIBUTES
    //
    // NOTE: Currently this will add in WDSS2 global attributes,
    // We might want a flag or something
    dataGrid->updateGlobalAttributes(dataType);
    IONetcdf::setAttributes(ncid, NC_GLOBAL, dataGrid->getGlobalAttributes()); // define

    // ------------------------------------------------------------
    // ARRAYS OF DATA
    //
    auto list = dataGrid->getArrays();

    // For netcdf3 we have to declare attributes of the arrays BEFORE enddef
    size_t count = 0;
    for (auto l:list) {
      // Skip hidden (FIXME: maybe getVisibleArrays to hide all this)
      auto hidden = l->getAttribute<std::string>("RAPIO_HIDDEN");
      if (hidden) { continue; }
      // Put attributes for this var...
      const int varid = datavars[count];
      IONetcdf::setAttributes(ncid, varid, l->getAttributes());
      count++;
    }

    // Non netcdf-4/hdf5 require separation between define and data...
    // netcdf-4 doesn't care though
    NETCDF(nc_enddef(ncid));

    // Now write data into each array in the data grid
    count = 0;
    for (auto l:list) {
      // Skip hidden (FIXME: maybe getVisibleArrays to hide all this)
      auto hidden = l->getAttribute<std::string>("RAPIO_HIDDEN");
      if (hidden) { continue; }
      void * data = l->getRawDataPointer();
      if (data != nullptr) {
        // Woh...mind blown generically write everything
        NETCDF(nc_put_var(ncid, datavars[count], data));
      } else {
        fLogSevere("Can't write variable {} because data is empty!", l->getName());
      }
      count++;
    }
  } catch (const NetcdfException& ex) {
    fLogSevere("Netcdf write error with DataGrid: {}", ex.getNetcdfStr());
    return (false);
  }

  return (true);
} // NetcdfDataGrid::write
