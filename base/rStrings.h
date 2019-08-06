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
private:

  static const bool myWhitespaceTable[];

public:

  /** Every character of string is a number part */
  static bool
  isNumeric(const std::string& s);

  /** Do two strings start with same part? */
  static bool
  samePrefix(const std::string&,
    const std::string&,
    char stopWhenReached);

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

  /** Compares two strings in a case-insensitive way. */
  static bool
  similar(const std::string&,
    const std::string&);

  /** Capitalize every word. */
  static void
  capitalize(std::string& s);

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

  /**
   * Splits a text buf into a vector of lines.
   *
   * "a\nb\nc"   -> "a", "b", "c"
   * "a\nb\n\nc"   -> "a", "b", "", "c"
   * "a\nb\n\nc\n" -> "a", "b", "", "c"
   */
  static size_t
  splitToLines(const std::string & s,
    std::vector<std::string>     & setme);

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

  /** Removes leading and trailing white space. */
  static std::string
  trimText(const std::string& s);

  /** In a string, replace one character with another.
   *  One useful application is
   *
   *  <pre>
   *  str = replaceCharacter( str, ' ', '_' );
   *  </pre>
   *
   *  which replaces white spaces with underscores.  Note that this
   *  does nothing to tabs and new line characters.
   *
   *  @param str       string on which to operate
   *  @param old_char  character to replace
   *  @param new_char  replacement character
   *
   *  @see replaceWhiteSpace
   */
  static std::string
  replaceCharacter(const std::string& str,
    char                            old_char,
    char                            new_char);

  /** Replace every white space character with another character.
   *
   *  @param str       string on which to operate
   *  @param new_char  replacement character
   */
  static std::string
  replaceWhiteSpace(const std::string& str,
    char                             new_char);

  /** Returns the string with any of these substrings removed. */
  static std::string
  removeSubstrings(const std::string & original,
    const std::vector<std::string>   & discards);

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

  inline static bool
  isWhiteSpace(char c)
  {
    return (myWhitespaceTable[(int) c]);
  }

  /**
   * STL unary predicate.
   * Can be used in countWords() and findNextWord()
   * to find whitespace-delimited words.
   */
  class WhiteSpacePred : public Utility {
public:
    WhiteSpacePred(){ }

    bool
    operator () (const char ch) const
    {
      return (Strings::isWhiteSpace(ch));
    }
  };

  /**
   * STL unary predicate.
   * Can be used in countWords() and findNextWord()
   * to find token-delimited words.
   */
  class DelimiterPred : public Utility {
public:
    char myDelimiter;
    DelimiterPred(char ch) : myDelimiter(ch){ }

    bool
    operator () (const char ch) const
    {
      return (myDelimiter == ch);
    }
  };

  /**
   * Counts the number of words in a string.
   * The predicate `pred' is used to find word breaks.
   */
  template <typename Pred>
  static size_t
  countWords(const std::string & s,
    const Pred                 & pred,
    std::string::size_type     pos = 0)
  {
    bool was(true);
    const size_t len(s.size());
    size_t n_tokens(0);

    while (pos < len) {
      const bool is(pred(s[pos++]));

      if (was && !is) { ++n_tokens; }
      was = is;
    }
    return (n_tokens);
  }

  /**
   * Convenience form of countWords()
   * which treats whitespace as the delimiter between words.
   */
  static size_t
  countWords(const std::string& s, std::string::size_type pos = 0)
  {
    static const WhiteSpacePred ws;

    return (countWords(s, ws, pos));
  }

  /**
   * Finds the next word in a string.
   * The predicate `pred' is used to find word breaks.
   */
  template <typename Pred>
  static bool
  findNextWord(const std::string & s,
    std::string::size_type       & setme_pos,
    std::string::size_type       & setme_len,
    const Pred                   & pred,
    std::string::size_type       pos_in = 0)
  {
    if ((pos_in == s.npos) || (pos_in >= s.size())) { return (false); }

    const char * begin(&s[pos_in]);

    while (*begin && pred(*begin)) { ++begin; }
    const char * end(begin);

    while (*end && !pred(*end)) { ++end; }
    setme_pos = begin - &s[0];
    setme_len = end - begin;
    return (setme_len != 0);
  }

  /**
   * Convenience form of findNextWord()
   * which treats whitespace as the delimiter between words.
   */
  static bool
  findNextWord(const std::string & s,
    std::string::size_type       & setme_pos,
    std::string::size_type       & setme_len,
    std::string::size_type       pos_in = 0)
  {
    static const WhiteSpacePred ws;

    return (findNextWord(s, setme_pos, setme_len, ws, pos_in));
  }

  /** Get path up to a found substr...
   * Example /test/test2/test3/test4 with /test3/ gives us /test/test2/test3/
   */
  static std::string
  getPathUpTo(const std::string& str, const std::string& to);
};
}
