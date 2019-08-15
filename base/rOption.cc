#include "rOption.h"

#include "rStrings.h"

using namespace rapio;
using namespace std;

// The default sort is alphabetical
bool
Option::operator < (const Option& rhs) const
{
  // We want lowercase i before uppercase I, but always before o, O
  const std::string tmpl = Strings::makeLower(this->opt);
  const std::string tmpr = Strings::makeLower(rhs.opt);

  if (tmpl == tmpr) { // So 'i' == 'I' here, let lowercase win
    return (this->opt > rhs.opt);
  }
  // Else based on lowercase spelling
  return (tmpl < tmpr);
}

bool
FilterBadSuboption::show(const Option& opt)
{
  bool bad_suboption = false;

  if (opt.suboptions.size() > 0) {
    size_t found = 0;

    // If we have suboptions, it better match one.
    // Required should already be checked to 'exist', so here we just
    // need to look at defaultValue or parsedValue
    std::string value;

    if (opt.parsed) {
      value = opt.parsedValue;
    } else { value = opt.defaultValue; }

    // The choice passed in may be a collection of suboptions such as "A B C
    // D',
    // In this case we have to check each value
    // Note we allow "" to pass the test...
    std::vector<string> allChoices;
    size_t count = Strings::split(value, ' ', &allChoices);

    for (auto& i: opt.suboptions) {
      // Go through each substring passed
      for (auto& j: allChoices) {
        if (i.opt == j) {
          found++; // found a match in list
          // break; // Allow dups by not breaking.
        }
      }
    }

    // Ok, we should have a 'hit' for each one...or bad string passed in.
    if (!(found == count)) { bad_suboption = true; }
  }
  return (bad_suboption);
} // show

bool
FilterMissingRequired::show(const Option& opt)
{
  return (opt.required && (!opt.parsed || opt.parsedValue.empty()));
}

bool
FilterRequired::show(const Option& opt)
{
  return (opt.required && !opt.system);
}

bool
FilterOptional::show(const Option& opt)
{
  return (!opt.required && !opt.system);
}

FilterName::FilterName(const std::string& name)
{
  myName = name;
}

bool
FilterName::show(const Option& opt)
{
  return (opt.opt == myName);
}

FilterGroup::FilterGroup(const std::string& group)
{
  myGroup = group;
}

bool
FilterGroup::show(const Option& opt)
{
  const size_t s = opt.groups.size();

  // For blank, get all options _without_ any group
  if (myGroup == "") {
    if (s == 0) {
      return (true);
    }
  }

  // Hunt for group string in list
  for (auto& i: opt.groups) {
    if (i == myGroup) {
      return (true);
    }
  }
  return (false);
}
