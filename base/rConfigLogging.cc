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
      auto flush = logging->getAttr("flush", int(900));
      Log::setLogFlushMilliSeconds(flush);

      // FIXME: We 'could' just make a language string and parse it,
      // which would allow any order, color, etc.  Might play with that
      // later, for now just toggle on off things
      auto color = logging->getAttr("color", std::string("true"));
      Log::setLogUseColors(color == "true");

      auto pattern = logging->getAttr("pattern", std::string(""));

      if (!pattern.empty()) {
        Log::setLogPattern(pattern);
      }

      #if 0
      // DFA count array for each token
      std::vector<size_t> ats;
      ats.resize(tokens.size());


      // For each character in pattern...
      std::string fill;
      for (auto s:pattern) {
        // For each token in list...
        fill += s;
        for (size_t i = 0; i < tokens.size(); i++) {
          auto& token = tokens[i];
          auto& at    = ats[i];
          // If character matches...progress token DFA forward...
          if (token[at] == s) {
            // Matched a token, remove from gathered string, this
            // will be the unmatched part filler before the token
            if (++at >= token.size()) {
              // Add one filler string to log
              Strings::removeSuffix(fill, token);
              if (!fill.empty()) {
                fillers.push_back(fill);
                outputtokens.push_back(-1);
                fill = "";
              }
              // Add one matched token to log and reset all DFAS
              outputtokens.push_back(i);
              for (auto& a:ats) {
                a = 0;
              }
              break; // important
            }
            // Otherwise that single token resets
          } else {
            at = 0; // token failed match, reset
          }
        }
      }

      // Verification/debugging if the code ever fails
      std::cout << ">>>\"";
      size_t fat = 0;
      for (auto a:outputtokens) {
        // std::cout << a << ":";
        if (a == -1) { // handle filler
          std::cout << fillers[fat++];
        } else { // handle token
          std::cout << "{" << tokens[a] << "}";
        }
      }
      std::cout << "\"<<<\n";
      std::cout << "filler size " << fillers.size() << " and token list " << outputtokens.size() << "\n";

      #endif // if 0
      // Thankfully we're not threaded, otherwise we'll need to start
      // adding locks for this stuff
      //       Log::fillers = fillers;
      //       Log::outputtokens = outputtokens;
    } else {
      LogSevere("No logging settings provided, will be using defaults\n");
    }
  }catch (const std::exception& e) {
    LogSevere("Error parsing logging settings\n");
  }
  return true;
} // ConfigLogging::readSettings
