#pragma once

#include <rConfig.h>

namespace rapio {
/** Holds loadable configuration settings for Logging */
class ConfigLogging : public ConfigType {
public:
  /** Virtual object from config map to static for clarity. */
  virtual bool
  readConfig(std::shared_ptr<PTreeData> d) override { return readSettings(d); }

  /** Introduce self to configuration */
  static void
  introduceSelf();

  /** Actual work of reading/checking settings */
  static bool
  readSettings(std::shared_ptr<PTreeData> );

private:
};
}
