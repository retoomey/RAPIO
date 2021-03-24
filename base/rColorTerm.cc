#include "rColorTerm.h"

#include "rError.h"
#include "rStrings.h"

#include <iostream>
#include <iomanip>

#include <sys/ioctl.h>

using namespace rapio;
using namespace std;

// Only calculated once for any module
unsigned int ColorTerm::console_width = 0;

std::string ColorTerm::fNormal       = "";
std::string ColorTerm::fBold         = "";
std::string ColorTerm::fBoldOff      = "";
std::string ColorTerm::fUnderline    = "";
std::string ColorTerm::fUnderlineOff = "";
std::string ColorTerm::fGreen        = "";
std::string ColorTerm::fRed  = "";
std::string ColorTerm::fBlue = "";

/** Output a string from current indention location until the edge of the
 * console, or wrap to a given indentation. */
void
ColorTerm::wrapWithIndent(size_t currentIndent,
  size_t                         indent,
  const std::string              & input,
  std::ostream                   & ss)
{
  size_t myOutputWidth = getOutputWidth();

  size_t leftover   = myOutputWidth - currentIndent;
  size_t wrapamount = myOutputWidth - indent;
  bool test;

  std::string noansii = Strings::removeANSII(input);
  test = (noansii.length() > leftover);

  if (test) {
    std::vector<std::string> output;
    size_t c = Strings::wrap(input, &output, wrapamount, leftover);

    // Dump first line out.  If it's now less than the remaining
    // space...otherwise new line iff
    // the indent is < currentIndent.  Still might not fit, but we'll try...
    noansii = Strings::removeANSII(output[0]);

    if (noansii.length() <= leftover) {
      ss << output[0] << "\n";
    } else {
      ss << "\n";

      if (indent > 0) {
        ss << setw(indent) << left << ""; // Indent 1 column
      }
      ss << output[0] << "\n";
    }

    // Dump rest of lines...
    for (size_t i = 1; i < c; i++) {
      if (indent > 0) {
        ss << setw(indent) << left << ""; // Indent 1 column
      }
      ss << output[i] << "\n";
    }
  } else {
    ss << input << "\n";
  }
} // ColorTerm::wrapWithIndent

void
ColorTerm::setColors(bool flag)
{
  if (flag) {
    fNormal       = "\033[0m";
    fBold         = "\033[1m";
    fBoldOff      = "\033[21m";
    fUnderline    = "\033[4m";
    fUnderlineOff = "\033[24m";
    // Ok..we can use the LS_COLORS to maybe choose user preferred colors
    // I think at some point that beats the hardcoded ones since
    // different people have different backgrounds, etc.
    fGreen = "\033[32m";
    // fGreen =  "\033[38;2;0;255;0;48;2;0;0;0m";
    // fRed  = "\033[31m";
    fRed = "\033[1;31m";
    // fRed = "\033[38;2;255;0;0;48;2;0;0;0m";
    // fBlue = "\033[34m";
    fBlue = "\033[1;34m";
    // fBlue = "\033[38;2;0;0;255;48;2;0;0;0m";
  } else {
    fNormal       = "";
    fBold         = "";
    fBoldOff      = "";
    fUnderline    = "";
    fUnderlineOff = "";
    fGreen        = "";
    fRed  = "";
    fBlue = "";
  }
}

bool
ColorTerm::haveColorSupport()
{
  // Try to tell if terminal supports color output
  bool haveColorTerms = false;
  char * term         = getenv("TERM");

  if (term != NULL) {
    // std::string theTerm = term;
    // Not wanting to add a ncurses dependency to check terminfo
    // In RedHat Enterprise based linux (which we are most
    // likely running on, ANSI colors 'should' be supported pretty much
    // always...)
    // if (false){
    //  std::cout << "The terminal is " << theTerm << "\n";
    // }
    // Could do a ncurses lookup in /usr/share/terminfo...
    haveColorTerms = true;
  }
  return (haveColorTerms);
}

size_t
ColorTerm::getOutputWidth()
{
  // Get the console width in linux
  if (console_width == 0) { // Static..do it once...
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    console_width = w.ws_col - 5; // 'more' likes a margin, subtract 5.
  }

  // Min/Max of console width set
  if (console_width < 50) { console_width = 50; }

  if (console_width > 200) { console_width = 200; }
  return (console_width);
}

std::string
ColorTerm::bold(const std::string& text)
{
  // Bold off seems to fail on some terminals.
  // So we use normal.
  return fBold + text + fNormal;
}

std::string
ColorTerm::underline(const std::string& text)
{
  // Underline off seems to fail on some terminals.
  // So we use normal.
  return fUnderline + text + fNormal;
}
