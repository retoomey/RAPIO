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

  // Register by the semantic DataType string returned by the objects in memory
  owner->introduce("BinaryTable", io);       // Base
  owner->introduce("FusionBinaryTable", io); // Stage 1 legacy
  owner->introduce("WObsBinaryTable", io);   // Stage 1 generic
  owner->introduce("RObsBinaryTable", io);   // Stage 1 generic + tracking
  owner->introduce("WindBinaryTable", io);   // Stage 1 wind payload
}

std::shared_ptr<DataType>
TextBinaryTable::read(IOConfig& config)
{
  return nullptr; // Read not supported for text dumps
}

bool
TextBinaryTable::write(std::shared_ptr<DataType> dt, IOConfig & keys)
{
  std::shared_ptr<BinaryTable> btable = std::dynamic_pointer_cast<BinaryTable>(dt);

  if (btable == nullptr) {
    fLogSevere("Not a BinaryTable we can't handle anything else.");
    return false;
  }

  try {
    std::ostream& o     = *IOText::theFile;
    const std::string i = "\t";

    // Extract the magic string for the header printout
    std::vector<std::string> levels;
    btable->getBlockLevels(levels);
    std::string magic_string;
    BinaryTable::blockLevelsToMagic(levels, magic_string);

    // 1. Output Standard Header
    o << "RAPIO Generic BinaryTable Dump (" << magic_string << ")\n";
    o << "Header:\n";

    std::string radarname;
    btable->getString("Radarname", radarname);
    o << i << "RadarName: '" << radarname << "'\n";

    std::string typename_str;
    btable->getString("Typename", typename_str);
    o << i << "Typename: '" << typename_str << "'\n";
    o << i << "Units: '" << btable->getUnits() << "'\n";

    long xBase = 0, yBase = 0;
    btable->getLong("xBase", xBase);
    btable->getLong("yBase", yBase);
    o << i << "xBase: '" << xBase << "'\n";
    o << i << "yBase: '" << yBase << "'\n";

    bool flag = btable->getUseMissingAsUnavailable();
    o << i << "NoMissing: '" << (flag ? "On" : "Off") << "'\n";

    o << i << "Time: '" << btable->getTime() << "'\n";
    o << i << "Center: '" << btable->getLocation() << "'\n";

    // 2. Introspect and Output Tables dynamically
    std::vector<BinaryTable::TableInfo> tables = btable->getTableInfo();

    for (const auto& table : tables) {
      o << "\nTable: " << table.name << " (Rows: " << table.size << ")\n";

      if ((table.size == 0) || table.columnNames.empty()) {
        o << i << "[Empty Table]\n";
        continue;
      }

      // Pre-fetch all vectors for this table to avoid repeated virtual calls
      std::vector<std::vector<float> > floatCols(table.columnNames.size());
      std::vector<std::vector<short> > shortCols(table.columnNames.size());
      std::vector<std::vector<char> > charCols(table.columnNames.size());
      std::vector<std::vector<unsigned short> > ushortCols(table.columnNames.size());

      for (size_t c = 0; c < table.columnNames.size(); ++c) {
        const std::string& cName = table.columnNames[c];
        const std::string& cType = table.columnTypes[c];

        if (cType == "float") { floatCols[c] = btable->getFloatVector(cName); }
        if (cType == "short") { shortCols[c] = btable->getShortVector(cName); }
        if (cType == "char") { charCols[c] = btable->getCharVector(cName); }
        if (cType == "ushort") { ushortCols[c] = btable->getUShortVector(cName); }
      }

      // Print Column Headers
      o << i;
      for (size_t c = 0; c < table.columnNames.size(); ++c) {
        o << table.columnNames[c] << " (" << table.columnUnits[c] << ")";
        if (c < table.columnNames.size() - 1) { o << ", "; }
      }
      o << "\n" << i << std::string(60, '-') << "\n";

      // Print Rows (Sample first 100 max to avoid console flooding)
      size_t printRows = std::min(table.size, static_cast<size_t>(100));
      for (size_t row = 0; row < printRows; ++row) {
        o << i << row << ": [";
        for (size_t c = 0; c < table.columnNames.size(); ++c) {
          const std::string& cType = table.columnTypes[c];

          if ((cType == "float") && !floatCols[c].empty()) {
            o << floatCols[c][row];
          } else if ((cType == "short") && !shortCols[c].empty()) {
            o << shortCols[c][row];
          } else if ((cType == "char") && !charCols[c].empty()) {
            o << (int) charCols[c][row]; // Cast to int so it prints as a number
          } else if ((cType == "ushort") && !ushortCols[c].empty()) {
            o << ushortCols[c][row];
          } else {
            o << "?"; // Fallback if type doesn't match fetched data
          }

          if (c < table.columnNames.size() - 1) { o << ", "; }
        }
        o << "]\n";
      }

      if (table.size > 100) {
        o << i << "... (" << (table.size - 100) << " more rows hidden)\n";
      }
    }

    o << std::defaultfloat;
    return true;
  } catch (const std::exception& e) {
    fLogSevere("Error writing to IOTEXT open file: {}", e.what());
    return false;
  }
} // TextBinaryTable::write
