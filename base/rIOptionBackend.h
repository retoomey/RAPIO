#pragma once

#include <string>
#include <string_view>
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
  boolean(std::string_view opt, std::string_view usage) = 0;
  virtual void
  require(std::string_view opt, std::string_view exampleArg, std::string_view usage) = 0;
  virtual void
  optional(std::string_view opt, std::string_view defaultValue, std::string_view usage) = 0;
  virtual void
  setDefaultValue(std::string_view opt, std::string_view defaultValue) = 0;
  virtual void
  addGroup(std::string_view sourceopt, std::string_view group) = 0;
  virtual void
  addSuboption(std::string_view sourceopt, std::string_view opt, std::string_view description) = 0;
  virtual void
  setHidden(std::string_view sourceopt) = 0;
  virtual void
  addAdvancedHelp(std::string_view sourceopt, std::string_view help) = 0;
  virtual void
  setEnforcedSuboptions(std::string_view key, bool flag) = 0;

  virtual std::string
  getString(std::string_view opt) const = 0;
  virtual bool
  getBoolean(std::string_view opt) const = 0;
  virtual float
  getFloat(std::string_view opt) const = 0;
  virtual double
  getDouble(std::string_view opt) const = 0;
  virtual int
  getInteger(std::string_view opt) const = 0;

  virtual bool
  isInSuboptions(std::string_view key) const = 0;
  virtual bool
  isParsed(std::string_view key) const = 0;
  virtual bool
  wantAdvancedHelp(std::string_view sourceopt) const = 0;

  virtual void
  storeParsedArg(std::string_view name, std::string_view value, const bool enforceStrict = true,
    const bool fromiconfig = false) = 0;

  virtual void
  setIsProcessed() = 0;
  virtual bool
  getIsProcessed() const = 0;

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
