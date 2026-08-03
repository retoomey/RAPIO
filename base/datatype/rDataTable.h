#pragma once

#include <rDataType.h>
#include <rDataGrid.h>

// Maybe specializer should be separate?
#include <rIODataType.h>

namespace rapio {
/** Special use in this case, we take a PTreeData
 * general object and try to specialize it into
 * something like a DataTable, etc.
 * This probably belongs somewhere else, but right
 * now we only specialize to a DataTable */
class PTreeDataSpecializer : public IOSpecializer
{
public:

  /** Specialize a PTreeData if possible.
   * Basically downcast to a specialized subclass */
  virtual std::shared_ptr<DataType>
  downcastPTreeDataType(IOConfig& config,
    std::shared_ptr<DataType> in) = 0;

  // Unused. deprecated hopefully ----------
  virtual std::shared_ptr<DataType>
  read(IOConfig& config)
  override { return nullptr; }

  virtual bool
  write(
    std::shared_ptr<DataType> dt,
    IOConfig                  & keys)
  override { return false; }
};

/** DataTable specializer for XML/JSON to datatype */
class PTreeDataTable : public PTreeDataSpecializer
{
public:
  /** Read a DataType from given information */
  virtual std::shared_ptr<DataType>
  downcastPTreeDataType(
    IOConfig& config,
    std::shared_ptr<DataType> in) override;
};

/** DataTable
 * Storage for the DataTable class from MRMS
 *
 * @author Robert Toomey */
class DataTable : public PTreeData {
public:

  /*** Specialize to a DataTable from a generic PTreeData */
  // DataTable(std::shared_ptr<PTreeData> rawdata)
  DataTable()
  {
    myDataType = "DataTable";
  }

  // FIXME:  Add API read/write methods to interacting with PTreeData that are
  // special for this type
};
}
