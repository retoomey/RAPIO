#pragma once

#include <rAlgorithm.h>

#include <string>
#include <vector>

namespace rapio {
// Collection of utility classes used by RAPIOOptions.
//

/** Suboption stores sets of valid values, such as the modes for '-verbose'.
 * @author Robert Toomey
 */
class Suboption : public Algorithm {
public:

  /** The option text */
  std::string opt;

  /** The description of this suboption */
  std::string description;
};

/** Class storing all information for a single parameter option such as '-r' for real time.
 * FIXME: 'Could' cleanup, do get/set for example here. This is mostly a simple storage
 * class for RAPIOOptions.
 * @see RAPIOOptions
 * @author Robert Toomey */
class Option : public Algorithm {
public:

  bool required;                     ///< Required option or not.  Will not run
                                     ///< if missing and required
  bool boolean;                      ///< Is this a boolean option true/false?
                                     ///<  "-r" for example
  bool system;                       ///< Is this a system argument for all
                                     ///< algorithms such as verbose?
  std::string opt;                   ///< The option text
  std::string name;                  ///< The full name of the option
  std::string usage;                 ///< The full help string for the option
  std::string example;               ///< Example for required
  std::string defaultValue;          ///< Default value given for optional string
  std::string parsedValue;           ///< Parsed value found for this option
  std::string advancedHelp;          ///< Lots of detail to show with the "help
                                     ///< option" ability
  bool parsed;                       ///< Was this found and parsed?
  std::vector<Suboption> suboptions; ///< Only allowed settings if size > 0
  bool enforceSuboptions;            ///< User MUST choose a registered suboption
  size_t suboptionmax;               ///< Max width of added suboptions (for
                                     ///< formatting)
  std::vector<std::string> groups;   ///< groups this option belongs too.

  /** Add a group for this option.  Used for grouping in parameter printout */
  void
  addGroup(const std::string& group);

  // Suboptions

  /** Add a suboption to this option. */
  void
  addSuboption(const std::string& opt, const std::string& description);

  /** Set enforced suboptions or not.  Enforced means the passed parameter must
   * match an added suboption, or blank which becomes the default suboption */
  void
  setEnforcedSuboptions(bool flag){ enforceSuboptions = flag; }

  /** Return true iff our found value matches one of our added suboptions.  This is
   * always true if suboptions are enforced */
  bool
  isInSuboptions();

  bool
  operator < (const Option& rhs) const;
};

/** Option filters. Used to filter big lists of options for printing or processing
 * Maybe we could lamba this or use a trick to simplify it at some point.
 */
class OptionFilter : public Algorithm {
public:

  virtual bool
  show(const Option& opt)
  {
    return (true);
  }
};

/** Get options back where a bad suboption was chosen */
class FilterBadSuboption :  public OptionFilter {
public:
  virtual bool
  show(const Option& opt) override;
};

/** Get options back where they were missing from command line */
class FilterMissingRequired :  public OptionFilter {
public:
  virtual bool
  show(const Option& opt) override;
};

/** Get list of required options. Required by used to enter on command line, such as -i */
class FilterRequired :  public OptionFilter {
public:
  virtual bool
  show(const Option& opt) override;
};

/** Filter options by given name.  Help uses this to try to find what you want advanced help for */
class FilterName :  public OptionFilter {
public:
  FilterName(const std::string& name);
  virtual bool
  show(const Option& opt) override;
protected:

  std::string myName;
};

/** Get list of options by group.  Used for printing out by categories */
class FilterGroup :  public OptionFilter {
public:
  FilterGroup(const std::string& name);
  virtual bool
  show(const Option& opt) override;
protected:

  std::string myGroup;
};
}
