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
WObsBinaryTable::getBlockLevels(std::vector<std::string>& levels)
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

void
RObsBinaryTable::getBlockLevels(std::vector<std::string>& levels)
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

bool
RObsBinaryTable::dumpToText(std::ostream& o)
{
  const std::string i = "\t";

  o << "MRMS RObsBinaryTable (Used by w2merger)\n";

  auto& t = *this;

  // ---------------------------------------------------------
  //  RObsBinaryTable stuff...
  o << i << "RadarName: " << t.radarName << "\n"; // RObsBinaryTable
  o << i << "VCP: " << t.vcp << "\n";             // RObsBinaryTable
  o << i << "Elevation: " << t.elev << "\n";      // RObsBinaryTable
  o << i << "Units: " << t.getUnits() << "\n";    // DataType

  // WObsBinaryTable.  FIXME: probably don't need multiple classes anymore?
  o << i << "TypeName: " << t.typeName << "\n";
  o << i << "MarkedLinesCacheFile: " << t.markedLinesCacheFile << "\n";
  o << i << "MarkedLinesInternalSize: " << t.markedLines.size() << "\n";
  o << i << "Latitude: " << t.lat << " Degrees.\n";
  o << i << "Longitude: " << t.lon << " Degrees.\n";
  o << i << "Height: " << t.ht << " Kilometers.\n";
  o << i << "Time: " << t.data_time << " Epoch.\n";
  o << i << "Valid Time: " << t.valid_time << " Seconds.\n";
  o << "\n------------------------------\n";

  // Marked lines for WObsBinarytable
  size_t aSize = markedLines.size();

  o << i << "Marked line array size: " << aSize << "\n";
  if (aSize > 5) { aSize = 5; }
  for (int i = 0; i < aSize; i++) {
    o << "  Line: " << i << "(" << markedLines[i].len << ") ( " << markedLines[i].x << ","
      << markedLines[i].y << "," << markedLines[i].z << ")\n";
  }

  // Arrays for WObsBinaryTable
  // Ok do we dump per point, or do separate arrays like ncdump?
  // Could be a key option passed in..
  // RObsBinaryTable adds azimuth, aztime
  size_t s = t.x.size();

  o << i << "Points: " << s << "\n";
  if (s > 100) { s = 100; } // For now dump just s points for debugging
  o << i << "Sampling first " << s << " data points:\n";

  // Dumping info per point vs per array.  Having a toggle even with ncdump would
  // be a nice ability
  auto& x = t.x; // WObs
  auto& y = t.y;
  auto& z = t.z;

  for (size_t j = 0; j < s; j++) {
    o << j << ":\n";
    o << i << x[j] << ", " << y[j] << ", " << z[j] << " (xyz)\n";
    o << i << t.newvalue[j] << ", " << t.scaled_dist[j] << ", " << (int) t.elevWeightScaled[j] <<
      " (value, w1-dist, w2-elev)\n";
    // Only RObs:
    o << i << t.azimuth[j] << ", " << t.aztime[j].epoch_sec << "." << t.aztime[j].frac_sec << ", " <<
      "(az, aztime)\n";
  }

  return true;
} // RObsBinaryTable::dumpToText
