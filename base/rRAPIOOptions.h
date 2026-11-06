#pragma once

#include <rIOptionBackend.h>
#include <rLLCoverageArea.h>

#include <string>
#include <string_view>
#include <memory>
#include <vector>

namespace rapio {
/* RAPIOOptions handles parsing the information from a command line
 * for parameters in/out of the stock algorithm, and printing them
 * to command line.
 * @author Robert Toomey
 */
class RAPIOOptions {
public:

  /** Construct default options */
  RAPIOOptions(const std::string& base = "Program");

  /** Base destructor */
  ~RAPIOOptions() = default;

  // DEFINITION & BINDING (Forwarded to Backend)

  /** Create a boolean argument */
  void
  boolean(std::string_view opt,
    std::string_view       usage);

  /** Create a required argument */
  void
  require(std::string_view opt,
    std::string_view       exampleArg,
    std::string_view       usage);

  /** Create an optional argument.  Optional have default values */
  void
  optional(std::string_view opt,
    std::string_view        defaultValue,
    std::string_view        usage);

  /** Set a default value for an optional argument */
  void
  setDefaultValue(std::string_view opt, std::string_view defaultValue)
  {
    myBackend->setDefaultValue(opt, defaultValue);
  }

  /** Add an option to a group in help */
  void
  addGroup(std::string_view sourceopt, std::string_view group)
  {
    myBackend->addGroup(sourceopt, group);
  }

  /** Add a choice to the suboption list of an option */
  void
  addSuboption(std::string_view sourceopt,
    std::string_view opt, std::string_view description)
  {
    myBackend->addSuboption(sourceopt, opt, description);
  }

  /** Make this option hidden in help unless explicitly asked for */
  void
  setHidden(std::string_view sourceopt)
  {
    myBackend->setHidden(sourceopt);
  }

  /** Add advanced longer help for an option */
  void
  addAdvancedHelp(std::string_view sourceopt, std::string_view help)
  {
    myBackend->addAdvancedHelp(sourceopt, help);
  }

  /** Turn on if we enforce suboption choices only */
  void
  setEnforcedSuboptions(std::string_view key, bool flag)
  {
    myBackend->setEnforcedSuboptions(key, flag);
  }

  // RETRIEVAL (Forwarded to Backend)

  /** Get a named option back as a string */
  std::string
  getString(std::string_view opt) const
  {
    return myBackend->getString(opt);
  }

  /** Get a named option back as a boolean */
  bool
  getBoolean(std::string_view opt) const
  {
    return myBackend->getBoolean(opt);
  }

  /** Get a named option back as a float */
  float
  getFloat(std::string_view opt) const
  {
    return myBackend->getFloat(opt);
  }

  /** Get a named option back as a double */
  double
  getDouble(std::string_view opt) const
  {
    return myBackend->getDouble(opt);
  }

  /** Get a named option back as an int */
  int
  getInteger(std::string_view opt) const
  {
    return myBackend->getInteger(opt);
  }

  /** Is an option in the list? */
  bool
  isInSuboptions(std::string_view key) const
  {
    return myBackend->isInSuboptions(key);
  }

  /** This this option parsed? */
  bool
  isParsed(std::string_view key) const
  {
    return myBackend->isParsed(key);
  }

  /** Do we want advanced help? */
  bool
  wantAdvancedHelp(std::string_view sourceopt) const
  {
    return myBackend->wantAdvancedHelp(sourceopt);
  }

  /** Store an argument directly */
  void
  storeParsedArg(std::string_view name, std::string_view value, const bool enforceStrict = true,
    const bool fromiconfig = false)
  {
    myBackend->storeParsedArg(name, value, enforceStrict, fromiconfig);
  }

  /** Set that we have processed all arguments */
  void setIsProcessed(){ myBackend->setIsProcessed(); }

  /** Have we processed all arguments */
  bool
  getIsProcessed() const { return myBackend->getIsProcessed(); }

  // PROCESSING (Forwarded to Backend)

  /** Process arguments from command line */
  bool
  processArgs(int argc, char ** argv);

  void setHelpFields(const std::vector<std::string>& list){ myBackend->setHelpFields(list); }

  // RAPIO/OPTIONLIST SPECIFICS (Forwarded to Backend)

  void addPostLoadedHelp(){ myBackend->addPostLoadedHelp(); }

  /** Read an xml configuration file into the options */
  void
  readConfigFile(std::string_view filename);

  /** Write an xml configuration file with options from all sources */
  void
  writeConfigFile(std::string_view filename);

  // Grid methods ---------------------

  /** Create a grid argument.  Grids are special 2d or 3d coordinates for
   * clipping or determining bounds. */
  void
  grid2D(std::string_view opt, std::string_view defaultValue, std::string_view usage);

  /** Legacy grid using the t, b, and s options.  This is for backward
   * compatibility with older WDSS2 algorithms */
  void
  declareLegacyGrid(std::string_view defaultGrid = "nw(37, -100) se(30.5, -93) h(0.5,20,NMQWD) s(0.01, 0.01)");

  /** Convenience get a 2D or 3D grid information from a grid option */
  bool
  getGrid(std::string_view name, LLCoverageArea& grid);

  /** Read the legacy grid t, b, and s, options from options.  Note,
   * declareLegacyGrid should be called in declareOptions */
  bool
  getLegacyGrid(LLCoverageArea& grid);

  /** Get the module name */
  const std::string&
  getName() const { return myName; }

  /** Set the module name or program name */
  void
  setName(std::string_view name);

  /** Get algorithm description */
  const std::string&
  getDescription() const { return myDescription; }

  /** If set, shows the description field for algorithm */
  void
  setDescription(std::string_view description);

  /** Get the author list */
  const std::string&
  getAuthors() const { return myAuthors; }

  /** If set, shows the authors field for algorithm */
  void
  setAuthors(std::string_view authors);

  /** Get the example */
  const std::string&
  getExample() const { return myExample; }

  /** If set, adds another example */
  void
  setExample(std::string_view e);

  /** Get the header */
  const std::string&
  getHeader() const { return myHeader; }

  /** If set, shows copyright or algorithm information header,
   *  default is RAPIO header. */
  void
  setHeader(std::string_view header);

  /** Draw a dash line */
  // void
  // dumpHeaderLine();

  /** Sets a macro replacement for 'just text', using a '%s',
   * for example for an alg that just takes a string */
  void setTextOnlyMacro(std::string_view macro){ myTextOnlyMacro = macro; }

  /** Was the macro applied on the positional args? */
  bool
  isMacroApplied() const { return myMacroApplied; }

  std::vector<std::string>
  getPositionalArgs() const { return myBackend->getPositionalArgs(); }

  void initToSettings(){ myBackend->initToSettings(); }

  // In the facade we do the 'global' work
  bool
  finalizeArgs(bool haveHelp);

private:

  /** Backend command line raw parser */
  std::unique_ptr<IOptionBackend> myBackend;

  std::string
  expandMacros(std::string_view original) const;

  std::string myTextOnlyMacro;

  bool myMacroApplied = false;

  /** Name of module or command line binary */
  std::string myName;

  /** Simple description of algorithm */
  std::string myDescription;

  /** Authors of this module/algorithm */
  std::string myAuthors;

  /** Extra example of this module/algorithm */
  std::string myExample;

  /** Header of this module/algorithm, used for copyright, etc. */
  std::string myHeader;
};
}
