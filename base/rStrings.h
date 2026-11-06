#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace rapio {
/** This is a helper class of functions that do commonly required stuff
 *  that is not in the Standard Library.
 *
 *  @ingroup rapio_utility
 *  @brief API for lots of string manipulation utilities
 */
class Strings {
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
  beginsWith(std::string_view str, std::string_view prefix);

  /** Does the string begin with prefix, and remove it */
  static bool
  removePrefix(std::string& str, std::string_view prefix);

  /** Does the string end with the given suffix? */
  static bool
  endsWith(std::string_view str, std::string_view suffix);

  /** Does the string end with suffix, and remove it */
  static bool
  removeSuffix(std::string& str, std::string_view suffix);

  /** Convert to upper case. */
  static void
  toUpper(std::string& s);

  /** Get a new upper case string  */
  static std::string
    makeUpper(std::string_view in);

  /** Convert to lower case. */
  static void
  toLower(std::string& s);

  /** Get a new lower case string  */
  static std::string
    makeLower(std::string_view in);

  /**
   * Split a string on whitespace and put the pieces into a vector.
   * @param in string to split
   * @param setme split strings are stored here
   * @return number of strings added to `setme'
   */
  static size_t
    split(std::string_view         in,
    std::vector<std::string> * setme);

  /** Split a string on a character and put the pieces into a vector.
   *
   *  @param in the string to split
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
    splitWithoutEnds(std::string_view in,
    char delimiter,
    std::vector<std::string> * setme);

  /** Split a string on a character and put the pieces into a vector.
   *
   *  @param in the string to split
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
    split(std::string_view         in,
    char delimiter,
    std::vector<std::string> * setme);

  /** Splits the string,s, on the first occurence of the
   *  string passed in.
   *
   *  @param in the string to split
   *  @param delimiter split token to search for in `splitme'
   *  @param setme result
   *  @return true on a match
   */
  static bool
    splitOnFirst(std::string_view in,
    std::string_view delimiter,
    std::vector<std::string> & setme);

  /** Replace all occurences of `from' with `to' in string `s'. */
  static void
    replace(std::string & in,
    std::string_view from,
    std::string_view to);

  /**  *bl?h.* matches Goopblah.jpg for example... */
  // static bool
  // matches(const std::string& pattern,
  //  const std::string      & test);

  /** Remove ANSII formatting from a string */
  static std::string
  removeANSII(std::string_view input);

  /** Remove non-number characters from a string, prepping for number conversion */
  static std::string
  removeNonNumber(std::string_view input);

  /** Split a string based on a width into words */
  static size_t
  wrap(std::string_view        inputin,
    std::vector<std::string> * setme,
    size_t                     restwidth,
    size_t                     firstwidth = 0);

  /** Simple * based pattern match */
  static bool
  matchPattern(std::string_view pattern,
    std::string_view            tocheck,
    std::string                 & star);

  /** Split an XML file that contains a \<signed\> signature
   * tag at the end of the file into message and signature parts.
   * Technically this is a string operation*/
  static bool
  splitSignedXMLFile(const std::string& signedxml,
    std::string                       & outmsg,
    std::string                       & outkey);

  /** Scan a single string for a give list of tokens.
   * @param pattern The string we are scanning for tokens.
   * @param tokens The list of tokens to match.
   * @param outputtokens A number list where each number references the token number, or -1 if a filler string (non token match)
   * @param outputfillers A list of non-matched strings represented by the ordered -1s in the outputtoken list */
  static void
  TokenScan(std::string_view pattern,
    std::vector<std::string> & tokens,
    std::vector<int>         & outputtokens,
    std::vector<std::string> & outputfillers);

  /** Remove up to a prefix off a source string */
  static std::string
  peel(std::string& s, std::string_view delimiter);

  /** Remove up to a prefix off a source string_view (zero-copy) */
  static std::string_view
  peel(std::string_view& s, std::string_view delimiter);

  /** Scan a single string for a give list of replacement strings
   * @param pattern The string we are scanning.
   * @param froms The list of from strings to match.
   * @param tos The list of strings to replace with.
   */
  static std::string
  replaceGroup(std::string_view    pattern,
    const std::vector<std::string> & froms,
    std::vector<std::string>       & tos);

  /** (AI) Format a given number of bytes into human readable size.
   * Useful for logging/debugging memory usage of things. */
  static std::string
  formatBytes(long long bytes, bool plusSign = false);

  /** Used for param lines --param=key,stuff to split the
   * string key,stuff into pieces */
  static void
  splitKeyParam(std::string_view commandline, std::string& key, std::string& params);
};

/** A class for DFA parsing a word/token by character */
class DFAWord
{
public:

  /** Create a word to parse */
  DFAWord(std::string_view t) : at(0), text(t)
  { }

  /** Parse another character */
  bool
  parse(const char& c)
  {
    if (text[at] == c) {
      at++;
      if (at >= text.size()) {
        at = 0;
        return true; // parse hit
      }
    } else {
      // Reset check first character again
      at = 0;
      if (text[at] == c) {
        at++;
      }
    }
    return false;
  }

protected:

  size_t at;
  std::string text;
};
}
