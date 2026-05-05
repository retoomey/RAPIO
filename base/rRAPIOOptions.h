#pragma once

#include <rIOptionBackend.h>
#include <rLLCoverageArea.h>

#include <string>
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
  boolean(const std::string& opt,
    const std::string      & usage);

  /** Create a required argument */
  void
  require(const std::string& opt,
    const std::string      & exampleArg,
    const std::string      & usage);

  /** Create an optional argument.  Optional have default values */
  void
  optional(const std::string& opt,
    const std::string       & defaultValue,
    const std::string       & usage);

  /** Set a default value for an optional argument */
  void
  setDefaultValue(const std::string& opt, const std::string& defaultValue)
  {
    myBackend->setDefaultValue(opt, defaultValue);
  }

  /** Add an option to a group in help */
  void
  addGroup(const std::string& sourceopt, const std::string& group)
  {
    myBackend->addGroup(sourceopt, group);
  }

  /** Add a choice to the suboption list of an option */
  void
  addSuboption(const std::string& sourceopt,
    const std::string& opt, const std::string& description)
  {
    myBackend->addSuboption(sourceopt, opt, description);
  }

  /** Make this option hidden in help unless explicitly asked for */
  void
  setHidden(const std::string& sourceopt)
  {
    myBackend->setHidden(sourceopt);
  }

  /** Add advanced longer help for an option */
  void
  addAdvancedHelp(const std::string& sourceopt, const std::string& help)
  {
    myBackend->addAdvancedHelp(sourceopt, help);
  }

  /** Turn on if we enforce suboption choices only */
  void
  setEnforcedSuboptions(const std::string& key, bool flag)
  {
    myBackend->setEnforcedSuboptions(key, flag);
  }

  // RETRIEVAL (Forwarded to Backend)

  /** Get a named option back as a string */
  std::string
  getString(const std::string& opt)
  {
    return myBackend->getString(opt);
  }

  /** Get a named option back as a boolean */
  bool
  getBoolean(const std::string& opt)
  {
    return myBackend->getBoolean(opt);
  }

  /** Get a named option back as a float */
  float
  getFloat(const std::string& opt)
  {
    return myBackend->getFloat(opt);
  }

  /** Get a named option back as an int */
  int
  getInteger(const std::string& opt)
  {
    return myBackend->getInteger(opt);
  }

  /** Is an option in the list? */
  bool
  isInSuboptions(const std::string& key)
  {
    return myBackend->isInSuboptions(key);
  }

  /** This this option parsed? */
  bool
  isParsed(const std::string& key)
  {
    return myBackend->isParsed(key);
  }

  /** Do we want advanced help? */
  bool
  wantAdvancedHelp(const std::string& sourceopt)
  {
    return myBackend->wantAdvancedHelp(sourceopt);
  }

  /** Store an argument directly */
  void
  storeParsedArg(const std::string& name, const std::string& value, const bool enforceStrict = true,
    const bool fromiconfig = false)
  {
    myBackend->storeParsedArg(name, value, enforceStrict, fromiconfig);
  }

  /** Set that we have processed all arguments */
  void setIsProcessed(){ myBackend->setIsProcessed(); }

  /** Have we processed all arguments */
  bool getIsProcessed(){ return myBackend->getIsProcessed(); }

  // PROCESSING (Forwarded to Backend)

  /** Process arguments from command line */
  bool
  processArgs(int argc, char ** argv);

  void setHelpFields(const std::vector<std::string>& list){ myBackend->setHelpFields(list); }

  // RAPIO/OPTIONLIST SPECIFICS (Forwarded to Backend)

  void addPostLoadedHelp(){ myBackend->addPostLoadedHelp(); }

  /** Read an xml configuration file into the options */
  void
  readConfigFile(const std::string& filename);

  /** Write an xml configuration file with options from all sources */
  void
  writeConfigFile(const std::string& filename);

  // Grid methods ---------------------

  /** Create a grid argument.  Grids are special 2d or 3d coordinates for
   * clipping or determining bounds. */
  void
  grid2D(const std::string& opt, const std::string& defaultValue, const std::string& usage);

  /** Legacy grid using the t, b, and s options.  This is for backward
   * compatibility with older WDSS2 algorithms */
  void
  declareLegacyGrid(const std::string& defaultGrid = "nw(37, -100) se(30.5, -93) h(0.5,20,NMQWD) s(0.01, 0.01)");

  /** Convenience get a 2D or 3D grid information from a grid option */
  bool
  getGrid(const std::string& name, LLCoverageArea& grid);

  /** Read the legacy grid t, b, and s, options from options.  Note,
   * declareLegacyGrid should be called in declareOptions */
  bool
  getLegacyGrid(LLCoverageArea& grid);

  /** Get the module name */
  std::string
  getName() const { return myName; }

  /** Set the module name or program name */
  void
  setName(const std::string& name);

  /** Get algorithm description */
  std::string
  getDescription() const { return myDescription; }

  /** If set, shows the description field for algorithm */
  void
  setDescription(const std::string& description);

  /** Get the author list */
  std::string
  getAuthors() const { return myAuthors; }

  /** If set, shows the authors field for algorithm */
  void
  setAuthors(const std::string& authors);

  /** Get the example */
  std::string
  getExample() const { return myExample; }

  /** If set, adds another example */
  void
  setExample(const std::string& e);

  /** Get the header */
  std::string
  getHeader() const { return myHeader; }

  /** If set, shows copyright or algorithm information header,
   *  default is RAPIO header. */
  void
  setHeader(const std::string& header);

  /** Draw a dash line */
  // void
  // dumpHeaderLine();

  /** Sets a macro replacement for 'just text', using a '%s',
   * for example for an alg that just takes a string */
  void setTextOnlyMacro(const std::string& macro){ myTextOnlyMacro = macro; }

  /** Was the macro applied on the positional args? */
  bool isMacroApplied(){ return myMacroApplied; }

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
  expandMacros(const std::string& original) const;

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
