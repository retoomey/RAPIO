#include "rConfigLogging.h"

#include "rError.h"
#include "rStrings.h"

using namespace rapio;

void
ConfigLogging::introduceSelf()
{
  std::shared_ptr<ConfigLogging> t = std::make_shared<ConfigLogging>();

  Config::introduce("logging", t);
}

bool
ConfigLogging::readSettings(std::shared_ptr<PTreeData> d)
{
  try{
    // This should be done by config higher 'up' I think...
    // as we add sections to global configuration
    // This should be first thing passed to us I think....
    auto topTree = d->getTree()->getChild("settings");
    auto logging = topTree.getChildOptional("logging");

    // Handle the PTreeNode we're given...
    if (logging != nullptr) {
      auto engine = logging->getAttr("use", std::string("cout"));
      Log::setLogEngine(engine);

      auto flush = logging->getAttr("flush", int(900));
      Log::setLogFlushMilliSeconds(flush);

      auto color = logging->getAttr("color", std::string("true"));
      Log::setLogUseColors(color == "true");

      auto hcolor = logging->getAttr("helpcolor", std::string("true"));
      Log::setHelpColors(hcolor == "true");

      auto pattern = logging->getAttr("pattern", std::string(""));

      if (!pattern.empty()) {
        Log::setLogPattern(pattern);
      }
    } else {
      fLogSevere("No logging settings provided, will be using defaults");
    }
  }catch (const std::exception& e) {
    fLogSevere("Error parsing logging settings");
  }
  return true;
} // ConfigLogging::readSettings
