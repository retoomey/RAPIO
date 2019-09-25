#pragma once

#include <rIO.h>
#include <rData.h>
#include <memory>

#include <boost/property_tree/ptree.hpp>

namespace rapio {
class URL;

/** Simple routines for reading/writing XML documents */
class IOXML : public IO {
public:

  // Writing
  // --------------------------------------------------------------------

  /** Write property tree to URL */
  static bool
  writeURL(
    const URL                  & path,
    boost::property_tree::ptree& tree,
    bool                       shouldIndent = true);

  // Reading
  // --------------------------------------------------------------------

  /** Read property tree from URL */
  static std::shared_ptr<boost::property_tree::ptree>
  readURL(const URL& url);
};
}
