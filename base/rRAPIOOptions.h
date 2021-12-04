#pragma once

#include <rOptionList.h>
#include <rOption.h>
#include <rLLH.h>
#include <rTime.h>

#include <string>
#include <vector>
#include <map>

namespace rapio {
/* RAPIOOptions handles parsing the information from a command line
 * for parameters in/out of the stock algorithm, and printing them
 * to command line.
 * @author Robert Toomey
 */
class RAPIOOptions : public OptionList {
public:

  RAPIOOptions();
  ~RAPIOOptions(){ }

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

  /** If set, shows copyright or algorithm information header,
   *  default is RAPIO header. */
  void
  setHeader(const std::string& header);

  /** Draw a dash line */
  void
  dumpHeaderLine();

  /** Process one argument */
  unsigned int
  processArg(std::vector<std::string>& args,
    unsigned int                     i,
    std::string                      & arg,
    std::string                      & value);

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

protected:

  /** Name of module or command line binary */
  std::string myName;

  /** Authors of this module/algorithm */
  std::string myAuthors;

  /** Header of this module/algorithm, used for copyright, etc. */
  std::string myHeader;

  /** Simple description of algorithm */
  std::string myDescription;

  /** Actual command line arguments */
  std::vector<std::string> myRawArgs;
};
}
