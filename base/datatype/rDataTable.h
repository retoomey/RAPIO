#pragma once

#include <rDataType.h>
#include <rPTreeData.h>
#include <rIODataType.h>
#include <string>
#include <vector>
#include <variant>
#include <map>
#include <memory>
#include <stdexcept>

namespace rapio {

// Lightweight wrapper for dynamic columnar growth
class DataColumn {
public:
  enum class Type { Integer, Float, String };

  DataColumn(Type t);

  void push_back(int val);
  void push_back(float val);
  void push_back(const std::string& val);

  size_t size() const;
  Type getType() const { return myType; }

  const std::vector<int>& getIntVector() const;
  const std::vector<float>& getFloatVector() const;
  const std::vector<std::string>& getStringVector() const;

private:
  Type myType;
  std::variant<std::vector<int>, std::vector<float>, std::vector<std::string>> myData;
};

// Replaces the PTreeData-inherited table with columnar storage
class DataTable : public DataType {
public:
  DataTable() { myDataType = "DataTable"; }

  void addColumn(const std::string& name, DataColumn::Type type);
  
  const std::vector<std::string>& getColumnNames() const { return myColumnOrder; }
  DataColumn& getColumn(const std::string& name);

  size_t getRowCount() const;
  bool validateRectangularShape() const;

private:
  std::vector<std::string> myColumnOrder;
  std::map<std::string, std::shared_ptr<DataColumn>> myColumns;
};

// Specializer for XML/JSON to downcast PTreeData to DataTable
class PTreeDataSpecializer : public IOSpecializer {
public:
  virtual std::shared_ptr<DataType>
  downcastPTreeDataType(IOConfig& config, std::shared_ptr<DataType> in) = 0;

  virtual std::shared_ptr<DataType> read(IOConfig& config) override { return nullptr; }
  virtual bool write(std::shared_ptr<DataType> dt, IOConfig& keys) override { return false; }
};

class PTreeDataTable : public PTreeDataSpecializer {
public:
  virtual std::shared_ptr<DataType>
  downcastPTreeDataType(IOConfig& config, std::shared_ptr<DataType> in) override;
};

} // namespace rapio
