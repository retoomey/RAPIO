#pragma once

#include <rConfig.h>
#include <rRecord.h>

namespace rapio {
/** Do the work of creating Records from xml, etc.
 * Having a dedicated class for Record reading functions allows have Records be non-virtual.
 * Since we read a ton of them this helps with ram/space*/
class ConfigRecord : public ConfigType {
public:

  /** Read param tag of record. */
  static void
  readParams(const std::string & params,
    const std::string          & changes,
    std::vector<std::string>   & v,
    const std::string          & indexPath);

  /** Record can read contents of (typically) an \<item\> tag and fill itself */
  static bool
  readXML(Record& rec, const PTreeNode& item,
    const std::string    &indexPath,
    size_t indexLabel);

  /** Dump XML to stream for a Record */
  static void
  constructXMLString(const Record& rec, std::ostream&, const std::string& indexPath);

  /** Record can generate file level meta data. Typically these go inside
   *  \<meta\> xml tags. */
  static void
  constructXMLMeta(const Record& rec, std::ostream&);
};
}
