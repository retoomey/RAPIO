#pragma once

#include <rOptionList.h>
#include <rOption.h>
#include <rLLH.h>
#include <rTime.h>

#include <string>
#include <vector>
#include <map>

namespace rapio {
class LLCoverageArea;

/* RAPIOOptions handles parsing the information from a command line
 * for parameters in/out of the stock algorithm, and printing them
 * to command line.
 * @author Robert Toomey
 */
class RAPIOOptions : public OptionList {
public:

  /** Construct default options */
  RAPIOOptions(const std::string& base = "Program");
  ~RAPIOOptions(){ }

  /** Add advanced help for default options */
  void
  addPostLoadedHelp();

  // We support iconfig and oconfig options for loading/saving parameters
  // FIXME: Should this in the OptionList generically?

  /** Read an xml configuration file into the options */
  void
  readConfigFile(const std::string& filename);

  /** Write an xml configuration file with options from all sources */
  void
  writeConfigFile(const std::string& filename);

  /** Verify all required arguments were passed in */
  bool
  verifyRequired();

  /** Verify any suboptions correctly match parameters */
  bool
  verifySuboptions();

  /** Verify all options given are recognized */
  bool
  verifyAllRecognized();

  /** Count arguments that match a given filter */
  size_t
  countArgs(const std::vector<Option>& options,
    OptionFilter                     & a);

  /** Output arguments that match a given filter */
  void
  dumpArgs(std::vector<Option>& options,
    OptionFilter              & a,
    bool                      postParse    = false,
    bool                      advancedHelp = false);

  /** Do the standard dump of help when nothing passed in */
  void
  dumpArgs();

  // FIXME: Grid stuff experimental.  Need to work on this/clean up
  // These three functions might have better locations

  /** Legacy grid using the t, b, and s options.  This is for backward
   * compatibility with older WDSS2 algorithms */
  void
  declareLegacyGrid();

  /** Read the legacy grid t, b, and s, options from options.  Note,
   * declareLegacyGrid should be called in declareOptions */
  bool
  getLegacyGrid(
    LLCoverageArea& grid
  );

  /** Create a grid argument.  Grids are special 2d or 3d coordinates for
   * clipping or determining bounds. */
  Option *
  grid2D(const std::string& opt,
    const std::string     & name,
    const std::string     & usage);

  /** Convenience get a 2D or 3D location from a string part */
  LLH
  getLocation(const std::string& data,
    const std::string          & gridname,
    const std::string          & part,
    const bool                 is3D);

  /** Convenience get a 2D or 3D grid information from a grid option */
  void
  getGrid(const std::string& name,
    LLH                    & location,
    float                  & lat_spacing,
    float                  & lon_spacing,
    int                    & lat_dim,
    int                    & lon_dim);

  // ^^^ End experimental grid stuff. I'll revisit it

  /** Set the module name or program name */
  void
  setName(const std::string& name);

  /** If set, shows the description field for algorithm */
  void
  setDescription(const std::string& description);

  /** If set, shows the authors field for algorithm */
  void
  setAuthors(const std::string& authors);

  /** If set, adds another example */
  void
  setExample(const std::string& e);

  /** If set, shows copyright or algorithm information header,
   *  default is RAPIO header. */
  void
  setHeader(const std::string& header);

  /** Sets a macro replacement for 'just text', using a '%s',
   * for example for an alg that just takes a string */
  void
  setTextOnlyMacro(const std::string& macro);

  /** Expand main arguments to a fully paired arg/value list */
  std::vector<std::string>
  expandArgs(const std::vector<std::string>& args, std::string& leftovers);

  /** Replace macro on leftovers of command line and rerun expansion
   * macro needs to have %s replaced  'file=%s -o=/tmp'
   */
  void
  applyLeftOverMacro(const std::string& macro, std::string& leftovers, std::vector<std::string>& expanded);

  /** Draw a dash line */
  void
  dumpHeaderLine();

  /** Set help fields for advanced help, used to dynamically
   * add only required advanced help **/
  void
  setHelpFields(const std::vector<std::string>& list);

  /** Dump help for options based on what is passed in */
  void
  dumpHelp();

  /** Process command line arguments, no printing. */
  bool
  processArgs(const int & argc,
    char **             & argv);

  /** Set anything from the global configuration settings */
  void
  initToSettings();

  /** Finalize command line arguments and print out of everything. */
  bool
  finalizeArgs(bool haveHelp);

  /** Was the macro applied on leftovers? */
  bool
  isMacroApplied(){ return myMacroApplied; }

protected:

  /** Name of module or command line binary */
  std::string myName;

  /** Authors of this module/algorithm */
  std::string myAuthors;

  /** Extra example of this module/algorithm */
  std::string myExample;

  /** Header of this module/algorithm, used for copyright, etc. */
  std::string myHeader;

  /** Macro for left over text, if anything */
  std::string myMacro;

  /** Simple description of algorithm */
  std::string myDescription;

  /** Actual command line arguments */
  std::vector<std::string> myRawArgs;

  /** Leftover non-args */
  std::string myLeftovers;

  /** Was macro applied? */
  bool myMacroApplied;
};
}
