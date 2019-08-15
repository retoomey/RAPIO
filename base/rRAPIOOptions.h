#pragma once

#include <rAlgorithm.h>
#include <rOption.h>
#include <rLLH.h>
#include <rTime.h>

#include <string>
#include <vector>
#include <map>

namespace rapio {
/* RAPIOOptions handles the information for parameters in/out of the stock
 * algorithm.
 * @author Robert Toomey
 */
class RAPIOOptions : public Algorithm {
public:

  RAPIOOptions();
  ~RAPIOOptions(){ }

  std::map<std::string, Option> optionMap;
  std::map<std::string, Option> unusedOptionMap;

  static unsigned int max_arg_width;
  static unsigned int max_name_width;

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

  /** Output arguments that match a given filter */
  void
  dumpArgs(std::vector<Option>& options,
    OptionFilter              & a,
    bool                      postParse = false,
    bool                      advancedHelp = false);

  /** Do the standard dump of help when nothing passed in */
  void
  dumpArgs();

  /** Create a boolean argument */
  Option *
  boolean(const std::string& opt,
    const std::string      & usage);

  /** Create a grid argument.  Grids are special 2d or 3d coordinates for
   * clipping or determining bounds. */
  Option *
  grid2D(const std::string& opt,
    const std::string     & name,
    const std::string     & usage);

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
    Time                   & time,
    float                  & lat_spacing,
    float                  & lon_spacing,
    int                    & lat_dim,
    int                    & lon_dim);

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

  /** Process one argument */
  unsigned int
  processArg(std::vector<std::string>& args,
    unsigned int                     i,
    std::string                      & arg,
    std::string                      & value);

  /** Return sorted options */
  void
  sortOptions(std::vector<Option>&,
    OptionFilter& a);

  /** Process command line arguments */
  bool
  processArgs(int & argc,
    char **       & argv);

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

  /** Add advanced help to an option.  */
  void
  addAdvancedHelp(const std::string& sourceopt,
    const std::string              & help);

private:

  /** Store a parsed argument value, from XML or command line */
  void
  storeParsedArg(const std::string& name,
    const std::string             & value);

  /** Replace all macros within a string */
  std::string
  replaceMacros(const std::string& original);

  /** Make an option */
  Option *
  makeOption(bool    required,
    bool             boolean,
    bool             system,
    const std::string& opt,
    const std::string& name,
    const std::string& usage,
    const std::string& extra);

  Option *
  getOption(const std::string& opt);

protected:

  /** Name of module or command line binary */
  std::string myName;

  /** Authors of this module/algorithm */
  std::string myAuthors;

  /** Header of this module/algorithm, used for copyright, etc. */
  std::string myHeader;

  /** Simple description of algorithm */
  std::string myDescription;

  /** The width of all our command line output */
  size_t myOutputWidth;

  /** Have arguments been sucessfully processed? */
  bool isProcessed;
};
}
