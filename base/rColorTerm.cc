#include "rColorTerm.h"

#include "rError.h"
#include "rStrings.h"
#include "rOS.h"

#include <iostream>
#include <iomanip>

#include <sys/ioctl.h>
#include <unistd.h>

using namespace rapio;
using namespace std;

bool ColorTerm::myUseColors       = false;
std::string ColorTerm::fNormal    = "";
std::string ColorTerm::fBold      = "";
std::string ColorTerm::fUnderline = "";

// Define array of standard colors
std::array<std::string, static_cast<size_t>(ColorTerm::Code::Count)> ColorTerm::myFColors = { };
std::array<std::string, static_cast<size_t>(ColorTerm::Code::Count)> ColorTerm::myBColors = { };
constexpr std::array<const char *, static_cast<size_t>(ColorTerm::Code::Count)> ColorTerm::colorValues;
constexpr std::array<const char *, static_cast<size_t>(ColorTerm::Code::Count)> ColorTerm::bgcolorValues;

void
ColorTerm::wrapWithIndent(size_t currentIndent,
  size_t                         indent,
  const std::string              & input,
  std::ostream                   & ss)
{
  const size_t w = getOutputWidth();

  const size_t leftover   = w - currentIndent;
  const size_t wrapamount = w - indent;

  if (Strings::removeANSII(input).length() > leftover) {
    std::vector<std::string> output;
    size_t c = Strings::wrap(input, &output, wrapamount, leftover);

    // Dump first line out.  If it's now less than the remaining
    // space...otherwise new line iff
    // the indent is < currentIndent.  Still might not fit, but we'll try...
    if (Strings::removeANSII(output[0]).length() <= leftover) {
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
  // Ok..we can use the LS_COLORS to maybe choose user preferred colors
  // I think at some point that beats the hardcoded ones since
  // different people have different backgrounds, etc.
  myUseColors = (flag && haveColorSupport());
  if (myUseColors) {
    fNormal    = "\033[0m";
    fBold      = "\033[1m";
    fUnderline = "\033[4m";
  } else {
    fNormal    = "";
    fBold      = "";
    fUnderline = "";
  }

  // Copy strings to active lookup
  for (size_t i = 0; i < static_cast<size_t>(Code::Count); ++i) {
    myFColors[i] = myUseColors ? colorValues[i] : "";
    myBColors[i] = myUseColors ? bgcolorValues[i] : "";
  }
}

bool
ColorTerm::haveColorSupport()
{
  // Check if outputting to terminal or pipe.
  // We'll turn off colors for pipes.  Makes less clutter
  // FIXME: need to ask logger probably, since it is possible
  // it's using a sink that isn't stdout, though the boost
  // logging seems flaky to me and I might try to rework all
  // of that.  Simple std::cout and std::err are probably enough.
  //
  // So for example ./binary help  --> displays colors
  // but ./binary help > output.txt --> turns off colors
  // FIXME: script still captures ansii anyway to fix that?
  bool terminal = isatty(fileno(stdout));

  if (!terminal) { return false; }

  // Respect standard NO_COLOR env variable
  std::string nc = OS::getEnvVar("NO_COLOR");

  if (!nc.empty()) {
    return false;
  }

  // Try to tell if terminal supports color output
  // If TERM is set to anything, we assume modern ANSI color support
  // without dragging in ncurses as a dependency.
  std::string term = OS::getEnvVar("TERM");

  // "dumb" is a standard TERM value for terminals that
  // explicitly DO NOT support ANSI escape codes like CICD pipelines.
  return !term.empty() && term != "dumb";
} // ColorTerm::haveColorSupport

size_t
ColorTerm::getOutputWidth()
{
  // Get the console width in linux
  static int console_width = 0;

  if (console_width == 0) { // Static..do it once...
    struct winsize w;
    if ((ioctl(0, TIOCGWINSZ, &w) == 0) && (w.ws_col > 0)) {
      console_width = w.ws_col - 5; // 'more' likes a margin, subtract 5.
    } else {
      console_width = 80; // Default fall back
    }

    // Min/Max of console width set
    if (console_width < 50) { console_width = 50; }

    if (console_width > 200) { console_width = 200; }
  }
  return (console_width);
}
