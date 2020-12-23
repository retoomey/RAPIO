#include "rOptionList.h"

#include "rError.h"
#include "rStrings.h"
#include "rOS.h"

#include <algorithm>
#include <iostream>

using namespace rapio;
using namespace std;

unsigned int OptionList::max_arg_width  = 5;
unsigned int OptionList::max_name_width = 10;

OptionList::OptionList() : isProcessed(false)
{ }

std::string
OptionList::replaceMacros(const std::string& original)
{
  std::string newString = original;

  std::string PWD = OS::getCurrentDirectory();
  Strings::replace(newString, "{PWD}", PWD);
  return (newString);
}

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
    std::cout << "WARNING: Code error: Option '" << opt
              << "' is already declared in OptionList.\n";
    exit(1);
  }
  Option o; // on stack..humm damn Java, lol
  o.required = required;
  o.boolean  = boolean;
  o.system   = system;
  o.opt      = opt;
  o.name     = name;
  o.enforceSuboptions = true;

  if (o.name.length() > 0) {
    o.name[0] = toupper(o.name[0]);
  }
  o.usage = replaceMacros(usage);

  // Clean up usage.  Capitol the first letter always...
  if (o.usage.length() > 0) {
    o.usage[0] = toupper(o.usage[0]);
    o.usage    = "--" + o.usage;
  }
  std::string extra = replaceMacros(extraIn);

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

/** Declare a required algorithm variable */
Option *
OptionList::require(const std::string& opt,
  const std::string& exampleArg, const std::string& usage)
{
  return (makeOption(true, false, false, opt, "", usage, exampleArg));
}

/** Declare an optional algorithm variable */
Option *
OptionList::optional(const std::string& opt,
  const std::string                   & defaultValue,
  const std::string                   & usage)
{
  return (makeOption(false, false, false, opt, "", usage, defaultValue));
}

/** Declare a boolean algorithm variable */
Option *
OptionList::boolean(const std::string& opt,
  const std::string                  & usage)
{
  return (makeOption(false, true, false, opt, "", usage, "false"));
}

/*
 * namespace {
 * std::string
 * getMapString(
 * std::map<std::string, std::string>& lookup, const std::string& name)
 * {
 * std::map<std::string, std::string>::iterator iter;
 * std::string s = "";
 * iter = lookup.find(name);
 *
 * if (iter != lookup.end()) {
 *  s = iter->second;
 * }
 * return (s);
 * }
 * }
 */

Option *
OptionList::getOption(const std::string& opt)
{
  auto i = optionMap.find(opt);

  std::string s = "";

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
OptionList::storeParsedArg(const std::string& arg, const std::string& value, const bool enforceStrict)
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
        o->parsedValue = "true"; // Because it is there...ignore any value not
                                 // "false"

        if (value == "false") {
          o->parsedValue = "false";
        } else {
          o->parsedValue = "true";
        }
      } else {
        o->parsedValue = value;
      }
    } else {
      // Humm...keep track here?
    }
  } else {
    // We were giving an option we don't know about...
    // FIXME: Hide the internals of option, right?
    Option o;
    o.opt         = arg;
    o.parsedValue = value;
    o.parsed      = true;
    // Just set them, not used in unused
    o.required = false;
    o.boolean  = false;
    o.system   = false;
    o.enforceSuboptions = false;
    o.suboptionmax      = 0;

    if (enforceStrict) {
      unusedOptionMap[arg] = o;
    } else {
      addOption(o);
    }
  }
} // OptionList::storeParsedArg

void
OptionList::sortOptions(std::vector<Option>& allOptions,
  OptionFilter                             & a)
{
  // Filter and Sort options how we want to display.
  for (auto& i:optionMap) {
    if (a.show(i.second)) {
      allOptions.push_back(i.second);
    }
  }
  std::sort(allOptions.begin(), allOptions.end());
}

std::string
OptionList::getString(const std::string& key)
{
  if (!isProcessed) {
    LogSevere(
      "Have to call processArgs before calling getString. This is a code error\n");
    exit(1);
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
    LogSevere(
      "WARNING - Trying to read uninitialized parameter \"" << key << "\"\n");
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

  return (atof(s.c_str()));
}

int
OptionList::getInteger(const std::string& opt)
{
  const std::string s = getString(opt);

  return (atoi(s.c_str()));
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
    std::cout << "WARNING: Code error: Trying to add suboption " << opt
              << " to missing option '" << opt << "'\n";
    std::cout
      << "You must add the option first with boolean, required or optional\n";
    exit(1);
  }
}

void
OptionList::addGroup(const std::string& sourceopt, const std::string& group)
{
  Option * have = getOption(sourceopt);

  if (have) {
    have->addGroup(group);
  } else {
    std::cout << "WARNING: Code error: Trying to add group " << group
              << " to missing option '" << sourceopt << "'\n";
    std::cout
      << "You must add the option first with boolean, required or optional\n";
    exit(1);
  }
}

void
OptionList::addAdvancedHelp(const std::string& sourceopt,
  const std::string                          & help)
{
  Option * have = getOption(sourceopt);

  if (have) {
    have->advancedHelp = help;
    have->usage        = have->usage + " (See help " + have->opt + ")";
  } else {
    std::cout
      << "WARNING: Code error: Trying to add detailed help to missing option '"
      << sourceopt << "'\n";
    std::cout
      << "You must add the option first with boolean, required or optional\n";
    exit(1);
  }
}
