#include "rTextBinaryTable.h"

#include "rDataGrid.h"
#include "rError.h"
#include "rUnit.h"
#include "rConstants.h"

#include "rSignals.h"

using namespace rapio;
using namespace std;

TextBinaryTable::~TextBinaryTable()
{ }

void
TextBinaryTable::introduceSelf(IOText * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<TextBinaryTable>();

  owner->introduce("BinaryTable", io);
}

std::shared_ptr<DataType>
TextBinaryTable::read(std::map<std::string, std::string>& keys,
  std::shared_ptr<DataType>                             dt)
{
  return nullptr;
}

bool
TextBinaryTable::write(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>             & keys)
{
  bool successful = false;

  // FIXME: Should be more general
  // Also the BinaryTable class needs refactor, but this is tricky because
  // we also have to maintain backwards compatibility with MRMS files.
  // std::shared_ptr<BinaryTable> table = std::dynamic_pointer_cast<BinaryTable>(dt);
  std::shared_ptr<RObsBinaryTable> table = std::dynamic_pointer_cast<RObsBinaryTable>(dt);

  if (table == nullptr) {
    LogSevere("Not a RObsBinaryTable, currently unimplemented\n");
    return false;
  }
  auto dataType = table->getDataType();

  try{
    std::ofstream& file = IOText::theFile; // no copy constructor

    // Mark standard with RAPIO and datatype
    // This might allow parsing/reading later
    file << "RAPIO/MRMS RObsBinaryTable\n";
    const std::string i = "\t";

    auto& t = *table;

    // -----------------
    // Write to file
    // FIXME: Ok need to rework table with cleaner API and methods
    //        This class has always been a special hack for merger
    //
    // ---------------------------------------------------------
    //  RObsBinaryTable stuff...
    file << i << "RadarName: " << t.radarName << "\n"; // RObsBinaryTable
    file << i << "VCP: " << t.vcp << "\n";             // RObsBinaryTable
    file << i << "Elevation: " << t.elev << "\n";      // RObsBinaryTable
    file << i << "Units: " << t.getUnits() << "\n";    // DataType

    // WObsBinaryTable.  FIXME: probably don't need multiple classes anymore?
    file << i << "TypeName: " << t.typeName << "\n";
    file << i << "MarkedLinesCacheFile: " << t.markedLinesCacheFile << "\n";
    file << i << "MarkedLinesInternalSize: " << t.markedLines.size() << "\n";
    file << i << "Latitude: " << t.lat << " Degrees.\n";
    file << i << "Longitude: " << t.lon << " Degrees.\n";
    file << i << "Height: " << t.ht << " Kilometers.\n";
    file << i << "Time: " << t.data_time << " Epoch.\n";
    file << i << "Valid Time: " << t.valid_time << " Seconds.\n";
    file << "\n------------------------------\n";
    // Arrays for WObsBinaryTable
    // Ok do we dump per point, or do separate arrays like ncdump?
    // Could be a key option passed in..
    // RObsBinaryTable adds azimuth, aztime
    size_t s = t.x.size();
    file << i << "Points: " << s << "\n";
    if (s > 100) { s = 100; } // For now dump just s points for debugging
    file << i << "Sampling first " << s << " data points:\n";

    // Dumping info per point vs per array.  Having a toggle even with ncdump would
    // be a nice ability
    auto& x = t.x; // WObs
    auto& y = t.y;
    auto& z = t.z;

    for (size_t j = 0; j < s; j++) {
      file << j << ":\n";
      file << i << x[j] << ", " << y[j] << ", " << z[j] << " (xyz)\n";
      file << i << t.newvalue[j] << ", " << t.scaled_dist[j] << ", " << (int) t.elevWeightScaled[j] <<
        " (value, w1-dist, w2-elev)\n";
      // Only RObs:
      file << i << t.azimuth[j] << ", " << t.aztime[j].epoch_sec << "." << t.aztime[j].frac_sec << ", " <<
        "(az, aztime)\n";
    }

    successful = true;
  }catch (const std::exception& e) {
    LogSevere("Error writing to IOTEXT open file\n");
  }
  return successful;
} // TextBinaryTable::write
