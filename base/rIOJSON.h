#pragma once

#include <rIO.h>
#include <rData.h>
#include <rIODataType.h>
#include <rJSONData.h>
#include <rDataGrid.h>
#include <memory>

#include <ostream>
#include <boost/property_tree/ptree.hpp>

namespace rapio {
class URL;

/** Simple routines for reading/writing XML documents */
class IOJSON : public IODataType {
public:

  // READING ------------------------------------------------------------
  //

  /** IODataType factory reader call */
  virtual std::shared_ptr<DataType>
  createDataType(const URL& path) override;

  // WRITING ------------------------------------------------------------

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    const URL                              & path,
    std::shared_ptr<DataFormatSetting>     dfs) override;

  /** Write property tree to URL */
  static bool
  writeURL(
    const URL                 & path,
    std::shared_ptr<JSONData> tree,
    bool                      shouldIndent = true,
    bool                      console = false);

  /** Write to a stream useful for debugging */

  /*static bool
   * writeStream(
   * std::ostream         & stream,
   * std::shared_ptr<JSONData>  tree,
   * bool                       shouldIndent = true);
   */

  /** Set netcdf attributes from a data attribute list */
  static
  void
  setAttributes(std::shared_ptr<JSONData> json, std::shared_ptr<DataAttributeList> list);

  /** Create JSON tree from a data grid */
  static
  std::shared_ptr<JSONData>
  createJSON(std::shared_ptr<DataGrid> datagrid);
};
}
