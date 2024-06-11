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
  PluginProductOutputFilter(const std::string& name) : RAPIOPlugin(name){ }

  /** Declare plugin. */
  static bool
  declare(RAPIOProgram * owner, const std::string& name = "O");

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
  declareProduct(const std::string& key, const std::string& help)
  {
    myKeys.push_back(key);
    myKeyHelp.push_back(help);
  }

  /** Is product with this key wanted? */
  virtual bool
  isProductWanted(const std::string& key);

  /** Resolve product name from the -O Key=Resolved pairs.*/
  virtual std::string
  resolveProductName(const std::string& key, const std::string& defaultName);

protected:

  /** List of keys for -O help */
  std::vector<std::string> myKeys;

  /** List of help for -O help */
  std::vector<std::string> myKeyHelp;
};
}
