#pragma once

#include "rIODataType.h"
#include "rDataGrid.h"
#include "rMultiDataType.h"

#include <iomanip>

namespace rapio {
/** Worker classes that handle read and write of particular DataType */
class ImageType {
public:
  /** Write DataType from given ncid (subclasses) */
  virtual bool
  write(int                    ncid,
    std::shared_ptr<DataType>  dt,
    std::shared_ptr<PTreeNode> dfs) = 0;

  /** Read DataTYpe from given ncid */
  virtual std::shared_ptr<DataType>
  read(
    const int ncid,
    const URL & loc
  ) = 0;

  /** Abstract class should have a virtual destructor */
  virtual ~ImageType(){ }
};

/**
 * The base class of all image formatters.
 *
 * @author Robert Toomey
 */
class IOImage : public IODataType {
public:

  /** Help for ioimage module */
  virtual std::string
  getHelpString(const std::string& key) override;

  // Registering of classes ---------------------------------------------
  virtual void
  initialize() override;

  /** Use this to introduce new subclasses. */
  static void
  introduce(const std::string  & dataType,
    std::shared_ptr<ImageType> new_subclass);

  static std::shared_ptr<ImageType>
  getIOImage(const std::string& type);

  // READING ------------------------------------------------------------
  //

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(const std::string& params) override;

  /** Do a full read from a param list */
  static std::shared_ptr<DataType>
  readImageDataType(const URL& path);

  // WRITING ------------------------------------------------------------

  /** Write our raw mrms data tile (for the imgui) */
  bool
  writeMRMSTile(MultiDataType& mdt, const std::string& filename, ProjLibProject& proj,
    size_t rows, size_t cols, double top, double left,
    double deltaLat, double deltaLon, bool transform);

  /** Write a png, jpeg, etc. depending on filename suffix using Magick */
  bool
  writeMAGICKTile(MultiDataType& mdt, const std::string& filename, ProjLibProject& proj,
    size_t rows, size_t cols, double top, double left,
    double deltaLat, double deltaLon, bool transform);

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    std::map<std::string, std::string>     & keys
  ) override;

  virtual
  ~IOImage();
};
}
