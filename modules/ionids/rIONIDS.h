#pragma once

#include <rIODataType.h>
#include "rBinaryIO.h"

namespace rapio {
class NIDSBlock {
public:
  /** Check block divider */
  void
  checkDivider(StreamBuffer& b)
  {
    short divider = b.readShort();

    if (divider != -1) {
      throw std::runtime_error("Block doesn't start with divider -1");
    }
  }

  /** Write block divider */
  void
  writeDivider(StreamBuffer& b)
  {
    b.writeShort(-1);
  }

  /** Read the block */
  virtual void
  read(StreamBuffer& b) = 0;

  /** Write the block */
  virtual void
  // write(StreamBuffer& b) = 0;
  write(StreamBuffer& b){ };
};

/**
 * The base class of all NIDS formatters.
 *
 */

class IONIDS : public IODataType {
public:

  /** Destructor */
  virtual
  ~IONIDS();

  /** Help for nids module */
  virtual std::string
  getHelpString(const std::string& key) override;

  // Registering of classes ---------------------------------------------

  virtual void
  initialize() override;

  // READING ------------------------------------------------------------

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(const std::string& params) override;

  // WRITING ------------------------------------------------------------

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    std::map<std::string, std::string>     & params
  ) override;

  void
  readHeaders(StreamBuffer& s);
};
}
