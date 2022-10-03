#include "rRAPIOOptions.h"

#include "rError.h"
#include "rTime.h"
#include "rStrings.h"
#include "rColorTerm.h"
#include "rOS.h"
#include "rAlgConfigFile.h"
#include "rLLCoverageArea.h"
#include "rIOURL.h"

#include <sys/ioctl.h>
#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <iostream>

using namespace rapio;
using namespace std;

namespace {
void
printHelpHelp()
{
  const std::string name = OS::getProcessName(true);

  std::cout << "Type:\n  '" << name << " help' to see help, or\n  '" << name <<
    " help arg1 argN' for detailed help on various arguments or groups.\n";
}

// Common functions for testing if string is argument or value and trimming

/* Attempt to allow -stuff --stuff, but we treat - and -- as just text
 * matched with strip function below, this determines how argument names
 * are treated.
 **/
bool
isArgument(const std::string& s)
{
  auto l     = s.length();
  bool isArg = false;

  if ((l > 1) && (s[0] == '-')) { // First char of two or more is '-'
    if (s[1] == '-') {            // --
      if (l > 2) { isArg = true; } // --x case
    } else {
      isArg = true; // -x case
    }
  }
  return isArg;
}

void
trimArgument(std::string& arg)
{
  // Remove one or two - from front of argument name
  Strings::removePrefix(arg, "-");
  Strings::removePrefix(arg, "-");
}
}

RAPIOOptions::RAPIOOptions(const std::string& base)
{
  setHeader(Constants::RAPIOHeader + base);
  setDescription(
    base + ".  See examples.  Call o.setHeader, o.setAuthors, o.setDescription in declareOptions to change.");
  setAuthors("Robert Toomey");

  optional("verbose",
    "info",
    "Error log verbosity levels.  Increasing level prints more stuff: A given file path will read this value from a file periodically and update.");
  addSuboption("verbose", "severe", "Print only the most severe errors");
  addSuboption("verbose", "info", "Print general information as well as errors");
  addSuboption("verbose", "debug", "Print everything, also turn on signal stacktraces");
  setEnforcedSuboptions("verbose", false);
  addGroup("verbose", "LOGGING");

  // grid2D("GridTest", "nw(34.5, 91.5) se(20.2, 109.5)", "Testing grid 2d");

  optional("iconfig",
    "",
    "Input URL to read a configuration file. Command line overrides.");
  addGroup("iconfig", "CONFIG");

  optional("oconfig",
    "",
    "Output URL to write a configuration file, using parameters from all sources.");
  addGroup("oconfig", "CONFIG");

  // The help group
  boolean("help",
    "Print out parameter help information.");
  addGroup("help", "HELP");
  myMacroApplied = false;
  myLeftovers    = "";
}

void
RAPIOOptions::addPostLoadedHelp()
{
  addAdvancedHelp("iconfig",
    "Can end with .xml for WDSS2 file, .config for HMET file, .json for JSON file. This will parse the given file and add found parameters. Anything passed to command line will override the contents of the passed in file.");
  addAdvancedHelp("oconfig",
    "Can end with .xml for WDSS2 file, .config for HMET file, .json for a JSON file. This will use all parameters if any from iconfig, override with command line arguments, then generate a new output file.  You can convert from one style to another as well.");
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
RAPIOOptions::setExample(const std::string& e)
{
  myExample = e;
}

void
RAPIOOptions::setTextOnlyMacro(const std::string& m)
{
  myMacro = m;
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

void
RAPIOOptions::declareLegacyGrid()
{
  // Legacy grid for calling from declareOptions
  optional("t", "37 -100 20", "The top corner of grid");
  optional("b", "30.5 -93 1", "The bottom corner of grid");
  optional("s", "0.05 0.05 1", "The grid spacing");
  addGroup("t", "SPACE");
  addGroup("b", "SPACE");
  addGroup("s", "SPACE");
}

bool
RAPIOOptions::getLegacyGrid(
  LLCoverageArea& grid
)
{
  AngleDegs nwLatDegs, nwLonDegs;
  AngleDegs seLatDegs, seLonDegs;
  AngleDegs latSpacing, lonSpacing;
  size_t numX;
  size_t numY;

  // TOP
  std::string topcorner = getString("t");

  AngleDegs nwlatsp, nwlonsp;

  std::vector<std::string> nwpieces;

  Strings::split(topcorner, &nwpieces);
  if (nwpieces.size() != 3) {
    LogSevere("Failed top left grid specification \n");
    return false;
  } else {
    nwlatsp = atof(nwpieces[0].c_str()); // FIXME: catch...and use c++ silly
    nwlonsp = atof(nwpieces[1].c_str());
  }
  nwLatDegs = nwlatsp;
  nwLonDegs = nwlonsp;

  // BOTTOM
  std::string botcorner = getString("b");

  AngleDegs selatsp, selonsp;

  std::vector<std::string> sepieces;

  Strings::split(botcorner, &sepieces);
  if (sepieces.size() != 3) {
    LogSevere("Failed bottom right grid specification \n");
    return false;
  } else {
    selatsp = atof(sepieces[0].c_str());
    selonsp = atof(sepieces[1].c_str());
  }
  seLatDegs = selatsp;
  seLonDegs = selonsp;

  // SPACING
  std::string spacing = getString("s");
  AngleDegs slatsp, slonsp;
  std::vector<std::string> spieces;

  Strings::split(spacing, &spieces);
  if (spieces.size() != 3) {
    LogSevere("Failed spacing specification \n");
    return false;
  } else {
    slatsp = atof(spieces[0].c_str());
    slonsp = atof(spieces[1].c_str());
  }
  latSpacing = slatsp;
  lonSpacing = slonsp;

  // Ok calculate the GRID dimensions.  Feels backwards, normally with say map tiles
  // we do the x y first and get any spacing from that
  int x = (seLonDegs - nwLonDegs) / lonSpacing;

  if (x < 0) { x = -x; }
  int y = (nwLatDegs - seLatDegs) / latSpacing;

  if (y < 0) { y = -y; }

  numX = x;
  numY = y;

  grid.set(nwLatDegs, nwLonDegs,
    seLatDegs, seLonDegs,
    latSpacing, lonSpacing,
    numX,
    numY);

  return true;
} // RAPIOOptions::getLegacyGrid

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
    // FIXME: Noticed name duplicated..we're not using grid yet
    // here..but need to come back and test this
    std::string name2, value;
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
            invalue ? value += c : name2 += c;
            break;
      }
    }

    // ALIAS
    if (name2 == "t") { name2 = "nw"; }
    if (name2 == "b") { name2 = "se"; } lookup[name2] = value;
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

/** Count a list of arguments with a given filter */
size_t
RAPIOOptions::countArgs(const std::vector<Option>& options,
  OptionFilter                                   & a)
{
  size_t counter = 0;

  for (auto& o: options) {
    // Output the option
    if (a.show(o)) {
      counter++;
    }
  }
  return counter;
}

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
      // Import that status include the colors so fill spaces
      // don't get them.
      status = ColorTerm::fBlue + status + ColorTerm::fNormal;
      ColorTerm::wrapWithIndent(c1, c1, status);

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
          if (optpad.empty()) { // Existance only such as -r
            optpad = "\"\"";
          }
          size_t max = o.suboptionmax;

          while (optpad.length() < max) {
            optpad += " ";
          }
          std::string out = ColorTerm::fBlue + ColorTerm::bold(optpad)
            + " --" + i.description;
          ColorTerm::wrapWithIndent(c1 + 4, c1 + 4 + max + 3, out);
        }
      }
      if (advancedHelp && (o.advancedHelp != "")) {
        std::vector<std::string> lines;
        Strings::splitWithoutEnds(o.advancedHelp, '\n', &lines);
        // s << fGreen;
        // s << setw(c1+2) << left << ""; // Indent 1 column
        int d = 5;
        s << ColorTerm::bold("DETAILED HELP:\n");
        s << setw(d) << left << ""; // Indent 1 column
        for (auto& l:lines) {
          // ColorTerm::wrapWithIndent(d, 0, o.advancedHelp);
          ColorTerm::wrapWithIndent(d, 0, l);
        }
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
    printHelpHelp();
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

bool
RAPIOOptions::verifyAllRecognized()
{
  // Couple of error cases can occur in the arguments
  // One: actually non-processed stuff in the arguments (bad format)
  // Two: processed but not defined (good format, but not used)
  // We have stuff left not processed...
  if (!myLeftovers.empty()) {
    std::cout
      << "This part of your command line was unrecognized and ignored.  Maybe a typo?\n>>";
    std::cout << myLeftovers << "\n";
  }

  bool good = true;

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
    good = false;
    printHelpHelp();
  }
  return (good);
} // RAPIOOptions::verifyAllRecognized

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

  // Dump example
  std::cout << ColorTerm::bold("EXAMPLE:\n");

  // Dump given example
  if (!myExample.empty()) {
    std::cout << setw(d) << left << ""; // Indent 1 column
    std::cout << myName << " " << myExample << "\n";
  }
  // Dump generated example from the required options
  // We'll have to column format this stuff eventually...
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

  // Collection option groups in desired order
  std::vector<std::string> layout = { "I/O", "", "TIME", "LOGGING", "CONFIG", "HELP" };

  // Get all groups not matching preordered, sort and insert
  // into position wanted.
  std::vector<std::string> user;

  for (const auto& o:allOptions) {
    for (const auto& g: o.groups) {
      // If not in our predefined group...
      if (std::find(layout.begin(), layout.end(), g) != layout.end()) {
        continue;
      }
      // ... and not already included ...
      if (std::find(user.begin(), user.end(), g) != user.end()) {
        continue;
      }
      user.push_back(g);
    }
  }
  std::sort(user.begin(), user.end());
  layout.insert(layout.begin() + 2, user.begin(), user.end());

  // Dump options
  for (auto& x:layout) {
    FilterGroup l(x);
    if (countArgs(allOptions, l) > 0) {
      std::cout << ColorTerm::bold(x.empty() ? "OPTIONS:\n" : x + ":\n");
    }
    dumpArgs(allOptions, l);
  }
} // RAPIOOptions::dumpArgs

namespace {
/** Is this allowed in config file, if not ignore.  Some options
 * will mess with our processing */
bool
allowInConfig(const std::string& o)
{
  if (o == "iconfig") { return false; }
  if (o == "oconfig") { return false; }
  if (o == "help") { return false; } // Don't let config file dump help lol
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
        // Don't enforce declaration with HMET files, there are too many options
        trimArgument(options[i]);
        storeParsedArg(options[i], values[i], false, true);
      } else {
        // LogDebug("**IGNORING*** " << options[i] << " == " << values[i] << "\n");
      }
    }
  } else {
    // Reader should complain already
    // LogSevere("Couldn't read configuration file at " << aURL << "\n");
    exit(1);
  }
} // RAPIOOptions::readConfigFile

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

void
RAPIOOptions::setHelpFields(const std::vector<std::string>& list)
{
  // Left over fields get the set of asked for help fields
  // so basically "myalg help field1 field2 field3" which will be
  // used to load advanced help settings.

  // FIXME: Humm this is from left over options (non -), issue or not?
  if (list.size() > 2) { // help is one of them... "help" ""
    std::vector<Option> allOptions;
    OptionFilter all;

    // Make it dynamic based on what else is left on line
    for (size_t i = 0; i < list.size(); i += 2) {
      auto& c = list[i];     // -argument
      auto& v = list[i + 1]; // value

      // Skip any arguments
      if (!c.empty()) { continue; }
      // Skip help for help itself...hummm
      // We always make sure that 'help' becomes an arg '--help'
      if (c == "help") { continue; }

      // Try to find a group matching argument...
      // So if user types 'alg help time' they get detailed time options.
      std::string group = Strings::makeUpper(v);
      if (group == "OPTIONS") { group = ""; }
      FilterGroup g(group);
      sortOptions(allOptions, g);

      // If not found, try to find a variable matching argument...
      if (allOptions.size() == 0) {
        FilterName aName(v); // Capital?
        sortOptions(allOptions, aName);
      }

      // Dump advanced help for this if found
      if (allOptions.size() > 0) {
        for (auto o:allOptions) {
          myHelpOptions.push_back(o);
        }
        allOptions.clear();
      } else {
        std::cout << "-->Unknown option/variable: '" << v << "'\n";
      }
    }
  }
} // RAPIOOptions::setHelpFields

void
RAPIOOptions::dumpHelp()
{
  // Here we handle help something, where we filter groups and then variables
  // to show just the help for those.  It's useful for algorithms with lots of
  // options
  // FIXME: Humm this is from left over options (non -), issue or not?
  if (myRawArgs.size() > 2) { // help is one of them... "help" ""
    std::vector<Option> allOptions;
    OptionFilter all;

    // Make it dynamic based on what else is left on line
    for (size_t i = 0; i < myRawArgs.size(); i += 2) {
      auto& c = myRawArgs[i];     // -argument
      auto& v = myRawArgs[i + 1]; // value

      // Skip any arguments
      if (!c.empty()) { continue; }
      // Skip help for help itself...hummm
      // We always make sure that 'help' becomes an arg '--help'
      if (c == "help") { continue; }

      // Try to find a group matching argument...
      // So if user types 'alg help time' they get detailed time options.
      std::string group = Strings::makeUpper(v);
      if (group == "OPTIONS") { group = ""; }
      FilterGroup g(group);
      sortOptions(allOptions, g);

      // If not found, try to find a variable matching argument...
      if (allOptions.size() == 0) {
        FilterName aName(v); // Capital?
        sortOptions(allOptions, aName);
      }

      // Dump advanced help for this if found
      if (allOptions.size() > 0) {
        std::cout << ColorTerm::bold(group + ":\n");
        dumpArgs(allOptions, all, false, true);
        allOptions.clear();
      } else {
        std::cout << "-->Unknown option/variable: '" << v << "'\n";
      }
    }
  } else {
    // Single help argument, dump basic non-detailed help
    dumpArgs();
  }
} // RAPIOOptions::dumpHelp

std::vector<string>
RAPIOOptions::expandArgs(const std::vector<std::string>& args, std::string& leftovers)
{
  // Expand into pairs, so return args will be multiple of 2 in form of:
  // argumentname value
  // We don't remove the arg name markers here like '-' so this function
  // can be called again after applying macros.
  // "-c=12" --> "-c" "12"
  // "-c 12" --> "-c" "12"
  // "leftover" --> "" "leftover"
  leftovers = "";
  std::vector<std::string> expanded;

  for (size_t i = 0; i < args.size(); ++i) {
    auto& at = args[i];
    auto l   = at.length();
    auto n   = (i == args.size() - 1) ? "" : args[i + 1];

    if (isArgument(at)) {
      // -z=zvalue
      std::vector<std::string> twoargs;
      Strings::splitOnFirst(at, '=', &twoargs);
      if (twoargs.size() > 1) {
        expanded.push_back(twoargs[0]);
        expanded.push_back(twoargs[1]);
        // -z value
      } else {
        expanded.push_back(at);
        if (isArgument(n)) {
          expanded.push_back(""); // -z no value after
        } else {
          expanded.push_back(n);
          i++;
        }
      }
    } else {
      // Unmatched text string
      if (!at.empty()) {    // skip completely empty pairs
        if (at == "help") { // make 'help' same as --help
          expanded.push_back("help");
          expanded.push_back("");
        } else { // otherwise no argument text
          expanded.push_back("");
          expanded.push_back(at);
          if (leftovers.empty()) {
            leftovers = at;
          } else {
            leftovers = leftovers + " " + at;
          }
        }
      }
    }
  }
  // for (size_t i = 0; i < expanded.size(); i += 2) {
  //  LogSevere("Expanded to " << expanded[i] << " == '" << expanded[i + 1] << "'\n");
  // }
  return expanded;
} // RAPIOOptions::expandArgs

void
RAPIOOptions::applyLeftOverMacro(const std::string& macro, std::string& leftoverStr, std::vector<std::string>& expanded)
{
  if (macro.empty()) { return; }
  if (leftoverStr.empty()) { return; }

  // Remove any leftover strings (since macro groups them)
  std::vector<std::string> newargs;

  for (size_t i = 0; i < expanded.size(); i += 2) {
    auto& c = expanded[i];     // -argument
    auto& v = expanded[i + 1]; // value
    if (!c.empty()) {
      newargs.push_back(c);
      newargs.push_back(v);
    }
  }

  // Apply macro by adding new args
  std::vector<std::string> macroargs;

  Strings::split(macro, ' ', &macroargs);
  for (auto& n: macroargs) { // each 'arg' in the macro
    if (!n.empty()) {
      std::string aa = n;
      Strings::replace(aa, "%s", leftoverStr);
      newargs.push_back(aa); // add it, needs expansion
    }
  }
  // Reexpand the args (still works on already expanded args)
  expanded       = expandArgs(newargs, leftoverStr);
  myMacroApplied = true;
}

bool
RAPIOOptions::processArgs(const int& argc, char **& argv)
{
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

  // Expand the arguments into full pairs c = v
  auto expanded = expandArgs(args, myLeftovers);

  // ------------------------------------------------
  // Check for help
  bool haveHelp = false;

  for (size_t i = 0; i < expanded.size(); i += 2) {
    auto& c = expanded[i];     // -argument
    auto& v = expanded[i + 1]; // value

    if (c == "help") { haveHelp = true; }
  }

  // Expand macro using leftovers if available
  if (!haveHelp) {
    applyLeftOverMacro(myMacro, myLeftovers, expanded);
  }

  // Store final arguments
  for (size_t i = 0; i < expanded.size(); i += 2) {
    auto c = expanded[i];     // -argument
    auto v = expanded[i + 1]; // value
    if (!c.empty()) {
      trimArgument(c);
      storeParsedArg(c, v);
    }
  }

  // We can use the pull methods now...note we have yet to verify
  // we have all required/etc..
  setIsProcessed();

  // Read in given configuration file, if any
  std::string fileName = getString("iconfig");

  if ((!haveHelp) && (!fileName.empty())) {
    readConfigFile(fileName);
  }

  if (haveHelp) {
    setHelpFields(expanded); // Update help field
  }

  // We have ALL args in here now
  // Only needed for help probably
  myRawArgs = expanded;

  return haveHelp;
} // RAPIOOptions::processArgs

void
RAPIOOptions::dumpHeaderLine()
{
  #if 0
  // Start outputting now...
  std::string header   = "";
  size_t myOutputWidth = ColorTerm::getOutputWidth();
  for (size_t i = 0; i < myOutputWidth; i++) {
    header += "-";
  }
  std::cout << header << "\n";
  #endif

  if (!myHeader.empty()) {
    ColorTerm::wrapWithIndent(0, 0, myHeader);
  }

  // std::string built = ColorTerm::bold("Binary Built:"+std::string(__DATE__) + "
  // " + __TIME__;
  // wrapWithIndent(0, 0, built);
}

void
RAPIOOptions::initToSettings()
{
  // Allow colors if possible and turned on in global config
  ColorTerm::setColors(Log::useColors && ColorTerm::haveColorSupport());
}

/** Finalize args by dumping info to user and possiblity ending */
bool
RAPIOOptions::finalizeArgs(bool haveHelp)
{
  dumpHeaderLine();

  // Help dump
  if (haveHelp) {
    dumpHelp();
    return false;
  }

  // Check for any unrecognized options
  if (!verifyAllRecognized()) { return false; }

  // Check for missing required arguments and complain
  if (!verifyRequired()) { return false; }

  // Check for invalid suboption in arguments and complain
  if (!verifySuboptions()) { return false; }

  // Check for configuration output and write out now...
  std::string ofileName = getString("oconfig");

  if (!ofileName.empty()) { // help already processed now
    writeConfigFile(ofileName);
    return false;
  }

  return true;
} // RAPIOOptions::processArgs
