#pragma once

#include "rIODataType.h"
#include "rDataGrid.h"
#include "rIO.h"

extern "C" {
#include <grib2.h>
}

#include <iomanip>

namespace rapio {

/** Worker classes that handle read and write of image types. */
class ImageType : public IO {
public:
  /** Write DataType from given ncid (subclasses) */
  virtual bool
  write(int                            ncid,
    std::shared_ptr<DataType>          dt,
    std::shared_ptr<DataFormatSetting> dfs) = 0;

  virtual std::shared_ptr<DataType>
  read(
    const int ncid,
    const URL & loc
  ) = 0;
};

/**
 * The base class of all image formatters.
 *
 * @author Robert Toomey
 */
class IOImage : public IODataType {
public:

  // Registering of classes ---------------------------------------------
  virtual void
  initialize() override;

  /** Use this to introduce new subclasses. */
  static void
  introduce(const std::string   & dataType,
    std::shared_ptr<ImageType> new_subclass);

  static std::shared_ptr<ImageType>
  getIOImage(const std::string& type);

  // READING ------------------------------------------------------------
  //

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(const URL& path) override;

  /** Do a full read from a param list */
  static std::shared_ptr<DataType>
  readImageDataType(const URL& path);

  // WRITING ------------------------------------------------------------

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    const URL                              & path,
    std::shared_ptr<DataFormatSetting>     dfs) override;

  virtual
  ~IOImage();
};
}
