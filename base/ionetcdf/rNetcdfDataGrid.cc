#include "rNetcdfDataGrid.h"

#include "rDataGrid.h"
#include "rError.h"
#include "rUnit.h"
#include "rConstants.h"
#include "rProcessTimer.h"

#include "rSignals.h"
#include <netcdf.h>

using namespace rapio;
using namespace std;

NetcdfDataGrid::~NetcdfDataGrid()
{ }

void
NetcdfDataGrid::introduceSelf()
{
  std::shared_ptr<NetcdfType> io = std::make_shared<NetcdfDataGrid>();

  IONetcdf::introduce("DataGrid", io);
}

std::shared_ptr<DataType>
NetcdfDataGrid::read(const int ncid, const URL& loc)
{
  ProcessTimer("Reading general netcdf data file\n");

  // Generic make DataGrid type
  std::shared_ptr<DataGrid> dataGridSP = std::make_shared<DataGrid>();
  if (readDataGrid(ncid, dataGridSP, loc)) {
    return dataGridSP;
  } else {
    return nullptr;
  }
}

bool
NetcdfDataGrid::readDataGrid(const int ncid, std::shared_ptr<DataGrid> dataGridSP, const URL& loc)
{
  try {
    DataGrid& dataGrid = *dataGridSP;

    // ------------------------------------------------------------
    // GLOBAL ATTRIBUTES
    // Do this first to allow subclasses to check/validate format
    IONetcdf::getAttributes(ncid, NC_GLOBAL, dataGrid.getGlobalAttributes());

    // DataGrid doesn't care, but other classes might be pickier
    if (!dataGrid.initFromGlobalAttributes()) {
      LogSevere("Bad or missing global attributes in data.\n");
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

    // ------------------------------------------------------------
    // Look for pixel dimension and assume this is a WDSS2 sparse netcdf
    // FIXME: It 'might' be better to read generically and then
    // 'unsparse' the data afterwards with the DataGrid
    bool sparse = false;
    for (size_t i = 0; i < s; i++) {
      if (dimnames[i] == "pixel") {
        sparse = true;
        // Remove pixel dimension
        dimids.erase(dimids.begin() + i);
        dimnames.erase(dimnames.begin() + i);
        dimsizes.erase(dimsizes.begin() + i);
        // dimnames.erase(&dimnames[i]);
        // dimsizes.erase(&dimsizes[i]);
        break;
      }
    }

    // Declare dimensions in data structure
    dataGridSP->declareDims(dimsizes, dimnames);
    auto dims = dataGridSP->getDims();

    // ------------------------------------------------------------
    // VARIABLES (DataArrays)
    //
    // Sparse WDSS2:
    // pixel dimension exists
    // pixel_x, pixel_y, pixel_count variables
    int varcount = -1;
    NETCDF(nc_inq_nvars(ncid, &varcount));
    const size_t vsize = varcount > 0 ? varcount : 0;
    for (size_t i = 0; i < vsize; ++i) {
      // ... get name
      char name_in[NC_MAX_NAME + 1];
      std::string name;
      NETCDF(nc_inq_varname(ncid, i, &name_in[0]));
      name = std::string(name_in);

      // If marked sparse
      if (sparse) {
        if (name == "pixel_x") {
          continue;
        }
        if (name == "pixel_y") {
          continue;
        }
        if (name == "pixel_count") {
          continue;
        }
      }

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
        arrayName = "primary";
      }

      // If there's a units attribute it will replace later in general pull
      const std::string units = "dimensionless";

      // If we had pixel dimension, check the sparse
      if (sparse) {
        if (name == aTypeName) {
          if ((ndimsp2 == 1)and(xtypep == NC_FLOAT)) {
            // SPARSE DATA READ
            // Expand it to 2D float array from sparse...
            // We're assuming it's using first two dimensions on 2D sparse...
            auto data2DF = dataGrid.addFloat2D(arrayName, units, { 0, 1 });
            IONetcdf::readSparse2D(ncid, varid, dimsizes[0], dimsizes[1],
              Constants::MissingData, Constants::RangeFolded, *data2DF);
          }
        }
      } else {
        // --------------------------------------------------

        void * data = nullptr;


        // Slowly evolving to more generalized code
        if (ndimsp2 == 1) { // 1D float
          if (xtypep == NC_FLOAT) {
            auto data1DF = dataGrid.addFloat1D(arrayName, units, dimindexes);
            // data = data1DF->data();
            data = data1DF->getRawDataPointer();

            //       NETCDF(nc_get_var_float(ncid, varid, data1DF->data()));
            //          NETCDF(nc_get_var(ncid, varid, data));
          } else if (xtypep == NC_INT) {
            auto data1DI = dataGrid.addInt1D(arrayName, units, dimindexes);
            // data = data1DI->data();
            data = data1DI->getRawDataPointer();

            //       NETCDF(nc_get_var_int(ncid, varid, data1DI->data()));
            //          NETCDF(nc_get_var(ncid, varid, data));
          }
        } else if (ndimsp2 == 2) { // 2D stuff
          if (xtypep == NC_FLOAT) {
            auto data2DF = dataGrid.addFloat2D(arrayName, units, dimindexes);
            // data = data2DF->data();
            data = data2DF->getRawDataPointer();

            //          const size_t start2[] = { 0, 0 };
            //          const size_t count2[] = { dims[dimindexes[0]].size(), dims[dimindexes[1]].size() };
            //          NETCDF(nc_get_vara(ncid, varid, start2, count2, data));

            //       Could we just do this? Seems the same output...
            //             NETCDF(nc_get_var_float(ncid, varid, data2DF->data()));
            //        NETCDF(nc_get_var(ncid, varid, data2DF->data())); yeah we can I think
            // Do we need this?  FIXME: should take any grid right?
            //          dataGrid.replaceMissing(data2DF, FILE_MISSING_DATA, FILE_RANGE_FOLDED);
            //          Only for NSSL data sets
          }
        }
        // Read generally into array
        if (data != nullptr) {
          NETCDF(nc_get_var(ncid, varid, data));
        }
      }

      // This should fill in attributes per array, such as the stored Units in wdssii
      auto theList = dataGridSP->getAttributes(arrayName);
      if (theList != nullptr) {
        IONetcdf::getAttributes(ncid, varid, theList);
      }
    }
    // End generic read 2D?

    return (true);
  } catch (const NetcdfException& ex) {
    LogSevere("Netcdf read error with data grid: " << ex.getNetcdfStr() << "\n");
    return (false);
  }
  return (false);
} // NetcdfDataGrid::read

bool
NetcdfDataGrid::write(int ncid, std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>& keys)
{
  std::shared_ptr<DataGrid> dataGrid = std::dynamic_pointer_cast<DataGrid>(dt);
  auto dataType = dataGrid->getDataType();
  ProcessTimer("Writing " + dataType + " (general writer) to netcdf");

  try {
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
      void * data = l->getRawDataPointer();
      if (data != nullptr) {
        // Woh...mind blown generically write everything
        NETCDF(nc_put_var(ncid, datavars[count], data));
      } else {
        LogSevere("Can't write variable " << l->getName() << " because data is empty!\n");
      }
      count++;
    }
  } catch (const NetcdfException& ex) {
    LogSevere("Netcdf write error with DataGrid: " << ex.getNetcdfStr() << "\n");
    return (false);
  }

  return (true);
} // NetcdfDataGrid::write
