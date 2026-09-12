#include "rIOCSV.h"
#include <rDataTable.h>
#include <rError.h>
#include <fstream>
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
  return "builder for inputting/outputting CSV tabular data.";
}

void
IOCSV::initialize()
{
  // Register any CSV-specific specializers here if needed in the future
}

std::shared_ptr<DataType>
IOCSV::createDataType(IOConfig& config)
{
  std::string path = config.getParamURL().getPath();

  fLogInfo("CSV reader: {}", path);
  // Default is header on (you should always use one)
  bool hasHeader = (config.get("header", "true") == "true");
  int headerIdx = hasHeader ? 0 : -1;

  try {
    rapidcsv::Document doc(path, rapidcsv::LabelParams(headerIdx, -1));
    auto table = std::make_shared<DataTable>();
    
    size_t colCount = doc.GetColumnCount();
    std::vector<std::string> colNames;
    
    // Auto-generate names if the header is missing
    if (hasHeader) {
      colNames = doc.GetColumnNames();
    } else {
      for (size_t c = 0; c < colCount; ++c) {
        colNames.push_back("Column_" + std::to_string(c));
      }
    }

    // Pass 1: Deduce column types and register them
    for (size_t c = 0; c < colCount; ++c) {
      DataColumn::Type colType = DataColumn::Type::String;
      if (doc.GetRowCount() > 0) {
        std::string firstVal = doc.GetCell<std::string>(c, 0);
        try {
          if (firstVal.find('.') != std::string::npos) {
            std::stof(firstVal);
            colType = DataColumn::Type::Float;
          } else {
            std::stoi(firstVal);
            colType = DataColumn::Type::Integer;
          }
        } catch (...) {
          colType = DataColumn::Type::String;
        }
      }
      table->addColumn(colNames[c], colType);
    }

    // Pass 2: Extract data by column index
    for (size_t r = 0; r < doc.GetRowCount(); ++r) {
      for (size_t c = 0; c < colCount; ++c) {
        DataColumn& dataCol = table->getColumn(colNames[c]);
        if (dataCol.getType() == DataColumn::Type::Float) {
          dataCol.push_back(doc.GetCell<float>(c, r));
        } else if (dataCol.getType() == DataColumn::Type::Integer) {
          dataCol.push_back(doc.GetCell<int>(c, r));
        } else {
          dataCol.push_back(doc.GetCell<std::string>(c, r));
        }
      }
    }
    
    fLogInfo("IOCSV initialized columnar DataTable with {} rows.", table->getRowCount());
    return table;
  } catch (const std::exception& e) {
    fLogSevere("IOCSV parsing failed: {}", e.what());
    return nullptr;
  }
}

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
  
  bool hasHeader = (keys.get("header", "true") == "true");
  bool successful = false;
  
  try {
    std::ofstream out(filename, std::ofstream::out);
    if (out.is_open()) {
      const auto& colNames = table->getColumnNames();
      
      // Only write the header row if requested
      if (hasHeader) {
        for (size_t i = 0; i < colNames.size(); ++i) {
          out << colNames[i];
          if (i < colNames.size() - 1) out << ",";
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
            std::string val = dataCol.getStringVector()[r];
            if (val.find(',') != std::string::npos || val.find('"') != std::string::npos) {
              out << "\"" << val << "\"";
            } else {
              out << val;
            }
          }
          if (c < colNames.size() - 1) out << ",";
        }
        out << "\n";
      }
      out.close();
      successful = true;
    } else {
      fLogSevere("Couldn't open {} for writing.", filename);
    }
  } catch (const std::exception& e) {
    fLogSevere("Failed to write {}, reason: {}", filename, e.what());
  }
  
  if (successful) {
    successful = postWriteProcess(filename, keys);
  }
  if (successful) {
    showFileInfo("CSV writer: ", keys);
  }
  return successful;
}
