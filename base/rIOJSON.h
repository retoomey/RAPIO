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

  // Registering of classes ---------------------------------------------

  /** Introduce self into factories */
  static void
  introduceSelf();

  // READING ------------------------------------------------------------
  //

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createObject(const std::vector<std::string>&) override;

  /** Read a JSON DataType from a param list */
  static std::shared_ptr<DataType>
  readJSONDataType(const std::vector<std::string>& args);

  /** Read JSON DataType from a given URL */
  static std::shared_ptr<JSONData>
  readURL(const URL& url);

  // WRITING ------------------------------------------------------------
  //
  // Virtual functions for DataWriter calls....

  /** Encode a DataType for writing */
  std::string
  encode(std::shared_ptr<DataType>     dt,
    const std::string                  & directory,
    std::shared_ptr<DataFormatSetting> dfs,
    std::vector<Record>                & records) override;

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
