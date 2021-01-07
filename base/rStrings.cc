#include "rStrings.h"

// #include <algorithm>

#include <rError.h>

#include <boost/algorithm/string.hpp>

using namespace rapio;
using namespace std;

bool
Strings::beginsWith(const std::string& str, const std::string& prefix)
{
  return (boost::algorithm::starts_with(str, prefix));
}

bool
Strings::removePrefix(std::string& str, const std::string& prefix)
{
  std::string original = str;
  str = boost::algorithm::replace_first_copy(str, prefix, "");
  return (original != str);
}

bool
Strings::endsWith(const std::string& str, const std::string& suffix)
{
  return (boost::algorithm::ends_with(str, suffix));
}

bool
Strings::removeSuffix(std::string& str, const std::string& suffix)
{
  std::string original = str;
  str = boost::algorithm::replace_last_copy(str, suffix, "");
  return (original != str);
}

void
Strings::toUpper(std::string& s)
{
  boost::algorithm::to_upper(s);
}

std::string
Strings::makeUpper(const std::string& in)
{
  return boost::algorithm::to_upper_copy(in);
}

void
Strings::toLower(std::string& s)
{
  boost::algorithm::to_lower(s);
}

std::string
Strings::makeLower(const std::string& in)
{
  return boost::algorithm::to_lower_copy(in);
}

size_t
Strings::split(const std::string& in, std::vector<std::string> * setme)
{
  // Split greedy on space here
  std::string s3 = boost::algorithm::trim_copy(in);
  boost::split(*setme, s3, boost::is_any_of(" "), boost::token_compress_on);
  return setme->size();
}

size_t
Strings::splitWithoutEnds(const std::string & in,
  char                                      delimiter,
  std::vector<std::string> *                setme)
{
  auto test = [&delimiter](char c){
      return (c == delimiter);
    };

  std::string s3 = boost::algorithm::trim_copy_if(in, test);
  boost::split(*setme, s3, boost::is_any_of(std::string(1, delimiter)), boost::token_compress_on);
  return setme->size();
}

size_t
Strings::split(const std::string & in,
  char                           delimiter,
  std::vector<std::string> *     setme)
{
  if (in.empty()) { return (0); } // Not sure this is needed
  boost::split(*setme, in, boost::is_any_of(std::string(1, delimiter)), boost::token_compress_on);
  return (setme->size());
}

size_t
Strings::splitOnFirst(const std::string & in,
  char                                  delimiter,
  std::vector<std::string> *            setme)
{
  size_t n_tokens = 0;
  const std::string::size_type pos(in.find(delimiter));

  if (pos != std::string::npos) {
    setme->push_back(in.substr(0, pos));
    setme->push_back(in.substr(pos + 1, in.size() - (pos + 1)));
    n_tokens = 2;
  }

  return (n_tokens);
}

void
Strings::replace(std::string & in,
  const std::string          & from,
  const std::string          & to)
{
  boost::replace_all(in, from, to);
}

void
Strings::trim_left(std::string& s)
{
  boost::algorithm::trim_left(s);
}

void
Strings::trim_right(std::string& s)
{
  boost::algorithm::trim_right(s);
}

void
Strings::trim(std::string& s)
{
  boost::algorithm::trim(s);
}

/*
 * bool
 * Strings::matches(const std::string& pattern, const std::string& tocheck)
 * {
 * // FIXME: We can just use boost.regex here probably...
 *
 * // *bl?h.* matches Goopblah.jpg for example...
 * const char * wild = pattern.c_str();
 * const char * s    = tocheck.c_str();
 *
 * while ((*s) && (*wild != '*')) {
 *  if ((tolower(*wild) != tolower(*s)) && (*wild != '?')) {
 *    return (false);
 *  }
 *  wild++;
 *  s++;
 * }
 *
 * const char * cp = 0, * mp = 0;
 *
 * while (*s) {
 *  if (*wild == '*') {
 *    if (!*++wild) { return (true); } mp = wild;
 *    cp = s + 1;
 *  } else if ((tolower(*wild) == tolower(*s)) || (*wild == '?')) {
 *    wild++;
 *    s++;
 *  } else {
 *    wild = mp;
 *    s    = cp++;
 *  }
 * }
 *
 * while (*wild == '*') {
 *  wild++;
 * }
 * return (!*wild != 0);
 * } // Strings::matches
 */

std::string
Strings::removeANSII(const std::string& input)
{
  size_t curpos = 0;
  size_t state  = 0;

  std::string nbuffer;

  while (curpos < input.length()) {
    char at = input.at(curpos);

    switch (at) {
        // Recognize ANSII sequence...
        case '\033':

          if (state == 0) {
            state = 1; // Escape character
          }
          break;

        case '[':

          if (state == 1) {
            state = 2; // Second char
          } else { nbuffer += at; } break;

        case 'm':

          if (state == 2) {
            state = 0; // End of ANSII sequence
          } else { nbuffer += at; } break;

        default:

          if (state == 2) {
            // These are the ANSII characters...
            // They don't contribute to our 'length'
          } else {
            nbuffer += at;
          }
          break;
    }
    curpos++;
  }
  return (nbuffer);
} // Strings::removeANSII

/** Utility to word wrap a given string into a list where each string is less
 * than width.
 * The StartupOptions uses this to format command line option output */
size_t
Strings::wrap(const std::string & inputin,
  std::vector<std::string> *    setme,
  size_t                        restwidth,
  size_t                        firstwidth)
{
  size_t curpos = 0;
  size_t count  = 0;

  // Lol, since I made ANSII colors, I have to take these into account when
  // wrapping.  The ANSII text has logical
  // length, but not visual length. We make a little parser.  And who says
  // compiler theory wasn't gonna be useful?  eh? eh?
  // We recognize "\033[.....m" as zero length in the string.
  size_t state = 0;

  std::string nbuffer;
  std::string buffer;

  // Visible width, doesn't include ANSII characters...
  size_t currentWidth = 0;

  // Real width is logical width..
  size_t realWidth  = 0;
  size_t width      = firstwidth;
  size_t space      = 0;
  size_t spacepos   = 0;
  bool haveSpace    = false;
  size_t start      = 0;
  std::string input = inputin;

  while (curpos < input.length()) {
    char at = input.at(curpos);

    switch (at) {
        // Recognize ANSII sequence...
        case '\033':

          if (state == 0) {
            state = 1; // Escape character
          }
          break;

        case '[':

          if (state == 1) {
            state = 2; // Second char
          } else { currentWidth++; nbuffer += at; }
          break;

        case 'm':

          if (state == 2) {
            state = 0; // End of ANSII sequence
          } else { currentWidth++; nbuffer += at; }
          break;

        case ' ':

          if (state == 0) { // Any space not in ANSII sequence is a possible break
                            // point
            space    = realWidth;
            spacepos = curpos;
            currentWidth++;
            nbuffer  += at;
            haveSpace = true;
          }
          break;

        default:

          if (state == 2) {
            // These are the ANSII characters...
            // They don't contribute to our 'length'
          } else {
            currentWidth++;
            nbuffer += at;
          }
          break;
    }
    realWidth++;
    buffer += at;

    // Check current length. This is length without ANSII characters..
    if ((state == 0) && (currentWidth == width)) { // humm should always be
                                                   // state 0 since we only
                                                   // increase width then..
      // haveSpace = false;
      if (haveSpace) {
        setme->push_back(input.substr(start, space));
        start  = spacepos + 1;
        curpos = spacepos; // Move 'back' to space...
      } else {             // No space, break the string where it's at.
        setme->push_back(input.substr(start, realWidth));
        start = curpos + 1;
      }
      count++;
      buffer       = "";
      nbuffer      = "";
      currentWidth = 0;
      realWidth    = 0;
      haveSpace    = false;
      width        = restwidth;
    }
    curpos++;
  }

  if (buffer.length() > 0) {
    setme->push_back(buffer);
    count++;
  }

  // Finally, snag the short end piece..
  // if (curpos != input.length()){
  //    setme->push_back(input.substr(curpos, input.npos));
  //    count++;
  // }

  return (count);
} // Strings::wrap

bool
Strings::matchPattern(const std::string& pattern,
  const std::string                    & tocheck,
  std::string                          & star)
{
  // "*"    "Velocity" YES "Velocity"
  // "Vel*" "Velocity" YES "ocity"
  // "Vel*" "Ref"      NO  ""
  // "Vel*" "Vel"      YES ""
  // ""     "Velocity" NO  ""
  // "Velocity" "VelocityOther" NO ""
  //
  bool match      = true;
  bool starFound  = false;
  const size_t pm = pattern.size();
  const size_t cm = tocheck.size();

  star = "";

  for (size_t i = 0; i < pm; i++) {
    if (pattern[i] == '*') { // Star found...
      starFound = true;
      match     = true;

      if (cm > i) { star = tocheck.substr(i); } break;
    } else if (pattern[i] != tocheck[i]) { // character mismatch
      match = false;
      break;
    }
  }

  // Ok if no star found should be _exact_ match
  // ie "Velocity" shouldn't match "VelocityOther" or "VelocityOther2"
  // but was good for "Velocity*" to match...
  if (!starFound && (cm > pm)) { // We checked up to length already
    match = false;
  }

  return (match);
} // Strings::matchPattern
