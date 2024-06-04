#pragma once

#include <rDataType.h>
#include <rDataGrid.h>

// Maybe specializer should be separate?
#include <rIODataType.h>

namespace rapio {
/** DataTable specializer for XML/JSON to datatype */
class PTreeDataTable : public IOSpecializer
{
  /** Write a given DataType */
  virtual bool
  write(std::shared_ptr<DataType>     dt,
    std::map<std::string, std::string>& keys) override;

  /** Read a DataType from given information */
  virtual std::shared_ptr<DataType>
  read(
    std::map<std::string, std::string>& keys,
    std::shared_ptr<DataType>         optionalOriginal) override;
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
