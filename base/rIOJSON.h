#pragma once

#include <rIO.h>
#include <rData.h>
#include <rIODataType.h>
#include <memory>

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

  /** Do a full read from a param list */
  static std::shared_ptr<DataType>
  readJSONDataType(const std::vector<std::string>& args);

  /** Read property tree from URL */
  static std::shared_ptr<boost::property_tree::ptree>
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
    const URL                  & path,
    boost::property_tree::ptree& tree,
    bool                       shouldIndent = true);
};
}
