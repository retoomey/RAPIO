#pragma once

#include <string>
#include <vector>
#include <map>

namespace rapio {
// Generic representation of a sub-option (e.g., "severe", "info")
struct HelpSubEntry {
  std::string opt;
  std::string description;
};

// Generic representation of a command-line flag/option
struct HelpEntry {
  std::string               opt;          // e.g., "verbose"
  std::string               usage;        // e.g., "--verbose (See help verbose)"
  std::string               defaultValue; // e.g., "info"
  std::string               exampleValue; // e.g., "/tmp" (if required)
  std::string               advancedHelp; // Detailed paragraph
  std::vector<std::string>  groups;       // e.g., ["LOGGING"]

  bool                      isRequired;
  bool                      isBoolean;
  bool                      isHidden;

  std::vector<HelpSubEntry> suboptions;
  size_t                    suboptionMaxLen; // Needed for aligning suboptions cleanly
};

// The standalone formatter class
class HelpFormatter {
public:
  HelpFormatter(const std::string& progName,
    const std::string            & description,
    const std::string            & authors,
    const std::string            & example,
    const std::string            & header);

  // Main entry point to print the help menu
  void
  printHelp(const std::vector<HelpEntry>& allOptions,
    bool                                showHidden   = false,
    bool                                advancedHelp = false) const;

private:
  void
  printHeader() const;
  void
  printMetadata() const;
  void
  printOptionGroup(const std::string& groupName,
    const std::vector<HelpEntry>    & groupOptions,
    size_t                          maxArgWidth,
    bool                            showHidden,
    bool                            advancedHelp) const;
  void
  printSingleOption(const HelpEntry& opt,
    size_t                         maxArgWidth,
    bool                           advancedHelp) const;
  void
  printSuboptions(const HelpEntry& opt,
    size_t                       indentWidth,
    bool                         advancedHelp) const;

  size_t
  calculateMaxArgWidth(const std::vector<HelpEntry>& options) const;

  std::string myProgName;
  std::string myDescription;
  std::string myAuthors;
  std::string myExample;
  std::string myHeader;
};
} // namespace rapio
