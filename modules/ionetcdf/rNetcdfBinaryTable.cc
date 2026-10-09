#include "rNetcdfBinaryTable.h"

#include "rBinaryTable.h"
#include "rConstants.h"
#include "rStrings.h"
#include "rError.h"
#include "rConstants.h"

#include <algorithm>
#include <cstring>

using namespace rapio;
using namespace std;

namespace {
char *
convert(const std::string& s)
{
  char * pc = new char[s.size() + 1];

  std::strcpy(pc, s.c_str());
  return (pc);
}

template <typename T> bool
validateLength(const std::string& name,
  std::vector<T>                & valid,
  size_t                        aSize)
{
  const size_t realSize = valid.size();

  if (realSize != aSize) {
    fLogSevere("Warning: Binary table returned a row length of {} for colume {} so we are padding/truncating",
      realSize, name);
    valid.resize(aSize);
    return (false);
  }
  return (true);
}
}

NetcdfBinaryTable::~NetcdfBinaryTable()
{ }

void
NetcdfBinaryTable::introduceSelf(IONetcdf * owner)
{
  std::shared_ptr<NetcdfBinaryTable> newOne = std::make_shared<NetcdfBinaryTable>();

  owner->introduce("BinaryTable", newOne);
}

std::shared_ptr<DataType>
NetcdfBinaryTable::readNETCDF(int ncid, IOConfig& keys)
{
  std::shared_ptr<BinaryTable> newOne = std::make_shared<BinaryTable>();

  try {
    // ------------------------------------------------------------
    // GLOBAL ATTRIBUTES
    // Do this first to allow subclasses to check/validate format
    IONetcdf::getAttributes(ncid, NC_GLOBAL, newOne->getGlobalAttributes());

    if (!newOne->initFromGlobalAttributes()) {
      fLogSevere("Bad or missing global attributes in binary table.");
      return (nullptr);
    }

    // ------------------------------------------------------------
    // VARIABLES (columns)
    //
    int varcount = 0;

    NETCDF(nc_inq_nvars(ncid, &varcount));

    for (int i = 0; i < varcount; ++i) {
      // Get the variable name, type, dimensions in one call
      char name_in[NC_MAX_NAME + 1];
      nc_type xtypep;
      int ndimsp2;
      int dimidsp[NC_MAX_VAR_DIMS];

      NETCDF(nc_inq_var(ncid, i, name_in, &xtypep, &ndimsp2, dimidsp, nullptr));

      const std::string name = name_in;

      // We only support 1D columns, one per table dimension
      if (ndimsp2 != 1) {
        fLogSevere("Skipping netcdf binary table column '{}' since it is not 1D.", name);
        continue;
      }

      size_t rowSize = 0;

      NETCDF(nc_inq_dimlen(ncid, dimidsp[0], &rowSize));

      // A units attribute is optional, defaults to dimensionless
      std::string units = "dimensionless";

      IONetcdf::getAtt(ncid, Constants::Units, units, i);

      // Handle our stock types, mirroring the writer.
      if (xtypep == NC_STRING) {
        std::vector<char *> raw(rowSize, nullptr);

        if (rowSize > 0) {
          NETCDF(nc_get_var_string(ncid, i, &raw[0]));
        }

        std::vector<std::string> data(rowSize);

        for (size_t r = 0; r < rowSize; ++r) {
          data[r] = (raw[r] != nullptr) ? raw[r] : "";
        }

        if (rowSize > 0) {
          nc_free_string(rowSize, &raw[0]);
        }
        newOne->addColumn(name, units, data);
      } else if (xtypep == NC_FLOAT) {
        std::vector<float> data(rowSize);

        if (rowSize > 0) {
          NETCDF(nc_get_var_float(ncid, i, &data[0]));
        }
        newOne->addColumn(name, units, data);
      } else if (xtypep == NC_USHORT) {
        std::vector<unsigned short> data(rowSize);

        if (rowSize > 0) {
          NETCDF(nc_get_var_ushort(ncid, i, &data[0]));
        }
        newOne->addColumn(name, units, data);
      } else if (xtypep == NC_UBYTE) {
        std::vector<unsigned char> data(rowSize);

        if (rowSize > 0) {
          NETCDF(nc_get_var_uchar(ncid, i, &data[0]));
        }
        newOne->addColumn(name, units, data);
      } else {
        fLogSevere(
          "Netcdf reader, unknown binary table column type for '{}', skipping.", name);
      }
    }
  } catch (const NetcdfException& ex) {
    fLogSevere("Netcdf read error with binary table: {}", ex.getNetcdfStr());
    return (nullptr);
  }
  return (newOne);
} // NetcdfBinaryTable::readNETCDF

bool
NetcdfBinaryTable::writeNETCDF(int ncid,
  std::shared_ptr<DataType>        dt,
  IOConfig                         & keys)
{
  try {
    std::shared_ptr<BinaryTable> pBinaryTable = std::dynamic_pointer_cast<BinaryTable>(dt);
    BinaryTable& binaryTable = *pBinaryTable;

    // Generically write a binary table's stuff to netcdf.  This uses an API
    // within the binary table to avoid coupling and to allow dynamic expansion
    std::vector<BinaryTable::TableInfo> infos = binaryTable.getTableInfo();
    size_t dimCount = infos.size();

    // fLogDebug("---> NETCDF GOT COUNT OF {}", dimCount);
    if (dimCount > 0) {
      std::vector<int> dims, vars;
      int dim, varid;

      // Step 1: For each 'table' of data (one dimension) that binary table
      // provides
      // (Think of dimension here as number of rows of 'a' table, with N
      // columns)
      for (size_t i = 0; i < dimCount; i++) {
        const BinaryTable::TableInfo t = infos[i];

        fLogDebug("---> Adding table  {} with row size {}", t.name, t.size);
        NETCDF(nc_def_dim(ncid, t.name.c_str(), t.size, &dim));
        dims.push_back(dim);
      }

      // Step 2: Now add the 'columns' for each table
      for (size_t i = 0; i < dimCount; i++) {
        const BinaryTable::TableInfo t = infos[i];

        const std::vector<std::string>& columnNames = t.columnNames;
        const std::vector<std::string>& columnTypes = t.columnTypes;

        for (size_t j = 0; j < columnNames.size(); j++) {
          const std::string& type = columnTypes[j];
          const std::string& name = columnNames[j];

          // fLogDebug("---> Adding column  {}", name);

          int aNcType = NC_FLOAT;

          if (type == "string") {
            aNcType = NC_STRING;
          } else if (type == "float") {
            aNcType = NC_FLOAT;
          } else if (type == "ushort") {
            aNcType = NC_USHORT;
          } else if (type == "uchar") {
            aNcType = NC_UBYTE;
          } else {
            fLogSevere("Netcdf encoder, binary table unknown data type '{}'", type);
          }

          // Create and push back the new nc var
          // int shuffle = NC_SHUFFLE; needs chunk size array in var_chunking..
          const int shuffle       = NC_CONTIGUOUS;
          const int deflate       = 1;
          const int deflate_level = IONetcdf::GZ_LEVEL;

          // New var.  Use '1' here because we create a vector variable
          NETCDF(nc_def_var(ncid, name.c_str(), aNcType, 1, &dims[i], &varid));

          // Compression for variable
          NETCDF(nc_def_var_chunking(ncid, varid, 0, 0));

          // shuffle == control the HDF5 shuffle filter
          // default == turn on deflate for variable
          // deflate_level 0 no compression and 9 (max compression)
          NETCDF(nc_def_var_deflate(ncid, varid, shuffle, deflate,
            deflate_level));

          vars.push_back(varid);
        }
      }

      // Step 3: Now add the actual column data for each column...
      size_t atVar = 0;

      for (size_t i = 0; i < dimCount; i++) {
        const BinaryTable::TableInfo t = infos[i];

        const std::vector<std::string>& columnNames = t.columnNames;
        const std::vector<std::string>& columnUnits = t.columnUnits;
        const std::vector<std::string>& columnTypes = t.columnTypes;

        for (size_t j = 0; j < columnNames.size(); j++) {
          const std::string& type = columnTypes[j];
          const std::string& name = columnNames[j];
          const std::string& unit = columnUnits[j];

          // fLogDebug("---> Adding data to column  {}, data type is '{}'", name, type);
          // Created in step two
          varid = vars[atVar];
          atVar++;

          // Handle our stock types.  Type checking usually a bad design,
          // however, we
          // probably won't add many types to this...
          if (type == "string") {
            std::vector<std::string> data = binaryTable.getStringVector(name);
            validateLength<std::string>(name, data, t.size);

            // Convert c++ string array to char* fun fun. Copies so for large
            // string arrays
            // might need some work for efficiency...
            // We could write each string one at a time without copying first,
            // need tests to
            // see which is faster..mass copy and one netcdf call, or multiple
            // netcdf calls..
            std::vector<char *> vc;
            std::transform(data.begin(), data.end(), std::back_inserter(
                vc), convert);
            const char ** vc2 = const_cast<const char **>(&vc[0]);

            // Now write the character array
            int retval = nc_put_var_string(ncid, varid, vc2);

            // Clean up c strings
            for (size_t z = 0; z < vc.size(); z++) {
              delete[] vc[z];
            }

            if (retval) {
              fLogSevere("Netcdf write error {}", nc_strerror(retval));
              return (false);
            }
          } else if (type == "float") {
            std::vector<float> data = binaryTable.getFloatVector(name);
            validateLength<float>(name, data, t.size);
            NETCDF(nc_put_var_float(ncid, varid, &data[0]));
          } else if (type == "ushort") {
            std::vector<unsigned short> data = binaryTable.getUShortVector(name);
            validateLength<unsigned short>(name, data, t.size);
            NETCDF(nc_put_var_ushort(ncid, varid, &data[0]));
          } else if (type == "uchar") {
            std::vector<unsigned char> data = binaryTable.getUCharVector(name);
            validateLength<unsigned char>(name, data, t.size);
            NETCDF(nc_put_var_uchar(ncid, varid, &data[0]));
          } else {
            fLogSevere(
              "NETCDF writer, unrecognized data type defined by a binary table.  We don't know how to write type '{}'",
              type);
            break;
          }

          // We'll probably need a unit as well...
          // Which one?  string, text, uchar?  lol..
          NETCDF(nc_put_att_text(ncid, varid, Constants::Units,
            unit.size(), &unit[0]));
        }
      }
    }

    // Add globals using the standard attribute machinery.  This writes
    // TypeName, DataType, space/time/reference and the missing data
    // constants in the same way as the other netcdf writers.
    binaryTable.updateGlobalAttributes(binaryTable.getDataType());
    IONetcdf::setAttributes(ncid, NC_GLOBAL, binaryTable.getGlobalAttributes());
  } catch (const NetcdfException& ex) {
    fLogSevere("Netcdf write error with BinaryTable: {}", ex.getNetcdfStr());
    return (false);
  }
  return (true);
} // NetcdfBinaryTable::write
