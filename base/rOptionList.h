#pragma once

#include <rOption.h>

#include <string>
#include <vector>
#include <map>

namespace rapio {
/* OptionList handles a list of named options
 * Options are added first.  Then someone parses/updates the options.
 * Undeclared options end up in the unused.  Once parsing done,
 * setProcessed is called.
 *
 * @see RAPIOOptions
 * @author Robert Toomey
 */
class OptionList : public Algorithm {
public:

  /** Create an empty option list */
  OptionList();

  /** Destory an option list */
  ~OptionList(){ }

  /** Collection of stored valid options by name */
  std::map<std::string, Option> optionMap;

  /** Collection of stored invalid options by name.  Should this
   * functionality be here or somewhere else? */
  std::map<std::string, Option> unusedOptionMap;

  /** Keep track of maximum argument width */
  static unsigned int max_arg_width;

  /** Keep track of maximum name width */
  static unsigned int max_name_width;

  // Create add option value by key -----------------------------

  /** Create a boolean argument */
  Option *
  boolean(const std::string& opt,
    const std::string      & usage);

  /** Create a required argument */
  Option *
  require(const std::string& opt,
    const std::string      & exampleArg,
    const std::string      & usage);

  /** Create an optional argument.  Optional have default values */
  Option *
  optional(const std::string& opt,
    const std::string       & defaultValue,
    const std::string       & usage);

  /** Return sorted options */
  void
  sortOptions(std::vector<Option>&,
    OptionFilter& a);

  // Access option value by key -----------------------------

  /** Fetch an option as a string */
  std::string
  getString(const std::string& opt);

  /** Fetch an option as a boolean */
  bool
  getBoolean(const std::string& opt);

  /** Fetch an option as a float, or 0 if invalid */
  float
  getFloat(const std::string& opt);

  /** Fetch an option as an integer, or 0 if invalid */
  int
  getInteger(const std::string& opt);

  /** Make this option enforce suboptions or not */
  void
  setEnforcedSuboptions(const std::string& key, bool flag);

  /** Does this option match one of its suboption list? */
  bool
  isInSuboptions(const std::string& key);

  /** Add a suboption.  Once added, ONLY these values are allowed to be passed
   * in.  */
  void
  addSuboption(const std::string& sourceopt,
    const std::string           & opt,
    const std::string           & description);

  /** Add a group to option.  */
  void
  addGroup(const std::string& sourceopt,
    const std::string       & group);

  /** Set hidden on an option */
  void
  setHidden(const std::string& sourceopt);

  /** Do we want advanced help for this option? */
  bool
  wantAdvancedHelp(const std::string& sourceopt);

  /** Add advanced help to an option.  */
  void
  addAdvancedHelp(const std::string& sourceopt,
    const std::string              & help);

  /** Store a parsed argument value, from XML or command line */
  void
  storeParsedArg(const std::string& name,
    const std::string             & value,
    const bool                    enforceStrict = true,
    const bool                    fromiconfig   = false);

  /** Replace all macros within a string */
  std::string
  replaceMacros(const std::string& original);

  /** Force add given option to map */
  void
  addOption(const Option& o);

  /** Make an option */
  Option *
  makeOption(bool    required,
    bool             boolean,
    bool             system,
    const std::string& opt,
    const std::string& name,
    const std::string& usage,
    const std::string& extra);

  /** Return option with this key */
  Option *
  getOption(const std::string& key);

  /** Return true if this option name is parsed */
  bool
  isParsed(const std::string& key);

  /** Set if processed, this allows get value methods to work */
  void
  setIsProcessed(){ isProcessed = true; }

  /** Get if processed, this allows get value methods to work */
  bool
  getIsProcessed(){ return isProcessed; }

protected:

  /** Advanced Help filter list of options */
  std::vector<Option> myHelpOptions;

private:

  /** Have arguments been sucessfully processed? */
  bool isProcessed;
};
}
