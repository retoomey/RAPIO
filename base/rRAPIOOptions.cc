#include "rRAPIOOptions.h"

#include "rOptionList.h" // engine for now
#include "rHelpFormatter.h"
#include "rColorTerm.h"
#include "rAlgConfigFile.h"
#include "rIOURL.h"
#include "rStrings.h"
#include "rTime.h"
#include "rOS.h"

#include <iostream>

#include "config.h" // generated header

using namespace rapio;
using namespace std;

RAPIOOptions::RAPIOOptions(const std::string& base)
  : myBackend(std::make_unique<OptionList>(base)),
  myName(base)
{
  setHeader(Constants::RAPIOHeader + base);
  setDescription(
    base + ".  See examples.  Call o.setHeader, o.setAuthors, o.setDescription in declareOptions to change.");
  setAuthors("Robert Toomey");
}

bool
RAPIOOptions::processArgs(int argc, char ** argv)
{
  std::vector<std::string> args;

  for (int k = 1; k < argc; k++) {
    args.push_back(std::string(argv[k]));
  }

  if (!myTextOnlyMacro.empty()) {
    auto separated = myBackend->separateArguments(args);

    if (!separated.positionals.empty()) {
      std::string posStr;
      for (size_t i = 0; i < separated.positionals.size(); ++i) {
        posStr += separated.positionals[i];
        if (i < separated.positionals.size() - 1) { posStr += " "; }
      }

      std::vector<std::string> macroArgs;
      Strings::split(myTextOnlyMacro, ' ', &macroArgs);

      // START with the user's original explicit flags (Preserves --verbose, etc.)
      args = separated.flags;

      // INJECT the macro args
      for (auto& mArg : macroArgs) {
        if (!mArg.empty()) {
          std::string expandedArg = mArg;
          Strings::replace(expandedArg, "%s", posStr);
          args.push_back(expandedArg);
        }
      }
      myMacroApplied = true;
    }
  }

  // Hand the final list (User Flags + Macro Flags) to the backend
  return myBackend->processArgs(args);
} // RAPIOOptions::processArgs

void RAPIOOptions::setName(const std::string& n){ myName = n; }

void RAPIOOptions::setDescription(const std::string& d){ myDescription = d; }

void RAPIOOptions::setAuthors(const std::string& a){ myAuthors = a; }

void RAPIOOptions::setExample(const std::string& e){ myExample = e; }

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

#if 0
void
RAPIOOptions::dumpHeaderLine()
{
  # if 0
  // Start outputting now...
  std::string header   = "";
  size_t myOutputWidth = ColorTerm::getOutputWidth();
  for (size_t i = 0; i < myOutputWidth; i++) {
    header += "-";
  }
  std::cout << header << "\n";
  # endif

  if (!myHeader.empty()) {
    ColorTerm::wrapWithIndent(0, 0, myHeader);
  }
  // std::string built = ColorTerm::bold("Binary Built:"+std::string(__DATE__) + "
  // " + __TIME__;
  // wrapWithIndent(0, 0, built);
  // FIXME: Maybe we make this in settings, use a token string
  // Right now with various compilers, etc. this is useful
  std::string built = " (" + std::string(COMPILER_NAME) + " " + COMPILER_VERSION + " C++" + CXX_VERSION
    + " " + BUILD_DATE + " " + BUILD_TIME + " UTC)";
  ColorTerm::wrapWithIndent(0, 0, built);
}

#endif // if 0

std::string
RAPIOOptions::expandMacros(const std::string& original) const
{
  std::string newString = original;
  std::string PWD       = OS::getCurrentDirectory();

  Strings::replace(newString, "{PWD}", PWD);
  return newString;
}

void
RAPIOOptions::boolean(const std::string& opt, const std::string& usage)
{
  myBackend->boolean(opt, expandMacros(usage));
}

void
RAPIOOptions::require(const std::string& opt, const std::string& exampleArg, const std::string& usage)
{
  myBackend->require(opt, expandMacros(exampleArg), expandMacros(usage));
}

void
RAPIOOptions::optional(const std::string& opt, const std::string& defaultValue, const std::string& usage)
{
  myBackend->optional(opt, expandMacros(defaultValue), expandMacros(usage));
}

namespace {
// Moved from OptionList.cc
bool
allowInConfig(const std::string& o)
{
  if (o == "iconfig") { return false; }
  if (o == "oconfig") { return false; }
  if (o == "help") { return false; }
  return true;
}
}

void
RAPIOOptions::readConfigFile(const std::string& string)
{
  // Get config file reader for given option string
  URL aURL(string);
  auto config = AlgConfigFile::getConfigFileForURL(aURL);

  if (config == nullptr) { exit(1); } // Bad parameter, no reader for it

  std::vector<std::string> options;
  std::vector<std::string> values;

  // Get the option/value pairs from the configuration file...
  if (config->readConfigURL(aURL, options, values)) {
    if (options.size() != values.size()) {
      exit(1);
    }
    // ... and store them in our options.
    for (size_t i = 0; i < options.size(); ++i) {
      if (allowInConfig(options[i])) {
        // fLogDebug("STORING {} == {}", options[i], values[i]);
        // Don't enforce declaration with HMET files, there are too many options
        std::string arg = options[i];
        // Clean up prefixes just in case
        Strings::removePrefix(arg, "-");
        Strings::removePrefix(arg, "-");

        // Inject into the backend parser!
        storeParsedArg(arg, values[i], false, true);
      } else {
        // fLogDebug("**IGNORING*** {} == {}", options[i], values[i]);
      }
    }
  } else {
    // Reader should complain already
    // fLogSevere("Couldn't read configuration file at {}", aURL.toString());
    exit(1);
  }
} // RAPIOOptions::readConfigFile

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

  // Use the Help Data DTOs to discover what options exist in the backend
  std::vector<HelpEntry> allOpts = myBackend->exportHelpData();

  for (const auto& opt : allOpts) {
    if (allowInConfig(opt.opt)) {
      options.push_back(opt.opt);
      // getString automatically returns parsedValue OR defaultValue, exactly what we want!
      values.push_back(getString(opt.opt));
    }
  }

  // Send to the writer (using the myName we moved to the Facade earlier)
  if (config->writeConfigURL(aURL, myName, options, values)) {
    std::cout << "Successfully generated configuration file " << aURL << "\n";
  }
}

void
RAPIOOptions::grid2D(const std::string& opt, const std::string& defaultValue, const std::string& usage)
{
  optional(opt, defaultValue, usage);
  addGroup(opt, "SPACE");
}

void
RAPIOOptions::declareLegacyGrid(const std::string& defaultGrid)
{
  // Legacy grid options
  optional("t", "", "The top corner of grid");
  optional("b", "", "The bottom corner of grid");
  optional("s", "", "The grid spacing");
  setHidden("t");
  setHidden("b");
  setHidden("s");
  addGroup("t", "SPACE");
  addGroup("b", "SPACE");
  addGroup("s", "SPACE");

  // Standard grid language default
  grid2D("grid", defaultGrid, "Grid language (also -t,-b,-s legacy)");
  addAdvancedHelp("grid",
    "Grid language: nw(lat,lon) se(lat,lon) s(deltalat,deltalon) h(lowestKMS,highestKMS, key).  The height key can be a pattern such as NMQWD, WISH, ARPS, or an increment number in KMs.  So, for example, h(0.5, 3, .5) will generate 500, 1000, 1500, 2000, 2500, 3000 meter layers.");
}

bool
RAPIOOptions::getGrid(const std::string& name, LLCoverageArea& grid)
{
  std::string gridstr = getString(name);

  if (!grid.parse(gridstr, "", "", "")) {
    exit(1);
  }
  return true;
}

bool
RAPIOOptions::getLegacyGrid(LLCoverageArea& grid)
{
  std::string topcorner = getString("t");
  std::string botcorner = getString("b");
  std::string spacing   = getString("s");
  std::string gridstr   = getString("grid");

  if (!grid.parse(gridstr, topcorner, botcorner, spacing)) {
    exit(1);
  }
  return true;
}

bool
RAPIOOptions::finalizeArgs(bool haveHelp)
{
  // 1. Build the formatter using the Backend's metadata getters
  HelpFormatter formatter(getName(), getDescription(), getAuthors(), getExample(), getHeader()
  );

  // 2. Handle intentional help request
  if (haveHelp) {
    std::vector<HelpEntry> helpData = myBackend->exportHelpData();

    std::vector<std::string> requestedHelp = myBackend->getRequestedAdvancedHelp();
    bool advancedHelp = !requestedHelp.empty();
    bool helpHidden   = myBackend->isHelpHiddenRequested();

    if (advancedHelp) {
      std::vector<HelpEntry> filteredData;
      for (const auto& optName : requestedHelp) {
        auto it = std::find_if(helpData.begin(), helpData.end(),
            [&optName](const HelpEntry& entry) {
          return entry.opt == optName;
        });
        if (it != helpData.end()) { filteredData.push_back(*it); }
      }
      formatter.printHelp(filteredData, true, true);
    } else {
      formatter.printHelp(helpData, helpHidden, false);
    }
    return false; // Exit program
  }

  // 3. Handle Configuration Input
  std::string fileName = myBackend->getString("iconfig");

  if (!fileName.empty()) {
    // Note: Assuming readConfigFile is also moved to the Facade,
    // or you call myBackend->readConfigFile(fileName) for now.
    readConfigFile(fileName);
  }

  // 4. Validation: Unrecognized Options
  auto unrecognized = myBackend->getUnrecognizedOptions();

  auto posArgs = myBackend->getPositionalArgs();
  std::string leftovers;

  for (size_t i = 0; i < posArgs.size(); ++i) {
    leftovers += posArgs[i];
    if (i < posArgs.size() - 1) { leftovers += " "; }
  }

  if (!unrecognized.empty() || !leftovers.empty()) {
    if (!leftovers.empty()) {
      std::cout << "This part of your command line was unrecognized and ignored:\n>> "
                << leftovers << "\n";
    }
    if (!unrecognized.empty()) {
      std::cout << ColorTerm::bold("UNRECOGNIZED OPTIONS:\n");
      formatter.printHelp(unrecognized, true, true);
    }
    return false;
  }

  // 5. Validation: Missing Required Options
  auto missing = myBackend->getMissingRequired();

  if (!missing.empty()) {
    std::cout << "Missing " << ColorTerm::bold("(" + std::to_string(missing.size()) + ")")
              << " REQUIRED arguments: \n";
    formatter.printHelp(missing, true, false);
    return false;
  }

  // 6. Validation: Bad Suboptions
  auto badSubs = myBackend->getInvalidSuboptions();

  if (!badSubs.empty()) {
    std::cout << "BAD option choice for " << ColorTerm::bold("(" + std::to_string(badSubs.size()) + ")")
              << " arguments:\n";
    formatter.printHelp(badSubs, true, true);
    return false;
  }

  // 7. Handle Configuration Output
  std::string ofileName = myBackend->getString("oconfig");

  if (!ofileName.empty()) {
    writeConfigFile(ofileName);
    return false;
  }

  return true;
} // RAPIOOptions::finalizeArgs
