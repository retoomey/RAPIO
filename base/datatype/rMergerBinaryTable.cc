#include "rMergerBinaryTable.h"

#include "rError.h"
#include "rBinaryIO.h"
#include "rStrings.h"

#include <iomanip>

using namespace rapio;
using namespace std;

size_t WObsBinaryTable::BLOCK_LEVEL;
size_t RObsBinaryTable::BLOCK_LEVEL;

// Weighted  ----------------------------------------
//
void
WObsBinaryTable::getBlockLevels(std::vector<std::string>& levels) const
{
  // Level stack
  BinaryTable::getBlockLevels(levels);
  levels.push_back("W");
  WObsBinaryTable::BLOCK_LEVEL = levels.size();
}

bool
WObsBinaryTable::readBlock(const std::string& path, FILE * fp)
{
  // Blocks have to be ordered by magic string
  // If superclass was able to read its block, then...
  if (BinaryTable::readBlock(path, fp) &&
    matchBlockLevel(WObsBinaryTable::BLOCK_LEVEL))
  {
    // ----------------------------------------------------------
    // Read our data block if available (match with write below)
    BinaryIO::read_string8(typeName, fp);
    myTypeName = typeName;

    // Handle units
    std::string unit;
    BinaryIO::read_string8(unit, fp);
    setUnits(unit);

    BinaryIO::read_type<float>(lat, fp);
    BinaryIO::read_type<float>(lon, fp);
    BinaryIO::read_type<float>(ht, fp);
    myLocation = LLH(lat, lon, ht / 1000.0);
    BinaryIO::read_type<time_t>(data_time, fp);
    BinaryIO::read_type<time_t>(valid_time, fp);

    fLogInfo("RAW INFO: {}/{},{},{},{},{},{}", typeName, unit, lat, lon, ht, data_time, valid_time);
    // Handle the marked lines reading in...
    BinaryIO::read_string8(markedLinesCacheFile, fp);
    std::vector<Line> markedLines;
    if (markedLinesCacheFile == "") {
      BinaryIO::read_vector(markedLines, fp);
    } else {
      fLogSevere("RAWERROR: RAW File wants us to read a cached file, not supported.");
    }

    // Read the six default arrays that always read
    BinaryIO::read_vector(x, fp);
    BinaryIO::read_vector(y, fp);
    BinaryIO::read_vector(z, fp);
    BinaryIO::read_vector(newvalue, fp);
    BinaryIO::read_vector(scaled_dist, fp);
    BinaryIO::read_vector(elevWeightScaled, fp);

    // ----------------------------------------------------------
    return true;
  } else {
    fLogSevere("WObsBinaryTable Missing our data block in .raw file");
  }
  return false;
} // WObsBinaryTable::readBlock

bool
WObsBinaryTable::writeBlock(FILE * fp)
{
  if (BinaryTable::writeBlock(fp)) {
    // ----------------------------------------------------------
    // Write our data block if available (match with read above)

    // Header
    BinaryIO::write_string8(typeName, fp);

    // Handle units
    std::string unit = getUnits();
    BinaryIO::write_string8(unit, fp);

    BinaryIO::write_type<float>(lat, fp);
    BinaryIO::write_type<float>(lon, fp);
    BinaryIO::write_type<float>(ht, fp);
    BinaryIO::write_type<time_t>(data_time, fp);
    BinaryIO::write_type<time_t>(valid_time, fp);

    // Handle the marked lines writing out...
    BinaryIO::write_string8(markedLinesCacheFile, fp);
    if (markedLinesCacheFile == "") {
      BinaryIO::write_vector("Marked", markedLines, fp);
    }

    // Write the six default arrays that always write
    BinaryIO::write_vector("X", x, fp);
    BinaryIO::write_vector("Y", y, fp);
    BinaryIO::write_vector("Z", z, fp);
    BinaryIO::write_vector("V", newvalue, fp);
    BinaryIO::write_vector("Scaled", scaled_dist, fp);
    BinaryIO::write_vector("ElevScaled", elevWeightScaled, fp);
    // ----------------------------------------------------------
    return true;
  } else {
    // No need to error, weighted already did...
  }
  return false;
} // WObsBinaryTable::writeBlock

// ------------------------------------------------------------------------
// WObsBinaryTable Introspection Implementation
// ------------------------------------------------------------------------

std::vector<BinaryTable::TableInfo>
WObsBinaryTable::getTableInfo()
{
  std::vector<TableInfo> info;

  TableInfo dataTable;

  dataTable.name        = "Data";
  dataTable.size        = x.size();
  dataTable.columnNames = { "X", "Y", "Z", "V", "ScaledDist", "ElevScaled" };
  dataTable.columnTypes = { "ushort", "ushort", "ushort", "float", "ushort", "char" };
  dataTable.columnUnits = { "index", "index", "index", "value", "w1", "w2" };
  info.push_back(dataTable);

  // We can also introspect the markedLines if needed, but for standard dumping, the primary vectors are key
  return info;
}

std::vector<float>
WObsBinaryTable::getFloatVector(const std::string& name)
{
  if (name == "V") { return newvalue; }
  return std::vector<float>();
}

std::vector<unsigned short>
WObsBinaryTable::getUShortVector(const std::string& name)
{
  if (name == "X") { return x; }
  if (name == "Y") { return y; }
  // Z is historically stored as unsigned short in this specific table
  if (name == "Z") { return z; }
  if (name == "ScaledDist") { return scaled_dist; }
  return std::vector<unsigned short>();
}

std::vector<char>
WObsBinaryTable::getCharVector(const std::string& name)
{
  if (name == "ElevScaled") { return elevWeightScaled; }
  return std::vector<char>();
}

void
RObsBinaryTable::getBlockLevels(std::vector<std::string>& levels) const
{
  // Level stack
  WObsBinaryTable::getBlockLevels(levels);
  levels.push_back("R");
  RObsBinaryTable::BLOCK_LEVEL = levels.size();
}

bool
RObsBinaryTable::readBlock(const std::string& path, FILE * fp)
{
  // Blocks have to be ordered by magic string
  // If superclass was able to read its block, then...
  if (WObsBinaryTable::readBlock(path, fp) &&
    matchBlockLevel(RObsBinaryTable::BLOCK_LEVEL))
  {
    // ----------------------------------------------------------
    // Read our data block if available (match with write below)

    // More header for us....
    BinaryIO::read_string8(radarName, fp);
    BinaryIO::read_type<int>(vcp, fp);    // fread( &vcp, sizeof(int), 1, fp );
    BinaryIO::read_type<float>(elev, fp); // fread( &elev, sizeof(float), 1, fp );

    // And our arrays
    BinaryIO::read_vector(azimuth, fp);
    BinaryIO::read_vector<mrmstime>(aztime, fp);

    // ----------------------------------------------------------
    return true;
  } else {
    fLogSevere("RObsBinaryTable: Missing our data in .raw file");
  }
  return false;
}

bool
RObsBinaryTable::writeBlock(FILE * fp)
{
  // Blocks have to be ordered by magic string
  if (WObsBinaryTable::writeBlock(fp)) {
    // ----------------------------------------------------------
    // Write our data block if available (match with read above)

    // Header...
    BinaryIO::write_string8(radarName, fp);
    BinaryIO::write_type<int>(vcp, fp);    // fwrite( &vcp, sizeof(int), 1, fp );
    BinaryIO::write_type<float>(elev, fp); // fwrite( &elev, sizeof(float), 1, fp );

    // Data...
    BinaryIO::write_vector("Azimuth", azimuth, fp);
    BinaryIO::write_vector("Aztime", aztime, fp);

    // ----------------------------------------------------------
    return true;
  } else {
    // No need to error, weighted already did...
  }
  return false;
}

// ------------------------------------------------------------------------
// RObsBinaryTable Introspection Implementation
// ------------------------------------------------------------------------

std::vector<BinaryTable::TableInfo>
RObsBinaryTable::getTableInfo()
{
  // Grab the parent table definition
  std::vector<TableInfo> info = WObsBinaryTable::getTableInfo();

  if (info.size() > 0) {
    // Append the extra columns that RObs tracking adds
    info[0].columnNames.push_back("Azimuth");
    info[0].columnTypes.push_back("ushort");
    info[0].columnUnits.push_back("degrees");

    info[0].columnNames.push_back("Epoch");
    info[0].columnTypes.push_back("float");
    info[0].columnUnits.push_back("seconds");
  }

  return info;
}

std::vector<float>
RObsBinaryTable::getFloatVector(const std::string& name)
{
  // For standard visualization, convert the struct to a standard float vector
  if (name == "Epoch") {
    std::vector<float> times(aztime.size());
    for (size_t i = 0; i < aztime.size(); ++i) {
      times[i] = aztime[i].epoch_sec + aztime[i].frac_sec;
    }
    return times;
  }
  return WObsBinaryTable::getFloatVector(name);
}

std::vector<unsigned short>
RObsBinaryTable::getUShortVector(const std::string& name)
{
  if (name == "Azimuth") { return azimuth; }
  return WObsBinaryTable::getUShortVector(name);
}
