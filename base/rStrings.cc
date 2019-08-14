#include "rStrings.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <cstring> // strncmp()

using namespace rapio;
using namespace std;

namespace {
/**
*** doSplit("//Hello//", s, Token('/') gives
*** s[0]=="", s[1]=="", s[2]=="Hello", s[3]=="", s[4]==""
***
*** doSplitGreedy folds adjacent `pred' hits together,
*** so doSplitGreedy("//Hello//", s, Token('/') gives
*** s[0]=="Hello"
**/

template <typename Predicate>
size_t
doSplitGreedy(const std::string & s,
  std::vector<std::string> *    setme,
  const Predicate               & pred)
{
  assert(setme && "Please pass in a valid vector.");

  const size_t old(setme->size());
  size_t n(old);
  setme->resize(old + Strings::countWords(s, pred));
  std::string::size_type pos(0), len(0);

  while (Strings::findNextWord(s, pos, len, pred, pos)) {
    (*setme)[n++].assign(s, pos, len);
    pos += len;
  }

  return (n - old);
}

template <typename Predicate>
size_t
doSplit(const std::string    & s,
  std::vector<std::string> * setme,
  const Predicate            & pred)
{
  assert(setme && "Please pass in a valid vector.");

  size_t old(setme->size());
  const size_t n_tokens(std::count_if(s.begin(), s.end(), pred) + 1);
  setme->resize(old + n_tokens);

  size_t n(old);
  std::string::const_iterator w(s.begin()), e(s.end());

  for (;;) {
    std::string::const_iterator it(std::find_if(w, e, pred));
    (*setme)[n++].assign(&*w, std::distance(w, it));

    if (it == e) { break; }
    w = it + 1;
  }

  assert(n == old + n_tokens);
  return (n_tokens);
}
}

bool
Strings::samePrefix(const std::string& a,
  const std::string                  & b,
  char                               delim)
{
  const std::string::size_type aPos = a.find(delim);
  const std::string::size_type bPos = b.find(delim);

  return (aPos == bPos && !a.compare(0, aPos, b, 0, aPos));
}

bool
Strings::beginsWith(const std::string& str, const std::string& prefix)
{
  return (!strncmp(str.c_str(), prefix.c_str(), prefix.size()));
}

bool
Strings::endsWith(const std::string& str, const std::string& suffix)
{
  return (str.size() >= suffix.size() && 0 == str.compare(
           str.size() - suffix.size(), suffix.size(), suffix));
}

void
Strings::toUpper(std::string& s)
{
  std::transform(s.begin(), s.end(), s.begin(), ::toupper);
}

std::string
Strings::makeUpper(const std::string& in)
{
  std::string out;
  std::transform(in.begin(), in.end(), std::back_inserter(out), ::toupper);
  return (out);
}

void
Strings::toLower(std::string& s)
{
  std::transform(s.begin(), s.end(), s.begin(), ::tolower);
}

std::string
Strings::makeLower(const std::string& in)
{
  std::string out;
  std::transform(in.begin(), in.end(), std::back_inserter(out), ::tolower);
  return (out);
}

bool
Strings::similar(const std::string& s1, const std::string& s2)
{
  std::string first(s1), second(s2);
  toUpper(first);
  toUpper(second);
  return (first == second);
}

void
Strings::capitalize(std::string& s)
{
  toLower(s);
  std::string::size_type pos(0), len(0);

  while (findNextWord(s, pos, len, pos)) {
    s[pos] = toupper(s[pos]);
    pos   += len;
  }
}

size_t
Strings::split(const std::string& s, std::vector<std::string> * setme)
{
  static const WhiteSpacePred ws;

  return (doSplitGreedy(s, setme, ws));
}

size_t
Strings::splitWithoutEnds(const std::string & s,
  char                                      delimiter,
  std::vector<std::string> *                setme)
{
  DelimiterPred pred(delimiter);

  return (doSplitGreedy(s, setme, pred));
}

size_t
Strings::split(const std::string & s,
  char                           delimiter,
  std::vector<std::string> *     setme)
{
  size_t n(0);

  if (s.empty()) { return (0); }

  if (s[0] == delimiter) { ++n; setme->push_back(""); }
  n += doSplitGreedy(s, setme, DelimiterPred(delimiter));

  if (s[s.size() - 1] == delimiter) { ++n; setme->push_back(""); }
  return (n);
}

size_t
Strings::splitToLines(const std::string & s,
  std::vector<std::string>              & lines)
{
  lines.clear();
  const DelimiterPred pred('\n');
  size_t n = doSplit(s, &lines, pred);

  // if unnecessary empty at end..
  if (!s.empty() && (s[s.size() - 1] == '\n')) {
    lines.resize(lines.size() - 1);
    --n;
  }
  return (n);
}

size_t
Strings::splitOnFirst(const std::string & s,
  char                                  delimiter,
  std::vector<std::string> *            setme)
{
  assert(setme && "Please pass in a valid vector.");

  size_t n_tokens = 0;
  const std::string::size_type pos(s.find(delimiter));

  if (pos != std::string::npos) {
    setme->push_back(s.substr(0, pos));
    setme->push_back(s.substr(pos + 1, s.size() - (pos + 1)));
    n_tokens = 2;
  }

  return (n_tokens);
}

#if 0

/* this code generates the whitespace table */
# include <stdio.h>
# include <string.h>
# include <limits.h>
# define min(a, b) (((a) < (b)) ? (a) : (b))
int
main(void)
{
  long i;
  const int per_line      = 10;
  const char * whitespace = " \t\r\n";

  printf("const bool Strings::myWhitespaceTable[] =\n{");

  for (i = 0; i < UCHAR_MAX; ++i) {
    if (!(i % per_line)) {
      printf("\n    /* %03d - %03d */  ", i,
        min(UCHAR_MAX, i + per_line) - 1);
    }
    const int is_space = i != 0 && strchr(whitespace, (char) i) != NULL;
    printf(is_space ? "1" : "0");

    if (i < UCHAR_MAX - 1) { printf(", "); }
  }
  printf("\n");
  printf("};\n");
  return (0);
}

#endif // if 0

// We have isspace in c++11
// 0x20 space (SPC)
// 0x09 horizontal tab (TAB) -- old
// 0x0a newline (LF)  -- old
// 0x0b vertical tab (VT)
// 0x0c feed (FF)
// 0x0d carriage return (CR) -- old
//
const bool Strings::myWhitespaceTable[] = {
  /* 000 - 009 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,

  /* 010 - 019 */ 1, 0, 0, 1, 0, 0, 0, 0, 0, 0,

  /* 020 - 029 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 030 - 039 */ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,

  /* 040 - 049 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 050 - 059 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 060 - 069 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 070 - 079 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 080 - 089 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 090 - 099 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 100 - 109 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 110 - 119 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 120 - 129 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 130 - 139 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 140 - 149 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 150 - 159 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 160 - 169 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 170 - 179 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 180 - 189 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 190 - 199 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 200 - 209 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 210 - 219 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 220 - 229 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 230 - 239 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 240 - 249 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 250 - 254 */ 0, 0, 0, 0, 0
};


void
Strings::replace(std::string & in,
  const std::string          & from,
  const std::string          & to)
{
  std::string out;
  std::string::size_type b(0), e(0);

  for (;;) {
    e = in.find(from, b);

    if (e == std::string::npos) {
      out.append(in, b, std::string::npos);
      break;
    } else {
      out.append(in, b, e - b);
      out.append(to);
      b = e + from.size();
    }
  }
  in = out;
}

std::string&
Strings::ltrim(std::string& s)
{
  s.erase(s.begin(), find_if_not(s.begin(), s.end(), [](int c){
    return isspace(c);
  }));
  return s;
}

std::string&
Strings::rtrim(std::string& s)
{
  s.erase(find_if_not(s.rbegin(), s.rend(), [](int c){
    return isspace(c);
  }).base(), s.end());
  return s;
}

std::string&
Strings::trim(std::string& s)
{
  //  string t=s;
  return ltrim(rtrim(s));
}

std::string
Strings::trimText(const std::string& s)
{
  std::string ret;

  const std::string::size_type len(s.size());

  if (!len) { return (ret); }

  // find the beginning
  const char * begin(&s[0]);
  const char * eos(begin + len);

  while (begin != eos && isWhiteSpace(*begin)) {
    ++begin;
  }

  if (begin == eos) { // all whitespace
    return (ret);
  }

  // find the end
  const char * end(&s[len - 1]);

  while (end != begin && isWhiteSpace(*end)) {
    --end;
  }
  ++end;

  ret.assign(begin, end - begin);
  return (ret);
}

std::string
Strings::replaceCharacter(const std::string& in, char old, char new_value)
{
  std::string s(in);
  std::replace(s.begin(), s.end(), old, new_value);
  return (s);
}

std::string
Strings::replaceWhiteSpace(const std::string& in, char new_value)
{
  std::string s(in);
  std::replace_if(s.begin(), s.end(), WhiteSpacePred(), new_value);
  return (s);
}

std::string
Strings::removeSubstrings(const std::string & in,
  const std::vector<std::string>            & discards)
{
  std::string out(in);

  std::string::size_type pos;

  for (auto it: discards) {
    if (((pos = out.find(it))) != out.npos) {
      out.erase(pos, it.size());
    }
  }

  return (out);
}

bool
Strings::matches(const std::string& pattern, const std::string& tocheck)
{
  // *bl?h.* matches Goopblah.jpg for example...
  const char * wild = pattern.c_str();
  const char * s    = tocheck.c_str();

  while ((*s) && (*wild != '*')) {
    if ((tolower(*wild) != tolower(*s)) && (*wild != '?')) {
      return (false);
    }
    wild++;
    s++;
  }

  const char * cp = 0, * mp = 0;

  while (*s) {
    if (*wild == '*') {
      if (!*++wild) { return (true); } mp = wild;
      cp = s + 1;
    } else if ((tolower(*wild) == tolower(*s)) || (*wild == '?')) {
      wild++;
      s++;
    } else {
      wild = mp;
      s    = cp++;
    }
  }

  while (*wild == '*') {
    wild++;
  }
  return (!*wild != 0);
} // Strings::matches

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

std::string
Strings::getPathUpTo(const std::string& str, const std::string& to)
{
  std::string out   = "";
  std::size_t found = str.find(to);

  if (found != std::string::npos) {
    out = str.substr(0, found + to.size());
  }
  return (out);
}
