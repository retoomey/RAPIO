#pragma once

#include <rIO.h>
#include <rData.h>
#include <rIODataType.h>

namespace rapio {
class URL;

/** Create fake DataTypes from records created by the fake
 * index.
 *
 * @author Robert Toomey
 */
class IOFakeDataType : public IODataType {
public:

  /** Help for XML */
  virtual std::string
  getHelpString(const std::string& key) override;

  /** Special setup of XML */
  virtual void
  initialize() override;

  // READING ------------------------------------------------------------

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(const std::string& params) override;

public:
  // WRITING ------------------------------------------------------------

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    std::map<std::string, std::string>     & keys
  ) override;
};
}
