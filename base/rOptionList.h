#pragma once

#include <rIOptionBackend.h>
#include <rOption.h>
#include <rHelpFormatter.h>

#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include <map>

namespace rapio {
class LLCoverageArea;

/* OptionList handles a list of named options
 * Options are added first.  Then someone parses/updates the options.
 * Undeclared options end up in the unused.  Once parsing done,
 * setProcessed is called.
 *
 * @see RAPIOOptions
 * @author Robert Toomey
 * @ingroup rapio_algorithm
 * @brief Stores a list or parameter options for algorithm.
 *
 */
class OptionList : public IOptionBackend {
public:

  /** Create an empty option list */
  OptionList();

  /** Construct default options */
  OptionList(std::string_view base = "Program");

  /** Destroy an option list */
  ~OptionList(){ }

  // Definition & Binding ---------------------------------------

  /** Create a boolean argument */
  void
  boolean(std::string_view opt,
    std::string_view       usage) override;

  /** Create a required argument */
  void
  require(std::string_view opt,
    std::string_view       exampleArg,
    std::string_view       usage) override;

  /** Create an optional argument.  Optional have default values */
  void
  optional(std::string_view opt,
    std::string_view        defaultValue,
    std::string_view        usage) override;

  /** Turn a required into an optional by providing a default value.
   * This can be called in declareOptions in order to turn a required value
   * into an optional one.  We use this in rWatch for example since o becomes optional.
   * The other way to do it could be a larger algorithm/program class tree.
   * We can also replace the default value of an optional too. */
  void
  setDefaultValue(std::string_view opt,
    std::string_view               defaultValue) override;

  /** Add a group to option.  */
  void
  addGroup(std::string_view sourceopt,
    std::string_view        group) override;

  /** Add a suboption.  Once added, ONLY these values are allowed to be passed
   * in.  */
  void
  addSuboption(std::string_view sourceopt,
    std::string_view            opt,
    std::string_view            description) override;

  /** Set hidden on an option */
  void
  setHidden(std::string_view sourceopt) override;

  /** Add advanced help to an option.  */
  void
  addAdvancedHelp(std::string_view sourceopt,
    std::string_view               help) override;

  /** Make this option enforce suboptions or not */
  void
  setEnforcedSuboptions(std::string_view key, bool flag) override;

  // Retrieval --------------------------------------------------

  /** Fetch an option as a string */
  std::string
  getString(std::string_view opt) const override;

  /** Fetch an option as a boolean */
  bool
  getBoolean(std::string_view opt) const override;

  /** Fetch an option as a float, or 0 if invalid */
  float
  getFloat(std::string_view opt) const override;

  /** Fetch an option as a double, or 0 if invalid */
  double
  getDouble(std::string_view opt) const override;

  /** Fetch an option as an integer, or 0 if invalid */
  int
  getInteger(std::string_view opt) const override;

  /** Does this option match one of its suboption list? */
  bool
  isInSuboptions(std::string_view key) const override;

  /** Return true if this option name is parsed */
  bool
  isParsed(std::string_view key) const override;

  /** Do we want advanced help for this option? */
  bool
  wantAdvancedHelp(std::string_view sourceopt) const override;

  /** Store a parsed argument value, from XML or command line */
  void
  storeParsedArg(std::string_view name,
    std::string_view              value,
    const bool                    enforceStrict = true,
    const bool                    fromiconfig   = false) override;

private:

  /** Force add given option to map */
  Option *
  addOption(Option o);

  /** Make an option */
  Option *
  makeOption(bool    required,
    bool             boolean,
    bool             system,
    std::string_view opt,
    std::string_view name,
    std::string_view usage,
    std::string_view extra);

  /** Return sorted options */
  void
  sortOptions(std::vector<Option *>&,
    OptionFilter& a);

  /** Return option with this key */
  const Option *
  getOption(std::string_view key) const;

  /** Return option with this key */
  Option *
  getOption(std::string_view key);

public:

  /** Set if processed, this allows get value methods to work */
  void
  setIsProcessed() override { isProcessed = true; }

  /** Get if processed, this allows get value methods to work */
  bool
  getIsProcessed() const override { return isProcessed; }

  void
  setHelpFields(const std::vector<std::string>& list) override;

  // --- Decoupled Parsing Implementations ---
  virtual SeparatedArgs
  separateArguments(const std::vector<std::string>& args) const override;
  virtual bool
  processArgs(const std::vector<std::string>& args) override;
  virtual std::vector<std::string>
  getPositionalArgs() const override { return myPositionalArgs; }

public:

  /** Help exporter.  This will allow help to work independently
   * of the underlying library handling options */
  std::vector<HelpEntry>
  exportHelpData() const override;

  std::vector<HelpEntry>
  getMissingRequired() const override;

  std::vector<HelpEntry>
  getInvalidSuboptions() const override;

  std::vector<HelpEntry>
  getUnrecognizedOptions() const override;

private:

  /** Expand main arguments to a fully paired arg/value list */
  std::vector<std::string>
  expandArgs(const std::vector<std::string>& args);

  /** Helper to convert internal Option to generic DTO */
  HelpEntry
  createHelpEntry(const Option& o) const;

protected:

  /** Advanced Help filter list of option keys */
  std::vector<std::string> myHelpOptions;

  /** Advanced Help filter hidden or not */
  bool myHelpOptionsHidden;

  /** Have arguments been sucessfully processed? */
  bool isProcessed;

  /** Collection of stored valid options by name */
  std::map<std::string, Option, std::less<> > optionMap;

  /** Collection of stored invalid options by name.  Should this
   * functionality be here or somewhere else? */
  std::map<std::string, Option, std::less<> > unusedOptionMap;

  /** Keep track of maximum argument width */
  static unsigned int max_arg_width;

  /** Keep track of maximum name width */
  static unsigned int max_name_width;

protected:

  /** Actual command line arguments */
  std::vector<std::string> myRawArgs;

  /** Positional args  */
  std::vector<std::string> myPositionalArgs;

public:

  /** Add advanced help for default options */
  void
  addPostLoadedHelp() override;

  /** Set anything from the global configuration settings */
  void
  initToSettings() override;

  std::vector<std::string>
  getRequestedAdvancedHelp() const override { return myHelpOptions; }

  bool
  isHelpHiddenRequested() const override { return myHelpOptionsHidden; }
};
}
