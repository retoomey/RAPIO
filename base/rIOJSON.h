#pragma once

#include <rIO.h>
#include <rData.h>
#include <rIODataType.h>
#include <rPTreeData.h>
#include <rDataGrid.h>
#include <memory>

#include <ostream>
#include <boost/property_tree/ptree.hpp>

namespace rapio {
class URL;

/** Simple routines for reading/writing JSON documents */
class IOJSON : public IODataType {
public:

  /** Help for JSON */
  virtual std::string
  getHelpString(const std::string& key) override;

  // READING ------------------------------------------------------------

  /** Read from a buffer as XML data */
  virtual std::shared_ptr<DataType>
  createDataTypeFromBuffer(std::vector<char>& buffer) override;

  /** IODataType factory reader call */
  virtual std::shared_ptr<DataType>
  createDataType(const std::string& params) override;

protected:
  /** Read from a buffer to a PTreeData object  */
  static std::shared_ptr<PTreeData>
  readPTreeDataBuffer(std::vector<char>& buffer);

public:
  // WRITING ------------------------------------------------------------

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    std::map<std::string, std::string>     & keys
  ) override;

  /** Write data type to a buffer */
  virtual size_t
  encodeDataTypeBuffer(std::shared_ptr<DataType> dt, std::vector<char>& buffer) override;

  /** Write property tree to a memory buffer */
  static size_t
  writePTreeDataBuffer(std::shared_ptr<PTreeData> d, std::vector<char>& buf);

protected:
  /** Write property tree to URL */
  static bool
  writeURL(
    const URL                  & path,
    std::shared_ptr<PTreeData> tree,
    bool                       shouldIndent = true,
    bool                       console      = false);
};
}
