#include "rIOCSV.h"
#include <rDataTable.h>
#include <rError.h>
#include <rStrings.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <locale>
#include "rapidcsv.h"

using namespace rapio;

extern "C" {
void *
createRAPIOIO(void)
{
  auto * z = new IOCSV();

  z->initialize();
  return reinterpret_cast<void *>(z);
}
};

std::string
IOCSV::getHelpString(const std::string& key)
{
  return
    "builder for inputting/outputting CSV tabular data. (Use 'fastwrite=true' to bypass rapidcsv memory allocations for massive files)";
}

void
IOCSV::initialize()
{
  // Register any CSV-specific specializers here if needed in the future
}

namespace {
constexpr const char * PREFIX_INT    = "int:";
constexpr const char * PREFIX_FLOAT  = "float:";
constexpr const char * PREFIX_STRING = "str:";

// Used only by the "fastwrite" mode
std::string
escapeCSVField(const std::string& val)
{
  if ((val.find(',') == std::string::npos) &&
    (val.find('"') == std::string::npos) &&
    (val.find('\n') == std::string::npos) &&
    (val.find('\r') == std::string::npos) )
  {
    return val;
  }

  std::string escaped;

  escaped.reserve(val.size() + 4);
  escaped += '"';

  for (char c : val) {
    if (c == '"') { escaped += "\"\""; } else { escaped += c; }
  }

  escaped += '"';
  return escaped;
}
}

std::shared_ptr<DataType>
IOCSV::createDataType(IOConfig& config)
{
  std::string path = config.getParamURL().getPath();

  fLogInfo("CSV reader: {}", path);
  bool hasHeader = (config.get("header", "true") == "true");
  int headerIdx  = hasHeader ? 0 : -1;

  try {
    // Force RapidCSV to ignore trailing empty newlines, and force "C" locale for floats
    // so it properly reads "." instead of breaking on "," if the system locale differs.
    rapidcsv::ConverterParams convParams;
    convParams.mNumericLocale = false; // Forces C-locale for string-to-float conversions

    rapidcsv::Document doc(path,
      rapidcsv::LabelParams(headerIdx, -1),
      rapidcsv::SeparatorParams(),
      convParams,
      rapidcsv::LineReaderParams(false, '#', true)); // skipEmptyLines = true

    auto table = std::make_shared<DataTable>();

    size_t colCount = doc.GetColumnCount();
    std::vector<std::string> colNames;

    if (hasHeader) {
      colNames = doc.GetColumnNames();
      // FIX: rapidcsv sometimes infers an extra phantom column at the end if lines have
      // trailing CRLF quirks or if InsertColumn allocated mismatched row bounds.
      while (!colNames.empty() && colNames.back().empty()) {
        colNames.pop_back();
      }
      colCount = colNames.size(); // Recalibrate column count
    } else {
      for (size_t c = 0; c < colCount; ++c) {
        colNames.push_back("Column_" + std::to_string(c));
      }
    }

    // Pass 1: Deduce column types and register them
    for (size_t c = 0; c < colCount; ++c) {
      std::string colName      = colNames[c];
      DataColumn::Type colType = DataColumn::Type::String;
      bool typeForced = false;

      if (Strings::beginsWith(colName, PREFIX_INT)) {
        colType = DataColumn::Type::Integer;
        Strings::removePrefix(colName, PREFIX_INT);
        typeForced = true;
      } else if (Strings::beginsWith(colName, PREFIX_FLOAT)) {
        colType = DataColumn::Type::Float;
        Strings::removePrefix(colName, PREFIX_FLOAT);
        typeForced = true;
      } else if (Strings::beginsWith(colName, PREFIX_STRING)) {
        colType = DataColumn::Type::String;
        Strings::removePrefix(colName, PREFIX_STRING);
        typeForced = true;
      }

      if (!typeForced && (doc.GetRowCount() > 0)) {
        bool hasDecimal   = false;
        bool isNumeric    = true;
        size_t rowsToScan = std::min<size_t>(doc.GetRowCount(), 100);

        for (size_t r = 0; r < rowsToScan; ++r) {
          std::string val = doc.GetCell<std::string>(c, r);
          Strings::trim(val);
          if (val.empty()) { continue; }

          try {
            size_t parsedLen = 0;
            std::stod(val, &parsedLen);

            if (parsedLen != val.length()) {
              isNumeric = false;
              break;
            }
            if ((val.find('.') != std::string::npos) ||
              (val.find('e') != std::string::npos) ||
              (val.find('E') != std::string::npos) )
            {
              hasDecimal = true;
            }
          } catch (...) {
            isNumeric = false;
            break;
          }
        }

        if (isNumeric) {
          colType = hasDecimal ? DataColumn::Type::Float : DataColumn::Type::Integer;
        }
      }

      colNames[c] = colName;
      table->addColumn(colName, colType);
    }

    // Pass 2: Extract data by column index
    for (size_t r = 0; r < doc.GetRowCount(); ++r) {
      for (size_t c = 0; c < colCount; ++c) {
        DataColumn& dataCol = table->getColumn(colNames[c]);

        if (dataCol.getType() == DataColumn::Type::Float) {
          try {
            dataCol.push_back(doc.GetCell<float>(c, r));
          } catch (...) {
            dataCol.push_back(static_cast<float>(Constants::MissingData));
          }
        } else if (dataCol.getType() == DataColumn::Type::Integer) {
          try {
            dataCol.push_back(doc.GetCell<int>(c, r));
          } catch (...) {
            dataCol.push_back(static_cast<int>(Constants::MissingData));
          }
        } else {
          try {
            dataCol.push_back(doc.GetCell<std::string>(c, r));
          } catch (...) {
            dataCol.push_back("");
          }
        }
      }
    }

    fLogInfo("IOCSV initialized columnar DataTable with {} rows.", table->getRowCount());
    return table;
  } catch (const std::exception& e) {
    fLogSevere("IOCSV parsing failed: {}", e.what());
    return nullptr;
  }
} // IOCSV::createDataType

bool
IOCSV::encodeDataType(std::shared_ptr<DataType> dt, IOConfig& keys)
{
  auto table = std::dynamic_pointer_cast<DataTable>(dt);

  if (!table) {
    fLogSevere("IOCSV writer requires a DataTable object.");
    return false;
  }

  std::string filename;

  if (!resolveFileName(keys, "csv", "csv-", filename)) {
    return false;
  }

  bool hasHeader    = (keys.get("header", "true") == "true");
  bool typedHeaders = (keys.get("typed_headers", "true") == "true");
  bool fastWrite    = (keys.get("fastwrite") == "true");
  bool successful   = false;

  if (!fastWrite) {
    // ---------------------------------------------------------
    // DEFAULT ROUTE: Robust writing using RapidCSV
    // ---------------------------------------------------------
    try {
      rapidcsv::Document doc("", rapidcsv::LabelParams(hasHeader ? 0 : -1, -1));
      const auto& colNames = table->getColumnNames();
      size_t rowCount      = table->getRowCount();

      for (size_t c = 0; c < colNames.size(); ++c) {
        DataColumn& dataCol    = table->getColumn(colNames[c]);
        std::string headerName = colNames[c];

        if (typedHeaders) {
          if (dataCol.getType() == DataColumn::Type::Integer) {
            headerName = std::string(PREFIX_INT) + headerName;
          } else if (dataCol.getType() == DataColumn::Type::Float) {
            headerName = std::string(PREFIX_FLOAT) + headerName;
          } else {
            headerName = std::string(PREFIX_STRING) + headerName;
          }
        }

        std::vector<std::string> stringifiedCol;
        stringifiedCol.reserve(rowCount);

        for (size_t r = 0; r < rowCount; ++r) {
          if (dataCol.getType() == DataColumn::Type::Integer) {
            stringifiedCol.push_back(std::to_string(dataCol.getIntVector()[r]));
          } else if (dataCol.getType() == DataColumn::Type::Float) {
            // FIX: Force C-locale so floats are written with '.' instead of ','
            std::ostringstream oss;
            oss.imbue(std::locale::classic());
            oss << dataCol.getFloatVector()[r];
            stringifiedCol.push_back(oss.str());
          } else {
            stringifiedCol.push_back(dataCol.getStringVector()[r]);
          }
        }
        doc.InsertColumn(c, stringifiedCol, headerName);
      }
      doc.Save(filename);
      successful = true;
    } catch (const std::exception& e) {
      fLogSevere("Failed to write {} via rapidcsv, reason: {}", filename, e.what());
    }
  } else {
    // ---------------------------------------------------------
    // FASTWRITE ROUTE: Low-memory streaming via std::ofstream
    // ---------------------------------------------------------
    try {
      std::ofstream out(filename, std::ofstream::out);
      if (out.is_open()) {
        // FIX: Force C-locale on the ofstream so floats are written with '.'
        out.imbue(std::locale::classic());

        const auto& colNames = table->getColumnNames();

        if (hasHeader) {
          for (size_t i = 0; i < colNames.size(); ++i) {
            std::string headerName = colNames[i];
            if (typedHeaders) {
              DataColumn& dataCol = table->getColumn(colNames[i]);
              if (dataCol.getType() == DataColumn::Type::Integer) {
                headerName = std::string(PREFIX_INT) + headerName;
              } else if (dataCol.getType() == DataColumn::Type::Float) {
                headerName = std::string(PREFIX_FLOAT) + headerName;
              } else {
                headerName = std::string(PREFIX_STRING) + headerName;
              }
            }
            out << escapeCSVField(headerName);
            if (i < colNames.size() - 1) { out << ","; }
          }
          out << "\n";
        }

        size_t rowCount = table->getRowCount();
        for (size_t r = 0; r < rowCount; ++r) {
          for (size_t c = 0; c < colNames.size(); ++c) {
            DataColumn& dataCol = table->getColumn(colNames[c]);
            if (dataCol.getType() == DataColumn::Type::Integer) {
              out << dataCol.getIntVector()[r];
            } else if (dataCol.getType() == DataColumn::Type::Float) {
              out << dataCol.getFloatVector()[r];
            } else {
              out << escapeCSVField(dataCol.getStringVector()[r]);
            }
            if (c < colNames.size() - 1) { out << ","; }
          }
          out << "\n";
        }
        out.close();
        successful = true;
      } else {
        fLogSevere("Couldn't open {} for fastwrite.", filename);
      }
    } catch (const std::exception& e) {
      fLogSevere("Failed to fastwrite {}, reason: {}", filename, e.what());
    }
  }

  if (successful) {
    successful = postWriteProcess(filename, keys);
  }
  if (successful) {
    std::string method = fastWrite ? " (fastwrite)" : " (rapidcsv)";
    showFileInfo("CSV writer" + method + ": ", keys);
  }
  return successful;
} // IOCSV::encodeDataType
