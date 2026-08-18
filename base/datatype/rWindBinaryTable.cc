#include "rWindBinaryTable.h"
#include "rError.h"
#include "rBinaryIO.h"

using namespace rapio;
using namespace std;

size_t WindBinaryTable::BLOCK_LEVEL;

WindBinaryTable::WindBinaryTable()
  : myVersionID(Version), myMissingMode(0), myValueSize(0), myMissingSize(0)
{
  myDataType = "WindBinaryTable";
  setReadFactory("raw");
}

WindBinaryTable::~WindBinaryTable(){ }

void
WindBinaryTable::getBlockLevels(std::vector<std::string>& levels) const
{
  BinaryTable::getBlockLevels(levels);
  levels.push_back("WIND");
  WindBinaryTable::BLOCK_LEVEL = levels.size();
}

bool
WindBinaryTable::writeBlock(FILE * fp)
{
  if (BinaryTable::writeBlock(fp)) {
    BinaryIO::write_type<size_t>(myVersionID, fp);

    std::string radarName, typeName, units;
    BinaryIO::write_type<char>(myMissingMode, fp);

    getString("Radarname", radarName);
    getString("Typename", typeName);
    units = getUnits();

    long xBase = 0, yBase = 0;
    getLong("xBase", xBase);
    getLong("yBase", yBase);

    LLH center = getLocation();
    Time t     = getTime();

    // Write standard header info
    BinaryIO::write_string8(radarName, fp);
    BinaryIO::write_string8(typeName, fp);
    BinaryIO::write_string8(units, fp);
    BinaryIO::write_type<long>(xBase, fp);
    BinaryIO::write_type<long>(yBase, fp);

    BinaryIO::write_type<double>(center.getLatitudeDeg(), fp);
    BinaryIO::write_type<double>(center.getLongitudeDeg(), fp);
    BinaryIO::write_type<float>(center.getHeightKM(), fp); // Passed as KM matching LLH standard

    BinaryIO::write_type<time_t>(t.getSecondsSinceEpoch(), fp);
    BinaryIO::write_type<double>(t.getFractional(), fp);

    myValueSize   = myXs.size();
    myMissingSize = myXMissings.size();

    BinaryIO::write_type<size_t>(myValueSize, fp);
    BinaryIO::write_type<size_t>(myMissingSize, fp);

    // Write the coordinates (ZLIB compressed)
    BinaryIO::write_vector("X", myXs, fp);
    BinaryIO::write_vector("Y", myYs, fp);
    BinaryIO::write_vector("Z", myZs, fp);

    // Write the 5 Matrix/Vector components
    BinaryIO::write_vector("M11", myM11, fp);
    BinaryIO::write_vector("M22", myM22, fp);
    BinaryIO::write_vector("M12", myM12, fp);
    BinaryIO::write_vector("P1", myP1, fp);
    BinaryIO::write_vector("P2", myP2, fp);

    // Write Missing RLE
    BinaryIO::write_vector("Xm", myXMissings, fp);
    BinaryIO::write_vector("Ym", myYMissings, fp);
    BinaryIO::write_vector("Zm", myZMissings, fp);
    BinaryIO::write_vector("Lm", myLMissings, fp);

    return true;
  }
  return false;
} // WindBinaryTable::writeBlock

bool
WindBinaryTable::readBlock(const std::string& path, FILE * fp)
{
  if (BinaryTable::readBlock(path, fp) && matchBlockLevel(WindBinaryTable::BLOCK_LEVEL)) {
    BinaryIO::read_type<size_t>(myVersionID, fp);
    if (myVersionID != Version) {
      fLogSevere("Error reading raw file. Version is unknown {}", myVersionID);
      return false;
    }

    std::string radarName, typeName, units;
    BinaryIO::read_type<char>(myMissingMode, fp);

    BinaryIO::read_string8(radarName, fp);
    setString("Radarname", radarName);

    BinaryIO::read_string8(typeName, fp);
    setString("Typename", typeName);

    BinaryIO::read_string8(units, fp);
    setUnits(units);

    long xBase = 0, yBase = 0;
    BinaryIO::read_type<long>(xBase, fp);
    setLong("xBase", xBase);

    BinaryIO::read_type<long>(yBase, fp);
    setLong("yBase", yBase);

    double lat, lon;
    float ht;
    BinaryIO::read_type<double>(lat, fp);
    BinaryIO::read_type<double>(lon, fp);
    BinaryIO::read_type<float>(ht, fp);
    setLocation(LLH(lat, lon, ht)); // Center.getHeightKM() was saved directly as float

    time_t t;
    double f;
    BinaryIO::read_type<time_t>(t, fp);
    BinaryIO::read_type<double>(f, fp);
    setTime(Time(t, f));

    BinaryIO::read_type<size_t>(myValueSize, fp);
    BinaryIO::read_type<size_t>(myMissingSize, fp);

    myXs.resize(myValueSize);
    myYs.resize(myValueSize);
    myZs.resize(myValueSize);

    myM11.resize(myValueSize);
    myM22.resize(myValueSize);
    myM12.resize(myValueSize);
    myP1.resize(myValueSize);
    myP2.resize(myValueSize);

    myXMissings.resize(myMissingSize);
    myYMissings.resize(myMissingSize);
    myZMissings.resize(myMissingSize);
    myLMissings.resize(myMissingSize);

    // Read the coordinates
    BinaryIO::read_vector(myXs, fp);
    BinaryIO::read_vector(myYs, fp);
    BinaryIO::read_vector(myZs, fp);

    // Read the 5 Matrix/Vector components
    BinaryIO::read_vector(myM11, fp);
    BinaryIO::read_vector(myM22, fp);
    BinaryIO::read_vector(myM12, fp);
    BinaryIO::read_vector(myP1, fp);
    BinaryIO::read_vector(myP2, fp);

    // Read Missing RLE
    BinaryIO::read_vector(myXMissings, fp);
    BinaryIO::read_vector(myYMissings, fp);
    BinaryIO::read_vector(myZMissings, fp);
    BinaryIO::read_vector(myLMissings, fp);

    return true;
  }
  return false;
} // WindBinaryTable::readBlock

// ------------------------------------------------------------------------
// Introspection Implementation
// ------------------------------------------------------------------------

std::vector<BinaryTable::TableInfo>
WindBinaryTable::getTableInfo()
{
  std::vector<TableInfo> info;

  // Table 1: Valid Data
  TableInfo dataTable;

  dataTable.name        = "Data";
  dataTable.size        = myXs.size();
  dataTable.columnNames = { "X", "Y", "Z", "M11", "M22", "M12", "P1", "P2" };
  dataTable.columnTypes = { "short", "short", "char", "float", "float", "float", "float", "float" };
  dataTable.columnUnits = { "index", "index", "index", "dimless", "dimless", "dimless", "m/s", "m/s" };
  info.push_back(dataTable);

  // Table 2: Missing Data (RLE)
  TableInfo missingTable;

  missingTable.name        = "MissingData";
  missingTable.size        = myXMissings.size();
  missingTable.columnNames = { "Xm", "Ym", "Zm", "Lm" };
  missingTable.columnTypes = { "short", "short", "char", "short" };
  missingTable.columnUnits = { "index", "index", "index", "run_length" };
  info.push_back(missingTable);

  return info;
}

std::vector<float>
WindBinaryTable::getFloatVector(const std::string& name)
{
  if (name == "M11") { return myM11; }
  if (name == "M22") { return myM22; }
  if (name == "M12") { return myM12; }
  if (name == "P1") { return myP1; }
  if (name == "P2") { return myP2; }
  return std::vector<float>();
}

std::vector<short>
WindBinaryTable::getShortVector(const std::string& name)
{
  if (name == "X") { return myXs; }
  if (name == "Y") { return myYs; }
  if (name == "Xm") { return myXMissings; }
  if (name == "Ym") { return myYMissings; }
  if (name == "Lm") { return myLMissings; }
  return std::vector<short>();
}

std::vector<char>
WindBinaryTable::getCharVector(const std::string& name)
{
  if (name == "Z") { return myZs; }
  if (name == "Zm") { return myZMissings; }
  return std::vector<char>();
}
