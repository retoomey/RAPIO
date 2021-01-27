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
    LogSevere("Warning: Binary table returned a row length of "
      << realSize << " for column " << name
      << " so we are padding/truncating\n");
    valid.resize(aSize);
    return (false);
  }
  return (true);
}
}

NetcdfBinaryTable::~NetcdfBinaryTable()
{ }

void
NetcdfBinaryTable::introduceSelf()
{
  std::shared_ptr<NetcdfBinaryTable> newOne = std::make_shared<NetcdfBinaryTable>();
  IONetcdf::introduce("BinaryTable", newOne);
}

std::shared_ptr<DataType>
NetcdfBinaryTable::read(const int ncid,
  const URL                       & loc)
{
  LogSevere("Unimplemented raw table, returning empty table\n");

  std::shared_ptr<BinaryTable> newOne = std::make_shared<BinaryTable>();
  return (newOne);
}

bool
NetcdfBinaryTable::write(int ncid, std::shared_ptr<DataType> dt,
  std::shared_ptr<PTreeNode> dfs)
{
  std::shared_ptr<BinaryTable> binaryTable = std::dynamic_pointer_cast<BinaryTable>(dt);

  return (write(ncid, *binaryTable, IONetcdf::MISSING_DATA,
         IONetcdf::RANGE_FOLDED));
}

/** Write routing using c library */
bool
NetcdfBinaryTable::write(int ncid, BinaryTable& binaryTable,
  const float missing, const float rangeFolded)
{
  try {
    // Generically write a binary table's stuff to netcdf.  This uses an API
    // within the binary table to avoid coupling and to allow dynamic expansion
    std::vector<BinaryTable::TableInfo> infos = binaryTable.getTableInfo();
    size_t dimCount = infos.size();

    // LogDebug("---> NETCDF GOT COUNT OF " << dimCount << "\n");
    if (dimCount > 0) {
      std::vector<int> dims, vars;
      int dim, varid;

      // Step 1: For each 'table' of data (one dimension) that binary table
      // provides
      // (Think of dimension here as number of rows of 'a' table, with N
      // columns)
      for (size_t i = 0; i < dimCount; i++) {
        const BinaryTable::TableInfo t = infos[i];

        // LogDebug("---> Adding table  "<< t.name << " with row size " <<
        // t.size << "\n");
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

          // LogDebug("---> Adding column  "<< name << "\n");

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
            LogSevere(
              "Netcdf encoder, binary table unknown data type '" << type
                                                                 << "'\n");
          }

          // Create and push back the new nc var
          // int shuffle = NC_SHUFFLE; needs chunk size array in var_chunking..
          const int shuffle       = NC_CONTIGUOUS;
          const int deflate       = 1;
          const int deflate_level = IONetcdf::getGZLevel();

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

          // LogDebug("---> Adding data to column  "<< name << ", data type is
          // '" << type << "'\n");
          // Created in step two
          int varid = vars[atVar];
          atVar++;

          // Handle our stock types.  Type checking usually a bad design,
          // however, we
          // probably won't add many types to this...
          if (type == "string") {
            std::vector<std::string> data = binaryTable.getStringVector(name);
            LogSevere("Incoming string array is size " << data.size() << "\n");
            validateLength<std::string>(name, data, t.size);
            LogSevere("String array is now " << data.size() << "\n");

            for (size_t zz = 0; zz < data.size(); zz++) {
              LogSevere("Value " << zz << " == " << data[zz] << "\n");
            }

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
            for (size_t i = 0; i < vc.size(); i++) {
              delete[] vc[i];
            }

            if (retval) {
              LogSevere("Netcdf write error " << nc_strerror(retval) << "\n");
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
            LogSevere(
              "NETCDF writer, unrecognized data type defined by a binary table.  We don't know how to write type '" << type
                                                                                                                    << "'\n");
            break;
          }

          // We'll probably need a unit as well...
          // Which one?  string, text, uchar?  lol..
          NETCDF(nc_put_att_text(ncid, varid, Constants::Units.c_str(),
            unit.size(), &unit[0]));
        }
      }
    }

    // Add globals...
    // FIXME: Binary table needs test case and work using
    // the new arrays and attributes
    // if (!IONetcdf::addGlobalAttr(ncid, binaryTable,
    //  "BinaryTable")) { return (false); }
    // Datatype and typename
    NETCDF(IONetcdf::addAtt(ncid, Constants::TypeName, binaryTable.getTypeName()));
    NETCDF(IONetcdf::addAtt(ncid, Constants::sDataType, "BinaryTable"));

    // Space-time-ref: Latitude, Longitude, Height, Time and FractionalTime
    Time aTime    = binaryTable.getTime();
    LLH aLocation = binaryTable.getLocation();
    NETCDF(IONetcdf::addAtt(ncid, Constants::Latitude, aLocation.getLatitudeDeg()));
    NETCDF(IONetcdf::addAtt(ncid, Constants::Longitude, aLocation.getLongitudeDeg()));
    NETCDF(IONetcdf::addAtt(ncid, Constants::Height, aLocation.getHeightKM() * 1000.0));
    NETCDF(IONetcdf::addAtt(ncid, Constants::Time, aTime.getSecondsSinceEpoch()));
    NETCDF(IONetcdf::addAtt(ncid, Constants::FractionalTime, aTime.getFractional()));

    // float MISSING_DATA( Constants::MissingData );
    // float RANGE_FOLDED( Constants::RangeFolded );
    NETCDF(IONetcdf::addAtt(ncid, "MissingData", missing));
    NETCDF(IONetcdf::addAtt(ncid, "RangeFolded", rangeFolded));
  } catch (const NetcdfException& ex) {
    LogSevere("Netcdf write error with BinaryTable: " << ex.getNetcdfStr()
                                                      << "\n");
    return (false);
  }
  return (true);
} // NetcdfBinaryTable::write
