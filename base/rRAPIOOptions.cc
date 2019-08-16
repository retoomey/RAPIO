#include "rRAPIOOptions.h"

#include "rError.h"
#include "rTime.h"
#include "rStrings.h"
#include "rColorTerm.h"
#include "rOS.h"
#include "rAlgConfigFile.h"
#include "rIOURL.h"

#include <sys/ioctl.h>
#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <iostream>

using namespace rapio;
using namespace std;

RAPIOOptions::RAPIOOptions()
{
  optional("verbose",
    "info",
    "Error log verbosity levels.  Increasing level prints more stuff:");
  addSuboption("verbose", "severe", "Print only the most severe errors");
  addSuboption("verbose", "info", "Print general information as well as errors");
  addSuboption("verbose", "debug", "Print everything, also turn on signal stacktraces");
  setEnforcedSuboptions("verbose", false);
  addGroup("verbose", "LOGGING");

  // grid2D("GridTest", "nw(34.5, 91.5) se(20.2, 109.5)", "Testing grid 2d");

  optional("logSize", "10000", "Maximum error log size in bytes.");
  addGroup("logSize", "LOGGING");

  optional("logFlush",
    "900",
    "Error log flush (force write) timer set in seconds.");
  addGroup("logFlush", "LOGGING");

  optional("iconfig",
    "",
    "Input URL to read a configuration file. Command line overrides.");
  addGroup("iconfig", "CONFIG");
  addAdvancedHelp("iconfig",
    "Can end with .xml for WDSS2 file, .config for HMET file. This will parse the given file and add found parameters. Anything passed to command line will override the contents of the passed in file.");

  optional("oconfig",
    "",
    "Output URL to write a configuration file, using parameters from all sources.");
  addGroup("oconfig", "CONFIG");
  addAdvancedHelp("oconfig",
    "Can end with .xml for WDSS2 file, .config for HMET file. This will all parameters if any from iconfig, override with command line arguments, then generate a new output file.  You can convert from one style to another as well.");

  // The help group
  boolean("help",
    "Print out parameter help information. Can also just type the program without arguments.");
  addGroup("help", "HELP");
  boolean("nf", "No ASCII formatting/colors in help output.");
  addGroup("nf", "HELP");
}

void
RAPIOOptions::setName(const std::string& n)
{
  myName = n;
}

void
RAPIOOptions::setDescription(const std::string& d)
{
  myDescription = d;
}

void
RAPIOOptions::setAuthors(const std::string& a)
{
  myAuthors = a;
}

void
RAPIOOptions::setHeader(const std::string& a)
{
  myHeader = a;

  /** Default WDSS2 weather algorithms which are copyrighted by
   * the University of Oklahoma and designed implemented at the
   * National Severe Storms Laboratory. */
  if (myHeader == "WDSS2") {
    Time now              = Time::CurrentTime();
    std::string year      = std::to_string(now.getYear());
    std::string copyright =
      ColorTerm::bold("WDSS2/MRMS Algorithm using RAPIO ")
      + "(c) 2001-" + year + " University of Oklahoma. ";
    copyright += "All rights reserved. See "
      + ColorTerm::underline("http://www.wdssii.org/");
    myHeader = copyright;
  }
}

Option *
RAPIOOptions::grid2D(const std::string& opt, const std::string& defaultValue,
  const std::string& usage)
{
  // For the moment, just make an optional grid...FIXME: check the default value
  // for valid
  Option * o = optional(opt, defaultValue, usage);

  addGroup(opt, "SPACE");
  return (o);
}

LLH
RAPIOOptions::getLocation(
  const std::string& data,
  const std::string& gridname,
  const std::string& part,
  const bool       is3D)
{
  std::vector<std::string> pieces;
  Strings::split(data, ',', &pieces);
  size_t expected = is3D ? 3 : 2;

  if (pieces.size() != expected) {
    std::cout << "Error in specifying the " + part + " of grid '" + gridname
      + "', expected " << expected << " pieces.\n";
    exit(1);
  }
  const float heightKMs = is3D ? atof(pieces[2].c_str()) : 10.0f;
  LLH loc(atof(pieces[0].c_str()),
    atof(pieces[1].c_str()),
    heightKMs);
  return (loc);
}

namespace {
std::string
getMapString(
  std::map<std::string, std::string>& lookup, const std::string& name)
{
  std::map<std::string, std::string>::iterator iter;
  std::string s = "";
  iter = lookup.find(name);

  if (iter != lookup.end()) {
    s = iter->second;
  }
  return (s);
}
}

void
RAPIOOptions::getGrid(const std::string& name,
  LLH                                  & location,
  float                                & lat_spacing,
  float                                & lon_spacing,
  int                                  & lat_dim,
  int                                  & lon_dim)
{
  std::string grid = getString(name);

  // -----------------------------------------------------------
  // Filter a grid string to our grid 'language'
  // FIXME: I'll want the value given by user to override
  // values from the default...

  // Allow spaces in input only after a ')'. Thus something like
  // "nw ( 5, 3)    se(40,4,  40)" becomes "nw(5,3) se(40,4,40)"
  std::string cgrid = "";
  bool space        = false;

  for (size_t i = 0; i < grid.size(); i++) {
    char c = tolower(grid[i]);

    switch (c) {
        case ' ': continue;

        case ')':
          space = true; // Force single space after ')'

        // break; fall though add character
        default:
          cgrid += c;

          if (space) {
            if (i != grid.size() - 1) {
              cgrid += ' ';
              space  = false;
            }
          }
          break;
    }
  }
  grid = cgrid;

  // Now break up the f(v) pairs into f --> v
  std::map<std::string, std::string> lookup;
  std::vector<std::string> fields;
  Strings::split(grid, ' ', &fields);

  for (size_t i = 0; i < fields.size(); i++) {
    // std::cout << "Grid setting  " << i << " is '"<<fields[i]<<"'\n";

    // Looking for name(value)....
    std::string name, value;
    bool invalue = false;

    for (size_t j = 0; j < fields[i].size(); j++) {
      char c = fields[i][j];

      switch (c) {
          case '(': { invalue = true;
                      continue;
          }

          case ')': { break;
          }

          default:
            invalue ? value += c : name += c;
            break;
      }
    }

    // ALIAS
    if (name == "t") { name = "nw"; }
    if (name == "b") { name = "se"; } lookup[name] = value;
  }

  std::string top     = getMapString(lookup, "nw");
  std::string bot     = getMapString(lookup, "se");
  std::string spacing = getMapString(lookup, "s");

  std::cout << "GRID IS " << top << "**" << bot << "**" << spacing << "\n";

  // Now convert them...
  LLH nwc = getLocation(top, "grid", "top corner", false);
  LLH sec = getLocation(bot, "grid", "bottom corner", false);

  // Ahhh wait this is for a 2D grid right?
  //   sec.setHeight( nwc.getHeight() ); // set them to same height ...

  std::vector<std::string> pieces2;
  Strings::split(spacing, ',', &pieces2);

  if (pieces2.size() < 2) {
    std::cout << "Error in specifying grid '" << name << "' spacing.'\n";
    exit(1);
  }
  const float latf = atof(pieces2[0].c_str());
  const float lonf = atof(pieces2[1].c_str());

  // This would be for 3D grids...
  // Length htsp = Length::Kilometers( atof(pieces[2].c_str()) );

  // Standard grid stuff.  This can be used to create a LatLonGrid
  if ((latf > 0.0) && (lonf > 0.0)) {
    double latsp = latf;
    double lonsp = lonf;
    lat_dim = int(0.5 + (nwc.getLatitudeDeg() - sec.getLatitudeDeg()) / latsp);
    lon_dim = int(0.5 + (sec.getLongitudeDeg() - nwc.getLongitudeDeg()) / lonsp);

    if ((lat_dim < 1) || (lon_dim < 1)) {
      std::cout << "Error in specifying grid '" << name << "' dimensions ("
                << lat_dim << " * " << lon_dim << ")\n";
      exit(1);
    }

    lat_spacing = latsp;
    lon_spacing = lonsp;
    location    = nwc; // copy
  } else {
    std::cout << "Error in specifying grid '" << name << "' spacing ("
              << latf << "," << lonf << ") must both be greater than zero.\n";
    exit(1);
  }
} // RAPIOOptions::getGrid

/** Print a list of arguments with a given filter */
void
RAPIOOptions::dumpArgs(std::vector<Option>& options,
  OptionFilter                            & a,
  bool                                    postParse,
  bool                                    advancedHelp)
{
  auto& s = std::cout;

  for (auto& o: options) {
    // Output the option
    if (a.show(o)) {
      size_t c1 = max_arg_width + 2 + 2;

      // size_t c2 = max_name_width+1;

      // Calculate the option output
      std::string opt;

      if (o.required) {
        opt = " " + o.opt;
      } else {
        opt = " [" + o.opt + "]";
      }

      // Always output the first two columns, regardless of terminal width...
      s << ColorTerm::fRed << ColorTerm::fBold;
      s << setw(c1) << left << opt << ColorTerm::fNormal;

      // Create the status string
      std::string status;

      if (postParse) {
        status = "(valueof=" + o.parsedValue + ")";
      } else {
        if (o.required) {
          // status = "(required)";
          status = "(example=" + o.example + ")";
        } else {
          status = "(default=" + o.defaultValue + ")";
        }
      }
      s << ColorTerm::fBlue;
      ColorTerm::wrapWithIndent(c1, c1, status);
      s << ColorTerm::fNormal;

      // Usage, using our width wrapping output
      if (o.usage != "") {
        // s << fGreen;
        s << setw(c1 + 2) << left << ""; // Indent 1 column
        ColorTerm::wrapWithIndent(c1 + 2, c1 + 4, o.usage);

        // s << fNormal;
      }

      // Choices for suboptions....
      if (o.suboptions.size() > 0) {
        for (auto& i: o.suboptions) {
          s << setw(c1 + 4) << left << ""; // Indent 1 column
          std::string optpad = i.opt;
          size_t max         = o.suboptionmax;

          while (optpad.length() < max) {
            optpad += " ";
          }
          std::string out = ColorTerm::fBlue + ColorTerm::bold(optpad)
            + " --" + i.description;
          ColorTerm::wrapWithIndent(c1 + 4, c1 + 4 + max + 3, out);
        }
      }

      if (advancedHelp && (o.advancedHelp != "")) {
        // s << fGreen;
        // s << setw(c1+2) << left << ""; // Indent 1 column
        int d = 5;
        s << ColorTerm::bold("DETAILED HELP:\n");
        s << setw(d) << left << ""; // Indent 1 column
        ColorTerm::wrapWithIndent(d, 0, o.advancedHelp);
        s << "\n";

        // s << fNormal;
      }
    }
  }
} // RAPIOOptions::dumpArgs

bool
RAPIOOptions::verifyRequired()
{
  bool good = true;

  // Copy missing
  FilterMissingRequired r;

  std::vector<Option> allOptions;
  sortOptions(allOptions, r);

  if (allOptions.size() > 0) {
    std::cout << "Missing " << ColorTerm::bold("(" + std::to_string(allOptions.size()) + ")")
              << " REQUIRED arguments: \n";
    OptionFilter all;
    dumpArgs(allOptions, all); // already filtered..but should work
    good = false;
  }
  return (good);
}

bool
RAPIOOptions::verifySuboptions()
{
  bool good = true;

  // Copy arguments with bad suboptions...
  FilterBadSuboption r;

  std::vector<Option> allOptions;
  sortOptions(allOptions, r);

  if (allOptions.size() > 0) {
    std::cout << "BAD option choice for " << ColorTerm::bold("(" + std::to_string(allOptions.size()) + ")")
              << " arguments:\n";
    OptionFilter all;
    dumpArgs(allOptions, all, true); // already filtered..but should work
    good = false;
  }

  return (good);
}

/** The default dump of all arguments available */
void
RAPIOOptions::dumpArgs()
{
  OptionFilter all;

  std::vector<Option> allOptions;
  sortOptions(allOptions, all);

  // Dump description, if given..
  size_t d = 5;

  if (!myDescription.empty()) {
    std::cout << ColorTerm::bold("DESCRIPTION:\n");
    std::cout << setw(d) << left << ""; // Indent 1 column
    ColorTerm::wrapWithIndent(d, 0, myDescription);
  }

  // Dump authors, if given..
  if (!myAuthors.empty()) {
    std::cout << ColorTerm::bold("AUTHOR(S):\n");
    std::cout << setw(d) << left << ""; // Indent 1 column
    ColorTerm::wrapWithIndent(d, 0, myAuthors);
  }

  // Dump a generated 'Example' statement...this uses the 'examples' from the
  // required options
  // We'll have to column format this stuff eventually...
  std::cout << ColorTerm::bold("EXAMPLE:\n");
  std::cout << setw(d) << left << ""; // Indent 1 column
  std::cout << myName << " ";
  std::string outputRequired;
  size_t commandLength = d + myName.length() + 1;
  FilterRequired r;

  for (unsigned int z = 0; z < allOptions.size(); z++) {
    // Output the option
    if (r.show(allOptions[z])) {
      outputRequired +=
        ("-" + ColorTerm::bold(allOptions[z].opt) + " " + ColorTerm::underline(allOptions[z].example) + " ");
    }
  }
  outputRequired += ColorTerm::bold("--verbose");
  ColorTerm::wrapWithIndent(commandLength, commandLength, outputRequired);

  FilterGroup lio("I/O");
  std::cout << ColorTerm::bold("I/O:\n");
  dumpArgs(allOptions, lio);

  std::cout << ColorTerm::bold("OPTIONS:\n");
  FilterGroup la("");
  dumpArgs(allOptions, la);

  // Get all groups not matching our stuff
  std::vector<std::string> otherGroups;

  for (size_t i = 0; i < allOptions.size(); i++) {
    const Option& o = allOptions[i];

    // For each group in option...
    for (size_t j = 0; j < o.groups.size(); j++) {
      // FIXME: Maybe a map here is better
      std::string group = o.groups[j];

      // Add to total group list if not found...
      bool found = false;

      if (group == "I/O") { found = true; }

      if (group == "CONFIG") { found = true; }

      if (group == "LOGGING") { found = true; }

      if (group == "HELP") { found = true; }

      for (size_t k = 0; k < otherGroups.size(); k++) {
        const std::string g = otherGroups[k];

        if (g == group) { found = true; break; }
      }

      if (!found) {
        otherGroups.push_back(group);
      }
    }
  }

  // Print out user registered groups...
  std::sort(otherGroups.begin(), otherGroups.end());

  for (size_t i = 0; i < otherGroups.size(); i++) {
    std::cout << ColorTerm::bold(otherGroups[i] + " OPTIONS:\n");
    FilterGroup og(otherGroups[i]);
    dumpArgs(allOptions, og);
  }

  FilterGroup ll("LOGGING");
  std::cout << ColorTerm::bold("LOGGING:\n");
  dumpArgs(allOptions, ll);

  FilterGroup lc("CONFIG");
  std::cout << ColorTerm::bold("CONFIG:\n");
  dumpArgs(allOptions, lc);

  FilterGroup lh("HELP");
  std::cout << ColorTerm::bold("HELP:\n");
  dumpArgs(allOptions, lh);
} // RAPIOOptions::dumpArgs

/** Process and remove a single argument from a vector of strings.  Either 1 or
 * 2 spots are removed,
 * depending on -arg=stuff or -arg stuff being processed.
 */
unsigned int
RAPIOOptions::processArg(std::vector<std::string>& args, unsigned int j,
  std::string& arg, std::string& value)
{
  unsigned int count = 0;

  std::string c = "";
  std::string v = "";

  if (j < args.size()) {
    c = args[j];

    bool haveOpt = false;

    if (c.substr(0, 2) == "--") { // Double slash options are of the form
                                  // --option=something
                                  // or the form --option (with implied =
                                  // "")
      c       = c.substr(2);
      haveOpt = true;
    } else if (c.substr(0, 1) == "-") { // Single slash options are of the form
                                        // -option something
                                        // or can also be -option=something
      c       = c.substr(1);
      haveOpt = true;
    } else {
      // What to do with non-slashed argument?
    }

    if (haveOpt) {
      count = 1;

      // Try to split it by the "="  We'll assume single arg in all cases
      std::vector<std::string> twoargs;
      Strings::splitOnFirst(c, '=', &twoargs);

      if (twoargs.size() > 1) {
        // std::cout << "DEBUG:: Arg broke into " << twoargs[0] << " and " <<
        // twoargs[1] << "\n";
        c = twoargs[0];
        v = twoargs[1];
      } else {
        // If we don't split and next is not a new option, we use that as value
        if (j + 1 < args.size()) {
          std::string c2 = args[j + 1];

          if (c2.substr(0, 1) != "-") {
            v     = c2;
            count = 2;
          }
        }
      }
    }
    arg   = c;
    value = v;
  }

  // Erase the argument so we can dump the unprocessed stuff later
  if (count == 0) { // String where we don't expect it...move j forward..
    j++;
  }

  if (count == 1) { // Single arg like "K=stuff"  Delete item, leave J alone
    args.erase(args.begin() + j);
    j = j;
  }

  if (count == 2) { // Double arg like "-K stuff" Delete two items, leave J
                    // alone
    args.erase(args.begin() + j);
    args.erase(args.begin() + j);
    j = j;
  }
  return (j);
} // RAPIOOptions::processArg

namespace {
/** Is this allowed in config file, if not ignore.  Some options
 * will mess with our processing */
bool
allowInConfig(const std::string& o)
{
  if (o == "iconfig") { return false; }
  if (o == "oconfig") { return false; }
  if (o == "help") { return false; } // Don't let config file dump help lol
  if (o == "nf") { return false; }   // nf right now only part of help
  return true;
}
}

/** Read a configuration file into our arguments.  Any arguments on the
 * command
 * line override the settings in the config file */
void
RAPIOOptions::readConfigFile(const std::string& string)
{
  // Get config file reader for given option string
  URL aURL(string);
  auto config = AlgConfigFile::getConfigFileForURL(aURL);

  if (config == nullptr) { exit(1); } // Bad parameter, no reader for it

  // Get the option/value pairs from the configuration file...
  std::vector<std::string> options;
  std::vector<std::string> values;
  if (config->readConfigURL(aURL, options, values)) {
    if (options.size() != values.size()) {
      LogSevere("Bad configuration file reader, options not equal to values\n");
      exit(1);
    }
    // ... and store them in our options.
    for (size_t i = 0; i < options.size(); ++i) {
      if (allowInConfig(options[i])) {
        // LogDebug("STORING " << options[i] << " == " << values[i] << "\n");
        storeParsedArg(options[i], values[i]);
      } else {
        // LogDebug("**IGNORING*** " << options[i] << " == " << values[i] << "\n");
      }
    }
  } else {
    // Reader should complain already
    // LogSevere("Couldn't read configuration file at " << aURL << "\n");
    exit(1);
  }
}

/** Write a configuration that contains all the final processed command line
 * arguments, including any imported command line settings */
void
RAPIOOptions::writeConfigFile(const std::string& string)
{
  // Get config file reader for given option string
  URL aURL(string);
  auto config = AlgConfigFile::getConfigFileForURL(aURL);

  if (config == nullptr) { exit(1); } // Bad parameter, no reader for it

  // Create a list of option/value pairs for configuration writer
  std::vector<std::string> options;
  std::vector<std::string> values;
  for (auto& i: optionMap) {
    if (allowInConfig(i.second.opt)) {
      options.push_back(i.second.opt);
      if (i.second.parsed) {
        values.push_back(i.second.parsedValue);
      } else {
        values.push_back(i.second.defaultValue);
      }
    }
  }

  // Send to the writer
  if (config->writeConfigURL(aURL, myName, options, values)) {
    // We're exiting...
    std::cout << "Successfully generated configuration file " << aURL << "\n";
  }
  ;
}

/** Process a command line argument list */
bool
RAPIOOptions::processArgs(int& argc, char **& argv)
{
  // Get the console width in linux
  myOutputWidth = ColorTerm::getOutputWidth();

  // Just make args a string vector, less C more C++, not as efficient
  // but a whole lot easier to work with.
  // Create a vector string copy of all args except program name
  std::vector<std::string> args;

  for (int k = 1; k < argc; k++) {
    std::string stringArg((argv[k]));
    args.push_back(stringArg);
  }

  // Grab the name from the main program argument
  std::vector<std::string> pathbreaks;
  Strings::split(std::string(argv[0]), '/', &pathbreaks);

  if (pathbreaks.size() > 0) {
    myName = pathbreaks[pathbreaks.size() - 1];
  } else {
    myName = argv[0];
  }

  // Find each option one after the other
  unsigned int j = 0;
  std::string arg;
  std::string value;

  while (j < args.size()) {
    arg   = "";
    value = "";
    j     = processArg(args, j, arg, value);

    if (arg.empty()) { // Empty could be overriding a default, right?
      // Just text we ignore by default. We 'could' add the ability to handle
      // text without option markers
    } else {
      storeParsedArg(arg, value);
    }
  }

  // We can use the pull methods now...note we have yet to verify
  // we have all required/etc..
  setIsProcessed();

  // Help first incase of help iconfig
  bool haveHelp = false;
  if (isParsed("help")) {
    haveHelp = true;
  }

  // Read in given configuration file, if any
  std::string fileName = getString("iconfig");
  if ((!haveHelp) && (!fileName.empty())) {
    readConfigFile(fileName);
  }

  // Turn off color formatting if we have a nf format tag
  bool useFormatting = ColorTerm::haveColorSupport();
  bool haveNF        = false;
  if (isParsed("nf")) {
    haveNF        = true;
    useFormatting = false;
  }

  // Set if we want color output or not...
  ColorTerm::setColors(useFormatting);

  // Start outputting now...
  std::string header = "";

  for (size_t i = 0; i < myOutputWidth; i++) {
    header += "-";
  }
  std::cout << header << "\n";
  ColorTerm::wrapWithIndent(0, 0, myHeader);

  // std::string built = ColorTerm::bold("Binary Built:"+std::string(__DATE__) + "
  // " + __TIME__;
  // wrapWithIndent(0, 0, built);

  // Just dump options if no args or if just "-nf" passed. "nf" is special, it
  // just removes
  // formatting
  if (haveHelp || (argc < 2) || ((argc < 3) && haveNF)) {
    // Here we handle help something, where we filter groups and then variables
    // to
    // show just the help for those.  It's useful for algorithms with lots of
    // options
    //
    if (args.size() > 1) { // help is one of them...
      // Ok if 'help' is the first argument, make
      // it dynamic based on what else is left on line
      for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == "help") { continue; }
        const std::string what = args[i];

        // Try to find a group matching argument...
        // and filter help for it
        OptionFilter all;
        std::string group;
        std::transform(what.begin(), what.end(), std::back_inserter(
            group), ::toupper);

        if (group == "OPTIONS") { group = ""; } FilterGroup g(group);
        std::vector<Option> allOptions;
        sortOptions(allOptions, g);

        if (allOptions.size() > 0) {
          std::cout << ColorTerm::bold(group + ":\n");
          dumpArgs(allOptions, all, false, true);
          std::exit(1);
        } else {
          // Try to find a variable matching argument...
          FilterName aName(what);
          std::vector<Option> names;
          sortOptions(names, aName);

          if (names.size() > 0) {
            std::cout << ColorTerm::bold(group + ":\n");
            dumpArgs(names, all, false, true);
            std::exit(1);
          } else {
            dumpArgs();
            std::exit(1);
          }
        }

        break;
      }
    } else {
      dumpArgs();
    }

    std::exit(1);
  }

  // Couple of error cases can occur in the arguments
  // One: actually non-processed stuff in the arguments (bad format)
  // Two: processed but not defined (good format, but not used)
  // We have stuff left not processed...
  if (args.size() > 0) {
    std::cout
      << "This part of your command line was unrecognized and ignored.  Maybe a typo?\n>>";

    for (unsigned int k = 0; k < args.size(); k++) {
      std::cout << args[k] << " ";
    }
    std::cout << "\n";
  }

  // Ok, so on unrecognized options we need to exit
  if (unusedOptionMap.size() > 0) {
    std::map<std::string, Option>::iterator i;
    std::vector<Option> unused;

    for (i = unusedOptionMap.begin(); i != unusedOptionMap.end(); ++i) {
      // Might have to modify some other parts...
      unused.push_back(i->second);
    }
    std::cout
      << "Unrecognized options were passed in. Possibly arguments have changed format:\n";
    std::cout << ColorTerm::bold("UNRECOGNIZED:\n");
    OptionFilter ff;
    dumpArgs(unused, ff, true);
    exit(1);
  }

  // Check for missing required arguments and complain
  if (!verifyRequired()) { exit(1); }

  // Check for invalid suboption in arguments and complain
  if (!verifySuboptions()) { exit(1); }

  // Check for configuration output and write out now...
  std::string ofileName = getString("oconfig");
  if (!ofileName.empty()) { // help already processed now
    writeConfigFile(fileName);
    exit(1);
  }

  // About to run..set each of the runtime things...
  // Let these crash if missing, since they should always be there
  // Log settings
  int logFlush  = getInteger("logFlush");
  int logSize   = getInteger("logSize");
  std::string v = getString("verbose");

  if (v == "") {
    v = "info"; // Why doesn't default string do this?  FIXME?
    // FIXME: because --verbose is empty --> "" is allow for optional lists of values
    // Humm how to fix this properly?
  }
  Log::setLogSettings(v, logFlush, logSize);

  return (false);
} // RAPIOOptions::processArgs
