#pragma once

#include <rUtility.h>

#include <string>
#include <vector>

namespace rapio {
/** This is a helper class of functions that do commonly required stuff
 *  that is not in the Standard Library.
 *
 */
class Strings : public Utility {
public:

  /** Modify a string to remove left white space */
  static void
  trim_left(std::string& s);

  /** Modify a string to remove right white space */
  static void
  trim_right(std::string& s);

  /** Modify a string to remove left and right white space */
  static void
  trim(std::string& s);

  /** Does the string begin with the given prefix? */
  static bool
  beginsWith(const std::string& str, const std::string& prefix);

  /** Does the string end with the given suffix? */
  static bool
  endsWith(const std::string& str, const std::string& suffix);

  /** Convert to upper case. */
  static void
  toUpper(std::string& s);

  /** Get a new upper case string  */
  static std::string
  makeUpper(const std::string& s);

  /** Convert to lower case. */
  static void
  toLower(std::string& s);

  /** Get a new lower case string  */
  static std::string
  makeLower(const std::string& s);

  /**
   * Split a string on whitespace and put the pieces into a vector.
   * @param splitme string to split
   * @param setme split strings are stored here
   * @return number of strings added to `setme'
   */
  static size_t
  split(const std::string      & splitme,
    std::vector<std::string> * setme);

  /** Split a string on a character and put the pieces into a vector.
   *
   *  @param splitme the string to split
   *  @param delimiter split token to search for in `splitme'
   *  @param setme result
   *
   *  @return number of fields
   *  Note that splitting a string /hello/ based on / will
   *  give you just one string. To get empty strings for the
   *  the location of the beginning and ending slashes,
   *  @see split
   */
  static size_t
  splitWithoutEnds(const std::string & splitme,
    char                             delimiter,
    std::vector<std::string> *       setme);

  /** Split a string on a character and put the pieces into a vector.
   *
   *  @param splitme the string to split
   *  @param delimiter split token to search for in `splitme'
   *  @param setme result
   *
   *  @return number of fields
   *  Note that splitting a string /hello/ based on / will
   *  give you three string with an empty string representing
   *  the location of the beginning and ending slashes.
   *  @see splitWithoutEnds to avoid getting these empty strings
   */
  static size_t
  split(const std::string      & splitme,
    char                       delimiter,
    std::vector<std::string> * setme);

  /** Splits the string,s, on the first occurence of the
   *  character,sep, passsed in.
   *
   *  The pieces are placed
   *  into the vector passed in. Returns 2 if sep is in the string
   *  and zero if sep is not in the string.
   *
   *  Thus, the split produce a maximum of two pieces. In the
   *  absence of the sep character, no pieces are added to the vector,
   *  and zero is returned.
   *
   *  @param splitme the string to split
   *  @param delimiter split token to search for in `splitme'
   *  @param setme result
   *  @return number of fields
   */
  static size_t
  splitOnFirst(const std::string & splitme,
    char                         delimiter,
    std::vector<std::string> *   setme);

  /** Replace all occurences of `from' with `to' in string `s'. */
  static void
  replace(std::string & s,
    const std::string & from,
    const std::string & to);

  /**  *bl?h.* matches Goopblah.jpg for example... */
  static bool
  matches(const std::string& pattern,
    const std::string      & test);

  /** Remove ANSII formatting from a string */
  static std::string
  removeANSII(const std::string& input);

  /** Split a string based on a width into words */
  static size_t
  wrap(const std::string       & input,
    std::vector<std::string> * setme,
    size_t                     width,
    size_t                     firstwidth = 0);
};
}
