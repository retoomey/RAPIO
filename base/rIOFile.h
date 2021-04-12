#pragma once

#include <rIO.h>
#include <rData.h>
#include <rIODataType.h>
#include <rPTreeData.h>
#include "rDataGrid.h"
#include <memory>

namespace rapio {
class URL;

/** Simple routines for reading/writing XML documents */
class IOFile : public IODataType {
public:

  /** Help for XML */
  virtual std::string
  getHelpString(const std::string& key) override;

  // READING ------------------------------------------------------------

  /** Read from a buffer as XML data */
  virtual std::shared_ptr<DataType>
  createDataTypeFromBuffer(std::vector<char>& buffer) override;

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(const std::string& params) override;

public:
  // WRITING ------------------------------------------------------------

  /** Our write out sends data onto a proxy based on file suffix */
  virtual bool
  writeout(std::shared_ptr<DataType> dt, const std::string& outputinfo,
    bool generateFileName,
    std::vector<Record>              & records,
    const std::string& factory = "",
    const std::map<std::string, std::string>& outputParams = std::map<std::string, std::string>()) override;

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    const std::string                      & params,
    std::shared_ptr<PTreeNode>             dfs,
    bool                                   directFile,
    // Output for notifiers
    std::vector<Record>                    & records
  ) override;

  /** Write data type to a buffer */
  virtual size_t
  encodeDataTypeBuffer(std::shared_ptr<DataType> dt, std::vector<char>& buffer) override;
};
}
