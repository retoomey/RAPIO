#include "rStrings.h"

#include <rError.h>
#include <rBOOST.h>

BOOST_WRAP_PUSH
#include <boost/algorithm/string.hpp>
BOOST_WRAP_POP

#include <fstream>
#include <iomanip>

using namespace rapio;
using namespace std;

bool
Strings::beginsWith(std::string_view str, std::string_view prefix)
{
  return (str.size() >= prefix.size()) &&
         (str.compare(0, prefix.size(), prefix) == 0);
}

bool
Strings::removePrefix(std::string& str, std::string_view prefix)
{
  if (!prefix.empty() && beginsWith(str, prefix)) {
    str.erase(0, prefix.size());
    return true;
  }
  return false;
}

bool
Strings::endsWith(std::string_view str, std::string_view suffix)
{
  return (str.size() >= suffix.size()) &&
         (str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0);
}

bool
Strings::removeSuffix(std::string& str, std::string_view suffix)
{
  if (!suffix.empty() && endsWith(str, suffix)) {
    str.erase(str.size() - suffix.size());
    return true;
  }
  return false;
}

void
Strings::toUpper(std::string& s)
{
  boost::algorithm::to_upper(s);
}

std::string
Strings::makeUpper(std::string_view in)
{
  std::string s(in);

  boost::algorithm::to_upper(s);
  return s;
}

void
Strings::toLower(std::string& s)
{
  boost::algorithm::to_lower(s);
}

std::string
Strings::makeLower(std::string_view in)
{
  std::string s(in);

  boost::algorithm::to_lower(s);
  return s;
}

size_t
Strings::split(std::string_view in, std::vector<std::string> * setme)
{
  // Split greedy on space here
  while (!in.empty() && std::isspace(static_cast<unsigned char>(in.front()))) {
    in.remove_prefix(1);
  }
  while (!in.empty() && std::isspace(static_cast<unsigned char>(in.back()))) {
    in.remove_suffix(1);
  }

  boost::split(*setme, in, boost::is_any_of(" "), boost::token_compress_on);
  return setme->size();
}

size_t
Strings::splitWithoutEnds(std::string_view in,
  char                                     delimiter,
  std::vector<std::string> *               setme)
{
  while (!in.empty() && (in.front() == delimiter)) {
    in.remove_prefix(1);
  }
  while (!in.empty() && (in.back() == delimiter)) {
    in.remove_suffix(1);
  }

  const std::string_view delimView(&delimiter, 1);

  boost::split(*setme, in, boost::is_any_of(delimView), boost::token_compress_on);
  return setme->size();
}

size_t
Strings::split(std::string_view in,
  char                          delimiter,
  std::vector<std::string> *    setme)
{
  if (in.empty()) { return (0); } // Not sure this is needed
  const std::string_view delimView(&delimiter, 1);

  boost::split(*setme, in, boost::is_any_of(delimView), boost::token_compress_on);
  return (setme->size());
}

bool
Strings::splitOnFirst(std::string_view in, std::string_view delimiter, std::vector<std::string>& setme)
{
  size_t pos = in.find(delimiter);

  if (pos != std::string_view::npos) {
    std::string prefix(in.substr(0, pos));
    std::string suffix(in.substr(pos + delimiter.size()));
    setme.clear();
    setme.push_back(std::move(prefix));
    setme.push_back(std::move(suffix));
    return true;
  }

  std::string full(in);

  setme.clear();
  setme.push_back(std::move(full)); // No delimiter found, return full input
  return false;
}

void
Strings::replace(std::string & in,
  std::string_view           from,
  std::string_view           to)
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
Strings::removeANSII(std::string_view input)
{
  size_t curpos = 0;
  size_t state  = 0;

  std::string nbuffer;

  while (curpos < input.length()) {
    char at = input[curpos];

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

std::string
Strings::removeNonNumber(std::string_view input)
{
  std::string output;
  bool decimalPointSeen = false;
  bool eSeen       = false;
  bool signAllowed = true;

  for (char c : input) {
    if (isdigit(c)) {
      output     += c;
      signAllowed = false;
    } else if (((c == '+') || (c == '-') ) && signAllowed) {
      output     += c;
      signAllowed = false;
    } else if ((c == '.') && !decimalPointSeen && !eSeen) {
      output += c;
      decimalPointSeen = true;
      signAllowed      = false;
    } else if (((c == 'e') || (c == 'E') ) && !eSeen) {
      output     += c;
      eSeen       = true;
      signAllowed = true;
    }
  }

  return output;
}

/** Utility to word wrap a given string into a list where each string is less
 * than width.
 * The StartupOptions uses this to format command line option output */
size_t
Strings::wrap(std::string_view input,
  std::vector<std::string> *   setme,
  size_t                       restwidth,
  size_t                       firstwidth)
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
  size_t realWidth = 0;
  size_t width     = firstwidth;
  size_t space     = 0;
  size_t spacepos  = 0;
  bool haveSpace   = false;
  size_t start     = 0;

  while (curpos < input.length()) {
    char at = input[curpos];

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
        setme->emplace_back(input.substr(start, space));
        start  = spacepos + 1;
        curpos = spacepos; // Move 'back' to space...
      } else {             // No space, break the string where it's at.
        setme->emplace_back(input.substr(start, realWidth));
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
Strings::matchPattern(std::string_view pattern,
  std::string_view                     tocheck,
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

      if (cm > i) { star.assign(tocheck.substr(i)); } break;
    } else if ((i >= cm) || (pattern[i] != tocheck[i])) { // character mismatch
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

bool
Strings::splitSignedXMLFile(const std::string& signedxml,
  std::string                                & outmsg,
  std::string                                & outkey)
{
  // Get full .sxml file
  std::ifstream xmlfile;

  xmlfile.open(signedxml.c_str());
  std::ostringstream str;

  str << xmlfile.rdbuf();
  std::string msg = str.str();

  xmlfile.close();

  // The last three lines should be of form:
  // <signed>
  // key
  // </signed>
  // where key may not be xml compliant.  We remove it before
  // passing to xml parsing.
  std::size_t found = msg.rfind("<signed>");

  if (found == std::string::npos) {
    fLogSevere("Missing signed tag end of file");
    return false;
  }

  // Split on the msg/signature
  std::string onlymsg    = msg.substr(0, found);
  std::string keywrapped = msg.substr(found);

  // Remove the signed tags from the key
  found = keywrapped.find("<signed>\n");
  if (found == std::string::npos) {
    fLogSevere("Missing start <signed> tag end of file");
    return false;
  }
  keywrapped.replace(found, 9, "");

  found = keywrapped.find("</signed>\n");
  if (found == std::string::npos) {
    fLogSevere("Missing end </signed> tag end of file");
    return false;
  }
  keywrapped.replace(found, 10, "");

  outmsg = onlymsg;
  outkey = keywrapped;
  return true;
} // Strings::splitSignedXMLFile

void
Strings::TokenScan(
  std::string_view        pattern,
  std::vector<std::string>& tokens,
  std::vector<int>        & outputtokens,
  std::vector<std::string>& outputfillers
)
{
  // Simple multi-DFA token match replacer.  For each token we store a number
  // representing a match.  We want this in a way for the logger
  // to use quickly with min delay O(n).  Token -1 means the next filler
  // string (between tokens), while numbers refer to the pattern list
  // I always love making these because teachers told me compiler theory wasn't
  // a useful class...lol

  // DFA count array for each token
  std::vector<size_t> ats;

  ats.resize(tokens.size());

  // For each character in pattern...
  std::string fill;

  for (auto s:pattern) {
    // For each token in list...
    fill += s;
    for (size_t i = 0; i < tokens.size(); i++) {
      auto& token = tokens[i];
      auto& at    = ats[i];
      // If character matches...progress token DFA forward...
      if (token[at] == s) {
        // Matched a token, remove from gathered string, this
        // will be the unmatched part filler before the token
        if (++at >= token.size()) {
          // Add one filler string to log
          Strings::removeSuffix(fill, token);
          if (!fill.empty()) {
            outputfillers.push_back(fill);
            outputtokens.push_back(-1);
            fill = "";
          }
          // Add one matched token to log and reset all DFAS
          outputtokens.push_back(i);
          for (auto& a:ats) {
            a = 0;
          }
          break; // important
        }
        // Otherwise that single token resets
      } else {
        at = 0; // token failed match, reset
      }
    }
  }
  // Write unmatched to the end
  if (!fill.empty()) {
    outputfillers.push_back(fill);
    outputtokens.push_back(-1);
  }

  // Verification/debugging if the code ever fails
  #if 0
  std::cout << ">>>\"";
  size_t fat = 0;
  for (auto a:outputtokens) {
    // std::cout << a << ":";
    if (a == -1) { // handle filler
      std::cout << outputfillers[fat++];
    } else { // handle token
      std::cout << "{" << tokens[a] << "}";
    }
  }
  std::cout << "\"<<<\n";
  std::cout << "filler size " << outputfillers.size() << " and token list " << outputtokens.size() << "\n";
  #endif // if 0
} // Strings::TokenScan

std::string
Strings::peel(std::string& s, std::string_view delimiter)
{
  // Edge case: if delimiter is empty, return the whole string and clear source
  if (delimiter.empty()) {
    std::string token = std::move(s);
    s.clear();
    return token;
  }

  size_t p = s.find(delimiter);

  if (p == std::string::npos) {
    // Delimiter not found: return the whole string and clear source
    std::string token = std::move(s);
    s.clear();
    return token;
  }

  // Delimiter found: extract token
  std::string token = s.substr(0, p);

  // Safety check: ensure we don't erase out of bounds (though find() guarantees this)
  // We use delimiter.length() instead of the dangerous C-style strlen()
  s.erase(0, p + delimiter.length());

  return token;
}

std::string_view
Strings::peel(std::string_view& s, std::string_view delimiter)
{
  if (delimiter.empty()) {
    std::string_view token = s;
    s = std::string_view();
    return token;
  }

  size_t p = s.find(delimiter);

  if (p == std::string_view::npos) {
    std::string_view token = s;
    s = std::string_view();
    return token;
  }

  std::string_view token = s.substr(0, p);

  s.remove_prefix(p + delimiter.length());

  return token;
}

std::string
Strings::replaceGroup(
  std::string_view              pattern,
  const std::vector<std::string>& froms,
  std::vector<std::string>      & tos
)
{
  // Toomey: Simple multi-DFA string replacer.  I made this to
  // replace X substrs in a string with a single N pass/copy.
  // For 'dynamic' replacement, use the TokenScan instead which
  // gives you a token/filler list to iterate over.

  std::string output; // stream 'might' be faster

  // DFA count array for each from
  std::vector<size_t> ats;

  ats.resize(froms.size());

  // For each character in pattern...
  std::string fill;

  for (auto s:pattern) {
    // For each token in list...
    fill += s;
    for (size_t i = 0; i < froms.size(); i++) {
      auto& from = froms[i];
      auto& at   = ats[i];
      // If character matches...progress token DFA forward...
      if (from[at] == s) {
        // Matched a token, remove from gathered string, this
        // will be the unmatched part filler before the token
        if (++at >= from.size()) {
          // Add one filler string to log
          Strings::removeSuffix(fill, from);
          if (!fill.empty()) {
            output += fill;
            fill    = "";
          }
          // Add one matched token to log and reset all DFAS
          output += tos[i];
          for (auto& a:ats) {
            a = 0;
          }
          break; // important
        }
        // Otherwise that single token resets
      } else {
        at = 0; // token failed match, reset
      }
    }
  }
  output += fill;
  return output;
} // Strings::replaceGroup

std::string
Strings::formatBytes(long long bytesin, bool plusSign)
{
  const std::string sign = (bytesin < 0) ? "-" : (plusSign ? "+" : "");

  unsigned long long bytes = std::abs(bytesin);

  const unsigned long long GB = 1ULL << 30;
  const unsigned long long MB = 1ULL << 20;
  const unsigned long long KB = 1ULL << 10;

  std::string size;
  double value;

  if (bytes >= GB) {
    size  = "GB";
    value = static_cast<double>(bytes) / GB;
  } else if (bytes >= MB) {
    size  = "MB";
    value = static_cast<double>(bytes) / MB;
  } else if (bytes >= KB) {
    size  = "KB";
    value = static_cast<double>(bytes) / KB;
  } else {
    size  = "Bytes";
    value = static_cast<double>(bytes);
  }

  std::ostringstream oss;

  oss << sign << std::fixed << std::setprecision(2) << value << " " << size;
  return oss.str();
}

void
Strings::splitKeyParam(std::string_view commandline, std::string& key, std::string& params)
{
  size_t pos = commandline.find(',');

  if (pos != std::string_view::npos) {
    std::string newKey(commandline.substr(0, pos));
    std::string newParams(commandline.substr(pos + 1));
    key    = std::move(newKey);
    params = std::move(newParams);
  } else {
    key.assign(commandline);
    params.clear();
  }
}
