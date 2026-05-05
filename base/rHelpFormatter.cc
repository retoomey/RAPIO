#include "rHelpFormatter.h"
#include "rColorTerm.h"
#include "rStrings.h"
#include "rOS.h"

#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace rapio;
using namespace std;

HelpFormatter::HelpFormatter(const std::string& progName,
  const std::string                           & description,
  const std::string                           & authors,
  const std::string                           & example,
  const std::string                           & header)
  : myProgName(progName), myDescription(description),
  myAuthors(authors), myExample(example), myHeader(header)
{ }

void
HelpFormatter::printHelp(const std::vector<HelpEntry>& allOptions,
  bool                                               showHidden,
  bool                                               advancedHelp) const
{
  // RESTORED: Sort options case-insensitively just like Option::operator<
  std::vector<HelpEntry> sortedOptions = allOptions;

  std::sort(sortedOptions.begin(), sortedOptions.end(),
    [](const HelpEntry& a, const HelpEntry& b) {
    std::string lowerA = Strings::makeLower(a.opt);
    std::string lowerB = Strings::makeLower(b.opt);
    if (lowerA == lowerB) { return a.opt > b.opt; }
    return lowerA < lowerB;
  });

  printHeader();

  // If this is a generic help dump (not targeted advanced help), print the metadata
  if (!advancedHelp) {
    printMetadata();

    // Print the required command line example
    size_t d = 5;
    std::cout << ColorTerm::bold("EXAMPLE:\n");
    if (!myExample.empty()) {
      std::cout << std::setw(d) << std::left << "";
      std::cout << myProgName << " " << myExample << "\n";
    }

    std::cout << std::setw(d) << std::left << "";
    std::cout << myProgName << " ";
    std::string outputRequired;
    size_t commandLength = d + myProgName.length() + 1;

    for (const auto& opt : allOptions) {
      if (opt.isRequired && !opt.isHidden) {
        outputRequired += ("-" + ColorTerm::bold(opt.opt) + " " + ColorTerm::underline(opt.exampleValue) + " ");
      }
    }
    outputRequired += ColorTerm::bold("--verbose");
    ColorTerm::wrapWithIndent(commandLength, commandLength, outputRequired);
  }

  size_t maxArgWidth = calculateMaxArgWidth(allOptions);

  // Layout groups (mimicking the original OptionList layout logic)
  std::vector<std::string> layout = { "I/O", "", "TIME", "LOGGING", "CONFIG", "HELP" };
  std::vector<std::string> userGroups;

  for (const auto& o : allOptions) {
    for (const auto& g : o.groups) {
      if ((std::find(layout.begin(), layout.end(), g) == layout.end()) &&
        (std::find(userGroups.begin(), userGroups.end(), g) == userGroups.end()))
      {
        userGroups.push_back(g);
      }
    }
  }
  std::sort(userGroups.begin(), userGroups.end());
  layout.insert(layout.begin() + 2, userGroups.begin(), userGroups.end());

  for (const auto& group : layout) {
    printOptionGroup(group, allOptions, maxArgWidth, showHidden, advancedHelp);
  }

  if (!showHidden && !advancedHelp) {
    std::cout << ColorTerm::bold("There are hidden options not shown (see help hidden)\n");
  }
} // HelpFormatter::printHelp

void
HelpFormatter::printHeader() const
{
  if (!myHeader.empty()) {
    ColorTerm::wrapWithIndent(0, 0, myHeader);
  }
  // Let the program handle compiler versions via RAPIOOptions if needed,
  // or pass it in as part of the header string.
}

void
HelpFormatter::printMetadata() const
{
  size_t d = 5;

  if (!myDescription.empty()) {
    std::cout << ColorTerm::bold("DESCRIPTION:\n");
    std::cout << std::setw(d) << std::left << "";
    ColorTerm::wrapWithIndent(d, 0, myDescription);
  }
  if (!myAuthors.empty()) {
    std::cout << ColorTerm::bold("AUTHOR(S):\n");
    std::cout << std::setw(d) << std::left << "";
    ColorTerm::wrapWithIndent(d, 0, myAuthors);
  }

  std::cout << ColorTerm::bold("BUILD INFO:\n");
  std::cout << std::setw(d) << std::left << "";
  ColorTerm::wrapWithIndent(d, 0, "(" + OS::getBuildInfo() + ")");
}

void
HelpFormatter::printOptionGroup(const std::string& groupName,
  const std::vector<HelpEntry>                   & groupOptions,
  size_t                                         maxArgWidth,
  bool                                           showHidden,
  bool                                           advancedHelp) const
{
  bool hasPrintableOptions = false;

  for (const auto& opt : groupOptions) {
    if (opt.isHidden && !showHidden) { continue; }

    // Check if it belongs to this group (empty string means default group)
    bool inGroup = false;
    if (groupName.empty()) {
      inGroup = opt.groups.empty();
    } else {
      inGroup = (std::find(opt.groups.begin(), opt.groups.end(), groupName) != opt.groups.end());
    }

    if (inGroup) {
      if (!hasPrintableOptions) {
        std::cout << ColorTerm::bold(groupName.empty() ? "OPTIONS:\n" : groupName + ":\n");
        hasPrintableOptions = true;
      }
      printSingleOption(opt, maxArgWidth, advancedHelp);
    }
  }
}

void
HelpFormatter::printSingleOption(const HelpEntry& opt, size_t maxArgWidth, bool advancedHelp) const
{
  size_t indentWidth    = maxArgWidth + 4;
  std::string optString = opt.isRequired ? (" " + opt.opt) : (" [" + opt.opt + "]");

  std::cout << ColorTerm::red() << ColorTerm::bold()
            << std::setw(indentWidth) << std::left << optString << ColorTerm::reset();

  std::string status = opt.isRequired ? ("(example=" + opt.exampleValue + ")") :
    ("(default=" + opt.defaultValue + ")");

  status = ColorTerm::blue() + status + ColorTerm::reset();
  ColorTerm::wrapWithIndent(indentWidth, indentWidth, status);

  if (!opt.usage.empty()) {
    std::cout << std::setw(indentWidth + 2) << std::left << "";
    ColorTerm::wrapWithIndent(indentWidth + 2, indentWidth + 4, opt.usage);
  }

  const bool advancedWillShow = (advancedHelp && !opt.advancedHelp.empty());

  if (!opt.suboptions.empty() && !advancedWillShow) {
    printSuboptions(opt, indentWidth, false);
  }

  if (advancedWillShow) {
    int d = 5;
    std::cout << ColorTerm::bold("DETAILED HELP:\n");
    std::cout << std::setw(d) << std::left << "";

    // RESTORED: Split the string by newline so ColorTerm can wrap each line individually
    std::vector<std::string> lines;
    Strings::splitWithoutEnds(opt.advancedHelp, '\n', &lines);

    for (const auto& l : lines) {
      ColorTerm::wrapWithIndent(d, 0, l);
    }
    std::cout << "\n";
    printSuboptions(opt, indentWidth, true);
  }
} // HelpFormatter::printSingleOption

void
HelpFormatter::printSuboptions(const HelpEntry& opt, size_t indentWidth, bool advancedHelp) const
{
  for (const auto& sub : opt.suboptions) {
    std::string optpad = sub.opt.empty() ? "\"\"" : sub.opt;

    size_t max = opt.suboptionMaxLen;
    while (optpad.length() < max) {
      optpad += " ";
    }

    if (!advancedHelp) {
      std::cout << std::setw(indentWidth + 4) << std::left << "";
      std::string out = ColorTerm::blue() + ColorTerm::bold(optpad) + " --" + sub.description;
      ColorTerm::wrapWithIndent(indentWidth + 4, indentWidth + 4 + max + 3, out);
    } else {
      std::cout << " " << ColorTerm::red() << optpad << ColorTerm::reset() << " : " + sub.description << "\n";
    }
  }
}

size_t
HelpFormatter::calculateMaxArgWidth(const std::vector<HelpEntry>& options) const
{
  size_t maxW = 5;

  for (const auto& opt : options) {
    if (opt.opt.length() > maxW) {
      maxW = opt.opt.length();
    }
  }
  return maxW;
}
