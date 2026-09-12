#include "rTextDataTable.h"
#include "rError.h"
#include <fmt/format.h>
#include <vector>
#include <algorithm>

using namespace rapio;

void TextDataTable::introduceSelf(IOText * owner) {
  std::shared_ptr<IOSpecializer> io = std::make_shared<TextDataTable>();
  owner->introduce("DataTable", io);
}

std::shared_ptr<DataType> TextDataTable::read(IOConfig& config) {
  return nullptr; 
}

bool TextDataTable::write(std::shared_ptr<DataType> dt, IOConfig& keys) {
  auto table = std::dynamic_pointer_cast<DataTable>(dt);
  if (!table) {
    fLogSevere("Not a DataTable.");
    return false;
  }

  std::ostream& o = *IOText::theFile;
  o << "RAPIO DataTable Dump\n";
  o << "Rows: " << table->getRowCount() << "\n\n";

  const auto& colNames = table->getColumnNames();
  const size_t numCols = colNames.size();
  const size_t numRows = table->getRowCount();

  if (numCols == 0) {
    o << "[Empty Table]\n";
    return true;
  }

  // Pass 1: Calculate max widths for each column
  std::vector<size_t> colWidths(numCols, 0);
  for (size_t c = 0; c < numCols; ++c) {
    colWidths[c] = colNames[c].length();
    DataColumn& col = table->getColumn(colNames[c]);
    
    for (size_t r = 0; r < numRows; ++r) {
      size_t len = 0;
      if (col.getType() == DataColumn::Type::Integer) {
        len = fmt::format("{}", col.getIntVector()[r]).length();
      } else if (col.getType() == DataColumn::Type::Float) {
        len = fmt::format("{}", col.getFloatVector()[r]).length();
      } else {
        len = col.getStringVector()[r].length();
      }
      colWidths[c] = std::max(colWidths[c], len);
    }
  }

  const size_t padding = 2; // Spacing between columns
  size_t totalWidth = 0;
  for (size_t c = 0; c < numCols; ++c) {
    totalWidth += colWidths[c];
  }
  totalWidth += padding * (numCols - 1);

  // Pass 2: Output with dynamic padding
  // Print Header
  for (size_t c = 0; c < numCols; ++c) {
    o << fmt::format("{:<{}}", colNames[c], colWidths[c] + (c < numCols - 1 ? padding : 0));
  }
  o << "\n";

  // Print Separator Line
  o << std::string(totalWidth, '-') << "\n";

  // Print Data Rows
  for (size_t r = 0; r < numRows; ++r) {
    for (size_t c = 0; c < numCols; ++c) {
      DataColumn& col = table->getColumn(colNames[c]);
      std::string valStr;
      
      if (col.getType() == DataColumn::Type::Integer) {
        valStr = fmt::format("{}", col.getIntVector()[r]);
      } else if (col.getType() == DataColumn::Type::Float) {
        valStr = fmt::format("{}", col.getFloatVector()[r]);
      } else {
        valStr = col.getStringVector()[r];
      }
      
      o << fmt::format("{:<{}}", valStr, colWidths[c] + (c < numCols - 1 ? padding : 0));
    }
    o << "\n";
  }

  return true;
}
