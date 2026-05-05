#pragma once

#include <string>
#include <vector>
#include "rHelpFormatter.h" // Requires the HelpEntry DTO


namespace rapio {
/**
 * @class IOptionBackend
 * @brief Abstract interface for command-line argument parsing backends.
 * * @details This interface acts as a bridge for the underlying argument
 * parsing implementation.
 *
 * Having subclasses of this would allow getting parameters from other sources,
 * such as a command server, Boost options, our simple OptionList or CLI11, etc.
 *
 * @author Robert Toomey
 */
class IOptionBackend {
public:
  struct SeparatedArgs {
    std::vector<std::string> flags;       // The -args and their values
    std::vector<std::string> positionals; // The leftovers
  };

  virtual
  ~IOptionBackend() = default;

  virtual SeparatedArgs
  separateArguments(const std::vector<std::string>& args) const = 0;
  virtual bool
  processArgs(const std::vector<std::string>& args) = 0;
  virtual std::vector<std::string>
  getPositionalArgs() const = 0;

  virtual void
  boolean(const std::string& opt, const std::string& usage) = 0;
  virtual void
  require(const std::string& opt, const std::string& exampleArg, const std::string& usage) = 0;
  virtual void
  optional(const std::string& opt, const std::string& defaultValue, const std::string& usage) = 0;
  virtual void
  setDefaultValue(const std::string& opt, const std::string& defaultValue) = 0;
  virtual void
  addGroup(const std::string& sourceopt, const std::string& group) = 0;
  virtual void
  addSuboption(const std::string& sourceopt, const std::string& opt, const std::string& description) = 0;
  virtual void
  setHidden(const std::string& sourceopt) = 0;
  virtual void
  addAdvancedHelp(const std::string& sourceopt, const std::string& help) = 0;
  virtual void
  setEnforcedSuboptions(const std::string& key, bool flag) = 0;

  virtual std::string
  getString(const std::string& opt) = 0;
  virtual bool
  getBoolean(const std::string& opt) = 0;
  virtual float
  getFloat(const std::string& opt) = 0;
  virtual int
  getInteger(const std::string& opt) = 0;

  virtual bool
  isInSuboptions(const std::string& key) = 0;
  virtual bool
  isParsed(const std::string& key) = 0;
  virtual bool
  wantAdvancedHelp(const std::string& sourceopt) = 0;

  virtual void
  storeParsedArg(const std::string& name, const std::string& value, const bool enforceStrict = true,
    const bool fromiconfig = false) = 0;

  virtual void
  setIsProcessed() = 0;
  virtual bool
  getIsProcessed() = 0;

  virtual void
  setHelpFields(const std::vector<std::string>& list) = 0;
  virtual std::vector<HelpEntry>
  exportHelpData() const = 0;
  virtual std::vector<HelpEntry>
  getMissingRequired() const = 0;
  virtual std::vector<HelpEntry>
  getInvalidSuboptions() const = 0;
  virtual std::vector<HelpEntry>
  getUnrecognizedOptions() const = 0;
  virtual std::vector<std::string>
  getRequestedAdvancedHelp() const = 0;
  virtual bool
  isHelpHiddenRequested() const = 0;
  virtual void
  addPostLoadedHelp() = 0;
  virtual void
  initToSettings() = 0;
};
} // namespace rapio
