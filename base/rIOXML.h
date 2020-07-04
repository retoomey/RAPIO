#pragma once

#include <rIO.h>
#include <rData.h>
#include <rIODataType.h>
#include <rXMLData.h>
#include <memory>

#include <boost/property_tree/ptree.hpp>

namespace rapio {
class URL;

/** Simple routines for reading/writing XML documents */
class IOXML : public IODataType {
public:

  // READING ------------------------------------------------------------

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(const URL& path) override;

  /** Split an XML file that contains a <signed> signature
   * tag at the end of the file into message and signature parts*/
  static bool
  splitSignedXMLFile(const std::string& signedxml,
    std::string                       & outmsg,
    std::string                       & outkey);

  // WRITING ------------------------------------------------------------

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    const URL                              & path,
    std::shared_ptr<DataFormatSetting>     dfs) override;

  /** Write property tree to URL */
  static bool
  writeURL(
    const URL                & path,
    std::shared_ptr<XMLData> tree,
    bool                     shouldIndent = true,
    bool                     console = false);
};
}
