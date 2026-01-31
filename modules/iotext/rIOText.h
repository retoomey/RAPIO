#pragma once

#include "rIODataType.h"

// #include <iomanip>

namespace rapio {
/**
 * Do text output for our various DataTypes
 * Some io modules like json, xml, etc. may also
 * do text outputs and better for reading/writing purposes.
 * For example, you want to write out json and read with Python,
 * in this case might want the IOJson instead.
 *
 * The main idea behind this module
 * is to have a dump tool for our supported DataType classes.
 * Bleh or we do a 'text' ability in each ioclass?
 * Humm how to text output IORaw?  Ok either we handle IORaw
 * here and dump it, or we put it into IORAW right?
 * Not sure on design here.
 *
 * Well do I want to general text output DataType...or do
 * I want to text output the raw files?  Guess that's the question.
 *
 * @author Robert Toomey
 */
class IOText : public IODataType {
public:

  /** Writing reference for submodules */
  static std::ostream * theFile;

  /** Help for ioimage module */
  virtual std::string
  getHelpString(const std::string& key) override;

  // Registering of classes ---------------------------------------------
  virtual void
  initialize() override;

  // READING ------------------------------------------------------------
  //

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(const std::string& params) override;

  /** Do a full read from a param list */
  static std::shared_ptr<DataType>
  readRawDataType(const URL& path);

  // WRITING ------------------------------------------------------------

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    std::map<std::string, std::string>     & keys
  ) override;

  virtual
  ~IOText();
};
}
