#pragma once

#include "rRAPIOPlugin.h"

#include <vector>

namespace rapio {
class RAPIOProgram;
class RAPIOOptions;

/** Subclass of plugin adding output filter ability (-O) */
class PluginProductOutputFilter : public RAPIOPlugin
{
public:

  /** Create a record filter plugin */
  PluginProductOutputFilter(std::string_view name) : RAPIOPlugin(name){ }

  /** Declare plugin. */
  static bool
  declare(RAPIOProgram * owner, std::string_view name = "O");

  /** Declare options for the plugin */
  virtual void
  declareOptions(RAPIOOptions& o) override;

  /** Declare advanced help for the plugin */
  virtual void
  addPostLoadedHelp(RAPIOOptions& o) override;

  /** Process our options */
  virtual void
  processOptions(RAPIOOptions& o) override;

  /** Add a static key for the -O help.  Note that keys can be static, such as
   * '2D' to refer to a class of product, or currently you can also use the
   * DataType typename as a dynamic key. */
  virtual void
  declareProduct(std::string_view key, std::string_view help)
  {
    myKeys.emplace_back(key);
    myKeyHelp.emplace_back(help);
  }

  /** Is product with this key wanted? */
  virtual bool
  isProductWanted(std::string_view key);

  /** Resolve product name from the -O Key=Resolved pairs.*/
  virtual std::string
  resolveProductName(std::string_view key, std::string_view defaultName);

protected:

  /** List of keys for -O help */
  std::vector<std::string> myKeys;

  /** List of help for -O help */
  std::vector<std::string> myKeyHelp;
};
}
