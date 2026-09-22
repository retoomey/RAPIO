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
using DataValue = std::variant<int, float, std::string>;

/**
 * @brief Lightweight wrapper for dynamic columnar growth.
 *
 * Encapsulates a std::variant of vectors to provide contiguous column
 * storage for integers, floats, or strings. Use visitData() for performance-critical
 * loops, or getCellAsX() methods for convenient, type-coerced individual access.
 */
class DataColumn {
public:
  enum class Type { Integer, Float, String };

  DataColumn(Type t);

  void
  push_back(int val);

  void
  push_back(float val);

  void
  push_back(const std::string& val);

  size_t
  size() const;

  Type
  getType() const { return myType; }

  const std::vector<int>&
  getIntVector() const;

  const std::vector<float>&
  getFloatVector() const;

  const std::vector<std::string>&
  getStringVector() const;

  /**
   * @brief Convenience helper to coerce a cell's value to a string.
   * Useful for I/O and text formatting.
   *
   * @param row The zero-based row index.
   * @return The string representation of the cell.
   */
  std::string
  getCellAsString(size_t row) const;

  /**
   * @brief Convenience helper to safely extract a cell's value as a double.
   * Casts numeric types and attempts to parse strings.
   *
   * @param row The zero-based row index.
   * @return The float representation of the cell, or 0.0 on string parse failure.
   */
  float
  getCellAsFloat(size_t row) const;

  /**
   * @brief Convenience helper to safely extract a cell's value as an integer.
   * Casts numeric types and attempts to parse strings.
   *
   * @param row The zero-based row index.
   * @return The integer representation of the cell, or 0 on string parse failure.
   */
  int
  getCellAsInt(size_t row) const;

  /**
   * @brief Applies a visitor to the underlying std::variant vector (const).
   *
   * Enables column-level type checking (resolving the variant once)
   * rather than row-by-row checking, eliminating loop branch overhead.
   *
   * @param vis A callable visitor (e.g., a generic lambda).
   * @return The result of the visitor execution.
   */
  template <typename Visitor>
  auto
  visitData(Visitor&& vis) const
  {
    return std::visit(std::forward<Visitor>(vis), myData);
  }

  /**
   * @brief Applies a visitor to the underlying std::variant vector (mutable).
   */
  template <typename Visitor>
  auto
  visitData(Visitor&& vis)
  {
    return std::visit(std::forward<Visitor>(vis), myData);
  }

private:
  Type myType;
  std::variant<std::vector<int>, std::vector<float>, std::vector<std::string> > myData;
};

/**
 * @brief Replaces the PTreeData-inherited table with columnar storage.
 *
 * Manages a collection of DataColumns mapped by string names, ensuring
 * rectangular data integrity for tabular datasets.
 */
class DataTable : public DataType {
public:
  DataTable(){ myDataType = "DataTable"; }

  void
  addColumn(const std::string& name, DataColumn::Type type);

  const std::vector<std::string>&
  getColumnNames() const { return myColumnOrder; }

  void
  setSchema(const std::vector<std::pair<std::string, DataColumn::Type> >& schema);

  void
  addRow(const std::map<std::string, DataValue>& rowData);

  // Feel like this API is dangerous.  Can lead to jagged tables
  DataColumn&
  getColumn(const std::string& name);

  size_t
  getRowCount() const;

  bool
  validateRectangularShape() const;

private:
  std::vector<std::string> myColumnOrder;
  std::map<std::string, std::shared_ptr<DataColumn> > myColumns;
};

/**
 * @brief Specializer for XML/JSON to downcast PTreeData to DataTable.
 */
class PTreeDataSpecializer : public IOSpecializer {
public:
  virtual std::shared_ptr<DataType>
  downcastPTreeDataType(IOConfig& config, std::shared_ptr<DataType> in) = 0;

  virtual std::shared_ptr<DataType>
  read(IOConfig& config) override { return nullptr; }

  virtual bool
  write(std::shared_ptr<DataType> dt, IOConfig& keys) override { return false; }
};

/**
 * @brief Implementation of PTreeDataSpecializer specifically for DataTables.
 */
class PTreeDataTable : public PTreeDataSpecializer {
public:
  virtual std::shared_ptr<DataType>
  downcastPTreeDataType(IOConfig& config, std::shared_ptr<DataType> in) override;
};
} // namespace rapio
