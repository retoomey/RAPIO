#pragma once

#include "rIODataType.h"

namespace rapio {
/**
 * The base class of all Grib/2/3/etc formatters.
 *
 * @author Robert Toomey
 */
class IOGrib : public IODataType {
public:

  static void
  test();

  // Registering of classes ---------------------------------------------

  /** Introduce self into factories */
  static void
  introduceSelf();

  // READING ------------------------------------------------------------
  //

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createObject(const std::vector<std::string>&) override;

  /** Do a full read from a param list */
  static std::shared_ptr<DataType>
  readGribDataType(const std::vector<std::string>& args);

  // WRITING ------------------------------------------------------------

  /** Encode a DataType for writing */
  static std::string
  writeGribDataType(const rapio::DataType& dt,
    const std::string                    & dir,
    std::shared_ptr<DataFormatSetting>   dfs,
    std::vector<Record>                  & records);

  // Virtual functions for DataWriter calls....
  std::string
  encode(const rapio::DataType         & dt,
    const std::string                  & directory,
    std::shared_ptr<DataFormatSetting> dfs,
    std::vector<Record>                & records) override;

  virtual
  ~IOGrib();

protected:

public:

private:
};
}
