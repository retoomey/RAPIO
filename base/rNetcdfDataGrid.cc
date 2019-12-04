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
NetcdfDataGrid::read(const int ncid, const URL& loc,
  const vector<string>& params)
{
  ProcessTimer("Reading general netcdf data file\n");

  try {
    // Create generic array storage...
    std::shared_ptr<DataGrid> dataGridSP = std::make_shared<DataGrid>();
    DataGrid& dataGrid = *dataGridSP;

    // ------------------------------------------------------------
    // DIMENSIONS
    //
    std::vector<int> dimids;
    std::vector<std::string> dimnames;
    std::vector<size_t> dimsizes;
    IONetcdf::getDimensions(ncid, dimids, dimnames, dimsizes);
    // If 'pixel' declared and nssl datatype, remove dimension.  yay
    dataGridSP->declareDims(dimsizes, dimnames);
    auto dims = dataGridSP->getDims();

    // ------------------------------------------------------------
    // GLOBAL ATTRIBUTES
    //
    IONetcdf::getAttributes(ncid, NC_GLOBAL, &(dataGrid.myAttributes));

    // ------------------------------------------------------------
    // VARIABLES (DataArrays)
    //
    int varcount = -1;
    NETCDF(nc_inq_nvars(ncid, &varcount)); // global?  How to get count from local?
    size_t vsize = varcount > 0 ? varcount : 0;
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
      int dimidsp[NC_MAX_VAR_DIMS]; // This was the bug
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

      std::string units = "NONE"; // FIXME: units part of DataAttributeList right?

      /* for NSSL datatype...
       *    // Presnag the sparse right?
       *    if (name == myTypeName){
       *       if (ndimsp2 == 1) and (xtypep == NC_FLOAT)){
       *          // SPARSE DATA READ
       *          // Expand it to 2D float array from sparse...
       *          auto data2DF = dataGrid.addFloat2D(name, units, { 0, 1 });
       *          IONetcdf::readSparse2D(ncid, data_var1, num_radials, num_gates,
       *            FILE_MISSING_DATA, FILE_RANGE_FOLDED, *data2DF);
       *          // FIXME: STILL NEED DATA ATTRIBUTE right?
       *       }
       *    }
       *    if (name == "pixel_count"){
       *    }
       *    if (name == "pixel_x"){
       *    }
       *    if (name == "pixel_y"){
       *       //ignore this since sparse will find it
       *    }
       */

      void * data = nullptr;

      // Slowly evolving to more generalized code
      if (ndimsp2 == 1) { // 1D float
        if (xtypep == NC_FLOAT) {
          auto data1DF = dataGrid.addFloat1D(name, units, dimindexes);
          data = data1DF->data();

          //       NETCDF(nc_get_var_float(ncid, varid, data1DF->data()));
          //          NETCDF(nc_get_var(ncid, varid, data));
        } else if (xtypep == NC_INT) {
          auto data1DI = dataGrid.addInt1D(name, units, dimindexes);
          data = data1DI->data();

          //       NETCDF(nc_get_var_int(ncid, varid, data1DI->data()));
          //          NETCDF(nc_get_var(ncid, varid, data));
        }
      } else if (ndimsp2 == 2) { // 2D stuff
        if (xtypep == NC_FLOAT) {
          auto data2DF = dataGrid.addFloat2D(name, units, dimindexes);
          data = data2DF->data();

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

      if (data != nullptr) {
        NETCDF(nc_get_var(ncid, varid, data));

        DataAttributeList * theList = dataGridSP->getRawAttributePointer(name);
        if (theList != nullptr) {
          IONetcdf::getAttributes(ncid, varid, theList);
        }
      }

      // -----------------------------------------------------------------------
      //   IONetcdf::readUnitValueList(ncid, dataGrid); // Can read now with value
      // object...
    }
    // End generic read 2D?

    return (dataGridSP);
  } catch (NetcdfException& ex) {
    LogSevere("Netcdf read error with data grid: " << ex.getNetcdfStr() << "\n");
    return (nullptr);
  }
  return (nullptr);
} // NetcdfDataGrid::read

bool
NetcdfDataGrid::write(int ncid, std::shared_ptr<DataType> dt,
  std::shared_ptr<DataFormatSetting> dfs)
{
  ProcessTimer("Writing DataGrid to netcdf");

  std::shared_ptr<DataGrid> dataGrid = std::dynamic_pointer_cast<DataGrid>(dt);
  // const float missing = IONetcdf::MISSING_DATA;
  // const float rangeFolded = IONetcdf::RANGE_FOLDED;

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
    std::vector<int> datavars = IONetcdf::declareGridVars(*dataGrid, "NOTHING", dimvars, ncid);

    // ------------------------------------------------------------
    // GLOBAL ATTRIBUTES
    //
    // auto& test = dataGrid.myAttributes.myAttributes;
    // for(auto i:test){
    //  auto& test = dataGrid.myAttributes.myAttributes;
    for (auto& i:dataGrid->myAttributes) {
      // For each type, write out attr...
      // FIXME: All the basic types
      auto field = i.get<std::string>();
      if (field) {
        NETCDF(IONetcdf::addAtt(ncid, i.getName().c_str(), *field));
        //    LogSevere("GOTZ " << i.getName() << " " << field << "\n");
      } else {
        //    LogSevere("NOGOTZ " << i.getName() << " not there\n");
      }
    }

    // Non netcdf-4/hdf5 require separation between define and data...
    // netcdf-4 doesn't care though
    NETCDF(nc_enddef(ncid));

    // Write pass using datavars
    size_t count = 0;

    // For each array type in the data grid
    auto list = dataGrid->getArrays();
    for (auto l:list) {
      auto index = l->getDimIndexes();
      // const size_t s = l->getDimIndexes().size();
      void * data     = l->getRawDataPointer();
      const int varid = datavars[count];
      if (data != nullptr) {
        // Write output depending on the dimension?

        /*        if (s == 2) {
         *        const size_t start2[] = { 0, 0 };
         *        const size_t count2[] = { dims[index[0]].size(), dims[index[1]].size() };
         *        NETCDF(nc_put_vara(ncid, datavars[count], start2, count2, data));
         *      } else if (s == 1) {
         *        NETCDF(nc_put_var(ncid, datavars[count], data));
         *      }
         */
        // Woh...mind blown.
        NETCDF(nc_put_var(ncid, datavars[count], data));
      } else {
        LogSevere("Can't write variable " << l->getName() << " because data is empty!\n");
      }
      count++;

      // Put attributes for this var...
      // For each type, write out attr...
      // FIXME: All the basic types
      // auto& test = dataGrid.myAttributes.myAttributes;
      DataAttributeList * raw = l->getRawAttributePointer();
      if (raw != nullptr) {
        // auto& test = raw->myAttributes;
        for (auto i:*raw) {
          // For each type, write out attr...
          // FIXME: All the basic types
          auto field = i.get<std::string>();
          if (field) {
            NETCDF(IONetcdf::addAtt(ncid, i.getName().c_str(), *field, varid));
            //    LogSevere("GOTZ " << i.getName() << " " << field << "\n");
          } else {
            //    LogSevere("NOGOTZ " << i.getName() << " not there\n");
          }
        }
      }
    }
  } catch (NetcdfException& ex) {
    LogSevere("Netcdf write error with DataGrid: " << ex.getNetcdfStr() << "\n");
    return (false);
  }

  return (true);
} // NetcdfDataGrid::write
