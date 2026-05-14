#include "rOptionList.h"

#include "rError.h"
#include "rStrings.h"
#include "rOS.h"
#include "rColorTerm.h"

#include <algorithm>
#include <iostream>
#include <iomanip>

#include "rError.h"
#include "rTime.h"
#include "rStrings.h"
#include "rColorTerm.h"
#include "rOS.h"

#include <sys/ioctl.h>
#include <unistd.h>
#include <fstream>
#include <algorithm>
#include <iostream>

// Generated header
#include "config.h"

using namespace rapio;
using namespace std;

unsigned int OptionList::max_arg_width  = 5;
unsigned int OptionList::max_name_width = 10;

namespace {
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

OptionList::OptionList() : isProcessed(false)
{ }

void
OptionList::addOption(const Option& o)
{
  const std::string opt = o.opt;

  optionMap[opt] = o;

  if (opt.length() > max_arg_width) {
    max_arg_width = opt.length();
  }

  if (o.name.length() > max_name_width) {
    max_name_width = o.name.length();
  }
}

Option *
OptionList::makeOption(
  bool             required,
  bool             boolean,
  bool             system,
  const std::string& opt,
  const std::string& name,
  const std::string& usage,
  const std::string& extraIn
)
{
  Option * have = getOption(opt);

  if (have) {
    throw StartupException(fmt::format("Code error: Option '{}' is already declared in OptionList.", opt));
  }

  Option o; // on stack..humm damn Java, lol

  o.required = required;
  o.boolean  = boolean;
  o.system   = system;
  o.opt      = opt;
  o.name     = name;
  o.enforceSuboptions = true;
  o.hidden = false;

  if (o.name.length() > 0) {
    o.name[0] = toupper(o.name[0]);
  }
  o.usage = usage;

  // Clean up usage.  Capitol the first letter always...
  if (o.usage.length() > 0) {
    o.usage[0] = toupper(o.usage[0]);
    o.usage    = "--" + o.usage;
  }
  std::string extra = extraIn;

  // Maybe this field could be shared..?
  if (required) {
    if (extra == "") {
      o.example = "???";
    } else {
      o.example = extra; // Required have an example given
    }
  } else {
    o.defaultValue = extra; // Optionals always have a default value
  }
  o.parsedValue  = "";
  o.parsed       = false;
  o.suboptionmax = 0;

  addOption(o);

  return (&optionMap[opt]);
} // OptionList::makeOption

void
OptionList::require(const std::string& opt,
  const std::string& exampleArg, const std::string& usage)
{
  makeOption(true, false, false, opt, "", usage, exampleArg);
}

void
OptionList::optional(const std::string& opt,
  const std::string                   & defaultValue,
  const std::string                   & usage)
{
  makeOption(false, false, false, opt, "", usage, defaultValue);
}

void
OptionList::setDefaultValue(const std::string& opt,
  const std::string                          & defaultValue)
{
  Option * op = getOption(opt);

  if (op) {
    auto& o = *op;
    if (o.required) {
      o.required = false; // make optional
    }
    o.defaultValue = defaultValue;
  }
}

/** Declare a boolean algorithm variable */
void
OptionList::boolean(const std::string& opt,
  const std::string                  & usage)
{
  makeOption(false, true, false, opt, "", usage, "false");
}

Option *
OptionList::getOption(const std::string& opt)
{
  auto i = optionMap.find(opt);

  if (i != optionMap.end()) {
    return (&i->second);
  }
  return (nullptr);
}

bool
OptionList::isParsed(const std::string& key)
{
  Option * o = getOption(key);

  if (o != nullptr) {
    return o->parsed;
  }
  return false;
}

void
OptionList::storeParsedArg(const std::string& arg, const std::string& value, const bool enforceStrict,
  const bool fromiconfig)
{
  // Store the option...
  Option * o = getOption(arg);

  if (o) {
    // If already parsed what to do...
    // If from iconfig, we just ignore, because we override the setting..
    // If duplicated from command line, maybe we want to warn...
    if (!o->parsed) {
      o->parsed = true;

      if (o->boolean) {
        // Because it is there...ignore any value not "false"
        if (value == "false") {
          o->parsedValue = "false";
        } else {
          o->parsedValue = "true";
        }
      } else {
        o->parsedValue = value;
      }
    } else {
      if (!fromiconfig) {
        // Because of macros, it's confusing to allow duplicates
        throw StartupException(fmt::format("Duplicated commandline option for '{}'.", arg));
      }
    }
  } else {
    // We were giving an option we don't know about...
    // FIXME: Hide the internals of option, right?
    Option o2;
    o2.opt         = arg;
    o2.parsedValue = value;
    o2.parsed      = true;
    // Just set them, not used in unused
    o2.required = false;
    o2.boolean  = false;
    o2.system   = false;
    o2.enforceSuboptions = false;
    o2.suboptionmax      = 0;

    if (enforceStrict) {
      unusedOptionMap[arg] = o2;
    } else {
      addOption(o2);
    }
  }
} // OptionList::storeParsedArg

void
OptionList::sortOptions(std::vector<Option *>& allOptions,
  OptionFilter                               & a)
{
  // Filter and Sort options how we want to display.
  for (auto& i:optionMap) {
    if (a.show(i.second)) {
      allOptions.push_back(&i.second);
    }
  }
  std::sort(allOptions.begin(), allOptions.end());
}

std::string
OptionList::getString(const std::string& key)
{
  if (!isProcessed) {
    throw StartupException("Code error: Have to call processArgs before calling getString.");
  }
  std::string s = "";
  Option * have = getOption(key);

  if (have != nullptr) {
    if (have->parsed) {
      s = have->parsedValue;
    } else {
      s = have->defaultValue;
    }
  } else {
    fLogSevere("WARNING - Trying to read uninitialized parameter \"{}\"", key);
  }
  return (s);
}

bool
OptionList::getBoolean(const std::string& opt)
{
  const std::string s = getString(opt);

  return (s != "false");
}

float
OptionList::getFloat(const std::string& opt)
{
  const std::string s = getString(opt);

  if (s.empty()) { return 0.0f; }
  try {
    return std::stof(s);
  } catch (const std::exception& e) {
    // fLogSevere("Failed to parse float for option '{}': {}", opt, s);
    return 0.0f;
  }
}

int
OptionList::getInteger(const std::string& opt)
{
  const std::string s = getString(opt);

  if (s.empty()) { return 0; }
  try {
    return std::stoi(s);
  } catch (const std::exception& e) {
    // fLogSevere("Failed to parse integer for option '{}': {}", opt, s);
    return 0;
  }
}

void
OptionList::setEnforcedSuboptions(const std::string& key, bool flag)
{
  Option * have = getOption(key);

  if (have != nullptr) {
    have->setEnforcedSuboptions(flag);
  }
}

bool
OptionList::isInSuboptions(const std::string& key)
{
  Option * have = getOption(key);

  if (have != nullptr) {
    return have->isInSuboptions();
  }
  return false;
}

void
OptionList::addSuboption(const std::string& sourceopt, const std::string& opt,
  const std::string& description)
{
  Option * have = getOption(sourceopt);

  if (have != nullptr) {
    have->addSuboption(opt, description);
  } else {
    throw StartupException(fmt::format(
              "Code error: Trying to add suboption '{}' to option '{}'. You must add the option first.", opt,
              sourceopt));
  }
}

void
OptionList::addGroup(const std::string& sourceopt, const std::string& group)
{
  Option * have = getOption(sourceopt);

  if (have) {
    have->addGroup(group);
  } else {
    throw StartupException(fmt::format(
              "Code error: Trying to add group '{}' to option '{}'. You must add the option first.", group, sourceopt));
  }
}

void
OptionList::setHidden(const std::string& sourceopt)
{
  Option * have = getOption(sourceopt);

  if (have) {
    have->hidden = true;
  } else {
    throw StartupException(fmt::format(
              "Code error: Trying to set hidden to missing option '{}'. You must add the option first.", sourceopt));
  }
}

bool
OptionList::wantAdvancedHelp(const std::string& sourceopt)
{
  bool want     = false;
  Option * have = getOption(sourceopt);

  if (have) {
    for (auto o:myHelpOptions) {
      if (o == have->opt) {
        want = true;
        break;
      }
    }
  }
  return want;
}

void
OptionList::addAdvancedHelp(const std::string& sourceopt,
  const std::string                          & help)
{
  if (help.empty()) {
    return;
  }

  Option * have = getOption(sourceopt);

  if (have) {
    have->advancedHelp = help;
    have->usage        = have->usage + " (See help " + have->opt + ")";
  } else {
    throw StartupException(fmt::format(
              "Code error: Trying to add detailed help to missing option '{}'. You must add the option first.",
              sourceopt));
  }
}

bool
OptionList::processArgs(const std::vector<std::string>& args)
{
  auto expanded = expandArgs(args);
  bool haveHelp = false;

  for (size_t i = 0; i < expanded.size(); i += 2) {
    if (expanded[i] == "helpRequestedPlease") { haveHelp = true; }
  }

  for (size_t i = 0; i < expanded.size(); i += 2) {
    auto c = expanded[i];
    auto v = expanded[i + 1];
    if (!c.empty()) {
      trimArgument(c);
      storeParsedArg(c, v);
    }
  }

  setIsProcessed();
  if (haveHelp) {
    setHelpFields(expanded);
  }

  myRawArgs = expanded;
  return haveHelp;
}

void
OptionList::setHelpFields(const std::vector<std::string>& list)
{
  // Left over fields get the set of asked for help fields
  // so basically "myalg help field1 field2 field3" which will be
  // used to load advanced help settings.

  myHelpOptionsHidden = false;

  // FIXME: Humm this is from left over options (non -), issue or not?
  if (list.size() > 2) { // help is one of them... "help" ""
    std::vector<Option *> allOptions;
    OptionFilter all;

    // Make it dynamic based on what else is left on line
    for (size_t i = 0; i < list.size(); i += 2) {
      auto& c = list[i];     // -argument
      auto& v = list[i + 1]; // value

      // Skip hidden
      if (v == "hidden") { myHelpOptionsHidden = true; continue; }

      // Skip any arguments (only help is c, rest are v )
      if (!c.empty()) { continue; }

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
          // Add only if not already there
          bool found = false;
          for (auto h:myHelpOptions) {
            if (h == o->opt) {
              found = true;
              break;
            }
          }
          if (!found) {
            // Do a key list because advanced not added yet
            myHelpOptions.push_back(o->opt);
          }
        }
        allOptions.clear();
      } else {
        std::cout << "-->set Unknown option/variable: '" << v << "'\n";
      }
    }
  }
} // OptionList::setHelpFields

HelpEntry
OptionList::createHelpEntry(const Option& o) const
{
  HelpEntry entry;

  entry.opt             = o.opt;
  entry.usage           = o.usage;
  entry.defaultValue    = o.defaultValue;
  entry.exampleValue    = o.example;
  entry.advancedHelp    = o.advancedHelp;
  entry.groups          = o.groups;
  entry.isRequired      = o.required;
  entry.isBoolean       = o.boolean;
  entry.isHidden        = o.hidden;
  entry.suboptionMaxLen = o.suboptionmax;

  for (const auto& sub : o.suboptions) {
    HelpSubEntry subEntry;
    subEntry.opt         = sub.opt;
    subEntry.description = sub.description;
    entry.suboptions.push_back(subEntry);
  }
  return entry;
}

std::vector<HelpEntry>
OptionList::exportHelpData() const
{
  std::vector<HelpEntry> entries;

  for (const auto& pair : optionMap) {
    entries.push_back(createHelpEntry(pair.second));
  }
  return entries;
}

std::vector<HelpEntry>
OptionList::getMissingRequired() const
{
  std::vector<HelpEntry> missing;

  for (const auto& pair : optionMap) {
    const Option& o = pair.second;
    // Logic from FilterMissingRequired
    if (o.required && (!o.parsed || o.parsedValue.empty())) {
      // Use your export translation logic to make a HelpEntry
      missing.push_back(createHelpEntry(o));
    }
  }
  return missing;
}

std::vector<HelpEntry>
OptionList::getInvalidSuboptions() const
{
  std::vector<HelpEntry> invalid;

  for (const auto& pair : optionMap) {
    const Option& o = pair.second;
    // Assuming your FilterBadSuboption logic here:
    if (!o.suboptions.empty() && o.enforceSuboptions) {
      std::string val = o.parsed ? o.parsedValue : o.defaultValue;
      bool found      = false;
      for (const auto& sub : o.suboptions) {
        if (sub.opt == val) { found = true; break; }
      }
      if (!found) {
        invalid.push_back(createHelpEntry(o));
      }
    }
  }
  return invalid;
}

std::vector<HelpEntry>
OptionList::getUnrecognizedOptions() const
{
  std::vector<HelpEntry> unrecognized;

  for (const auto& pair : unusedOptionMap) {
    unrecognized.push_back(createHelpEntry(pair.second));
  }
  return unrecognized;
}

OptionList::OptionList(const std::string& base)
{
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
}

void
OptionList::addPostLoadedHelp()
{
  addAdvancedHelp("iconfig",
    "Can end with .xml for WDSS2 file, .config for HMET file, .json for JSON file. This will parse the given file and add found parameters. Anything passed to command line will override the contents of the passed in file.");
  addAdvancedHelp("oconfig",
    "Can end with .xml for WDSS2 file, .config for HMET file, .json for a JSON file. This will use all parameters if any from iconfig, override with command line arguments, then generate a new output file.  You can convert from one style to another as well.");
}

void
OptionList::initToSettings()
{
  // Allow colors if possible and turned on in global config
  ColorTerm::setColors(Log::useColors && ColorTerm::haveColorSupport());
}

IOptionBackend::SeparatedArgs
OptionList::separateArguments(const std::vector<std::string>& args) const
{
  SeparatedArgs result;
  bool foundHelpRequest = false;

  for (size_t i = 0; i < args.size(); ++i) {
    auto& at = args[i];
    auto n   = (i == args.size() - 1) ? "" : args[i + 1];
    const bool helpString = ((at == "help") || (at == "--help"));

    if (isArgument(at) && !helpString) {
      result.flags.push_back(at); // It's a flag
      std::vector<std::string> twoargs;
      Strings::splitOnFirst(at, "=", twoargs);
      if (twoargs.size() == 1) {
        // --- NEW LOGIC: Check if it's a boolean ---
        std::string stripped = at;
        trimArgument(stripped);
        auto it     = optionMap.find(stripped);
        bool isBool = (it != optionMap.end() && it->second.boolean);

        bool consumesNext = !isBool;
        if (isBool) {
          std::string nLower = n;
          Strings::toLower(nLower);
          if ((nLower == "true") || (nLower == "false") ) {
            consumesNext = true; // explicitly passed boolean value
          }
        }
        // ------------------------------------------

        if (consumesNext && !isArgument(n) && !n.empty()) {
          result.flags.push_back(n); // It's the value for the flag
          i++;
        }
      }
    } else {
      if (!at.empty()) {
        if (helpString && !foundHelpRequest) {
          result.flags.push_back(at); // Treat help as a flag
          foundHelpRequest = true;
        } else {
          result.positionals.push_back(at); // It's a leftover/positional
        }
      }
    }
  }
  return result;
} // OptionList::separateArguments

std::vector<std::string>
OptionList::expandArgs(const std::vector<std::string>& args)
{
  myPositionalArgs.clear();
  std::vector<std::string> expanded;
  bool foundHelpRequest = false;

  for (size_t i = 0; i < args.size(); ++i) {
    auto& at = args[i];
    auto n   = (i == args.size() - 1) ? "" : args[i + 1];
    const bool helpString = ((at == "help") || (at == "--help"));

    if (isArgument(at) && !helpString) {
      std::vector<std::string> twoargs;
      Strings::splitOnFirst(at, "=", twoargs);
      if (twoargs.size() > 1) {
        expanded.push_back(twoargs[0]);
        expanded.push_back(twoargs[1]);
      } else {
        expanded.push_back(at);

        // --- NEW LOGIC: Check if it's a boolean ---
        std::string stripped = at;
        trimArgument(stripped);
        Option * opt = getOption(stripped);
        bool isBool  = (opt != nullptr && opt->boolean);

        bool consumesNext = !isBool;
        if (isBool) {
          std::string nLower = n;
          Strings::toLower(nLower);
          if ((nLower == "true") || (nLower == "false") ) {
            consumesNext = true;
          }
        }
        // ------------------------------------------

        if (!consumesNext || isArgument(n) || n.empty()) {
          expanded.push_back("");
        } else {
          expanded.push_back(n);
          i++;
        }
      }
    } else {
      if (!at.empty()) {
        if (helpString && !foundHelpRequest) {
          expanded.push_back("helpRequestedPlease");
          expanded.push_back("");
          foundHelpRequest = true;
        } else {
          expanded.push_back("");
          expanded.push_back(at);
          myPositionalArgs.push_back(at); // Track positional safely
        }
      }
    }
  }
  return expanded;
} // OptionList::expandArgs
