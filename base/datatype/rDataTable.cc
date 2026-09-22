#include "rDataTable.h"
#include <rError.h>
#include <rStrings.h>

using namespace rapio;

// ---------------------------------------------------------
// DataColumn Implementation
// ---------------------------------------------------------
DataColumn::DataColumn(Type t) : myType(t)
{
  if (t == Type::Integer) { myData = std::vector<int>(); } else if (t == Type::Float) {
    myData = std::vector<float>();
  } else { myData = std::vector<std::string>(); }
}

void DataColumn::push_back(int val){ std::get<std::vector<int> >(myData).push_back(val); }

void DataColumn::push_back(float val){ std::get<std::vector<float> >(myData).push_back(val); }

void DataColumn::push_back(const std::string& val){ std::get<std::vector<std::string> >(myData).push_back(val); }

size_t
DataColumn::size() const
{
  return std::visit([](auto&& vec) {
    return vec.size();
  }, myData);
}

const std::vector<int>&
DataColumn::getIntVector() const { return std::get<std::vector<int> >(myData); }

const std::vector<float>&
DataColumn::getFloatVector() const { return std::get<std::vector<float> >(myData); }

const std::vector<std::string>&
DataColumn::getStringVector() const { return std::get<std::vector<std::string> >(myData); }

std::string
DataColumn::getCellAsString(size_t row) const
{
  switch (myType) {
      case Type::String:
        return getStringVector()[row];

      case Type::Float:
        return std::to_string(getFloatVector()[row]);

      case Type::Integer:
        return std::to_string(getIntVector()[row]);

      default:
        return "";
  }
}

float
DataColumn::getCellAsFloat(size_t row) const
{
  switch (myType) {
      case Type::Float:
        return getFloatVector()[row];

      case Type::Integer:
        return static_cast<float>(getIntVector()[row]);

      case Type::String:
        try { return std::stof(getStringVector()[row]);
        } // Note: stof instead of stod
        catch (...) { return 0.0f;
        }
      default:
        return 0.0f;
  }
}

int
DataColumn::getCellAsInt(size_t row) const
{
  switch (myType) {
      case Type::Integer:
        return getIntVector()[row];

      case Type::Float:
        return static_cast<int>(getFloatVector()[row]);

      case Type::String:
        try { return std::stoi(getStringVector()[row]);
        }
        catch (...) { return 0;
        }
      default:
        return 0;
  }
}

// ---------------------------------------------------------
// DataTable Implementation
// ---------------------------------------------------------

void
DataTable::setSchema(const std::vector<std::pair<std::string, DataColumn::Type> >& schema)
{
  // Blanket replace any existing schema and data
  myColumnOrder.clear();
  myColumns.clear();

  myColumnOrder.reserve(schema.size());

  for (const auto& [name, type] : schema) {
    myColumnOrder.push_back(name);
    myColumns[name] = std::make_shared<DataColumn>(type);
  }
}

void
DataTable::addRow(const std::map<std::string, DataValue>& rowData)
{
  for (const auto& colName : myColumnOrder) {
    auto& col   = *myColumns[colName];
    auto dataIt = rowData.find(colName);

    if (dataIt != rowData.end()) {
      std::visit([&col](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        // Safely route the variant value to the correct column type
        if (col.getType() == DataColumn::Type::Float) {
          if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double> ) {
            col.push_back(static_cast<float>(arg));
          } else if constexpr (std::is_same_v<T, int> ) { col.push_back(static_cast<float>(arg)); } else {
            col.push_back(static_cast<float>(Constants::MissingData));
          }
        } else if (col.getType() == DataColumn::Type::Integer) {
          if constexpr (std::is_same_v<T, int> ) {
            col.push_back(arg);
          } else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double> ) {
            col.push_back(static_cast<int>(arg));
          } else { col.push_back(static_cast<int>(Constants::MissingData)); }
        } else if (col.getType() == DataColumn::Type::String) {
          if constexpr (std::is_same_v<T, std::string> ) { col.push_back(arg); } else {
            col.push_back(std::to_string(arg));
          }
        }
      }, dataIt->second);
    } else {
      // Key missing from the rowData map? Automatically pad to maintain rectangularity
      if (col.getType() == DataColumn::Type::Float) {
        col.push_back(static_cast<float>(Constants::MissingData));
      } else if (col.getType() == DataColumn::Type::Integer) {
        col.push_back(static_cast<int>(Constants::MissingData));
      } else {
        col.push_back(std::string("-99900"));
      }
    }
  }
} // DataTable::addRow

void
DataTable::addColumn(const std::string& name, DataColumn::Type type)
{
  if (myColumns.find(name) == myColumns.end()) {
    myColumnOrder.push_back(name);
    myColumns[name] = std::make_shared<DataColumn>(type);
  }
}

DataColumn&
DataTable::getColumn(const std::string& name)
{
  if (myColumns.find(name) == myColumns.end()) {
    throw std::runtime_error("Column not found: " + name);
  }
  return *myColumns[name];
}

size_t
DataTable::getRowCount() const
{
  if (myColumnOrder.empty()) { return 0; }
  return myColumns.at(myColumnOrder.front())->size();
}

bool
DataTable::validateRectangularShape() const
{
  if (myColumnOrder.empty()) { return true; }
  size_t expected = getRowCount();

  for (const auto& pair : myColumns) {
    if (pair.second->size() != expected) { return false; }
  }
  return true;
}

// ---------------------------------------------------------
// PTreeDataTable Implementation
// ---------------------------------------------------------
std::shared_ptr<DataType>
PTreeDataTable::downcastPTreeDataType(IOConfig& keys, std::shared_ptr<DataType> dt)
{
  std::shared_ptr<PTreeData> xml = std::dynamic_pointer_cast<PTreeData>(dt);

  if (!xml) { return nullptr; }

  auto table     = std::make_shared<DataTable>();
  auto datatable = xml->getTree()->getChildOptional("datatable");

  // Migrate metadata exactly as before
  if (datatable != nullptr) {
    auto datatype = datatable->getChildOptional("datatype");
    if (datatype != nullptr) {
      try {
        const auto theDataType = datatype->getAttr("name", std::string(""));
        table->setTypeName(theDataType);

        auto time        = datatype->getChild("stref.time");
        const auto value = time.getAttr("value", (time_t) (0));
        table->setTime(Time::SecondsSinceEpoch(value));

        auto lat = datatype->getChild("stref.location.lat");
        auto lon = datatype->getChild("stref.location.lon");
        auto ht  = datatype->getChild("stref.location.ht");
        table->setLocation(LLH(
            lat.getAttr("value", double(0)),
            lon.getAttr("value", double(0)),
            ht.getAttr("value", double(0))
        ));
      } catch (const std::exception& e) {
        fLogSevere("Tried to read stref tag in datatable and failed: {}", e.what());
      }
    }
  }

  // Iterate over data and dynamically map to string columns as a default safe fallback for XML
  auto items = xml->getTree()->getChildren("item");
  bool schemaInitialized = false;

  for (const auto& item : items) {
    auto attributes = item.getAttrMap();
    if (!schemaInitialized) {
      for (const auto& kv : attributes) {
        table->addColumn(kv.first, DataColumn::Type::String);
      }
      schemaInitialized = true;
    }
    for (const auto& kv : attributes) {
      table->getColumn(kv.first).push_back(kv.second);
    }
  }

  return table;
} // PTreeDataTable::downcastPTreeDataType
