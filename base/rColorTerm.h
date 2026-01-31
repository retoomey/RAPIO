#pragma once

#include <iostream>
#include <array>

namespace rapio {
/** Handles colors and terminal pretty printing abilities for terminal.
 * Used by algorithm to do the pretty printing of algorithm help, etc.
 *
 * @author Robert Toomey
 * @ingroup rapio_utility
 * @brief Terminal color and padding utility
 **/
class ColorTerm {
public:

  /** Standard ANSI 8+8 lookup.  Usually mapped by terminal theme. */
  enum class Code : uint8_t {
    Black = 0,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White,
    BrightBlack,
    BrightRed,
    BrightGreen,
    BrightYellow,
    BrightBlue,
    BrightMagenta,
    BrightCyan,
    BrightWhite,
    Count // always keep this last!
  };

  /** Group class, no constructor */
  ColorTerm() = delete;

  /** Group class, no destructor */
  ~ColorTerm() = delete;

  /** Output a string from current indention location until the edge of the
   * console, or wrap to a given indentation. */
  static void
  wrapWithIndent(size_t currentIndent,
    size_t              indent,
    const std::string   & input,
    std::ostream        & ss = std::cout);

  /** Turn on/off color output */
  static void
  setColors(bool flag);

  /** See if terminal supports colors */
  static bool
  haveColorSupport();

  /** Get output width (usually based on console if available) */
  static size_t
  getOutputWidth();

  // Text colors and reset

  /** Turn off any formatting */
  static inline std::string
  reset()
  {
    return fNormal;
  }

  // Static color accessor functions
  static inline const std::string& black(){ return myFColors[static_cast<size_t>(Code::Black)]; }

  static inline const std::string& red(){ return myFColors[static_cast<size_t>(Code::Red)]; }

  static inline const std::string& green(){ return myFColors[static_cast<size_t>(Code::Green)]; }

  static inline const std::string& yellow(){ return myFColors[static_cast<size_t>(Code::Yellow)]; }

  static inline const std::string& blue(){ return myFColors[static_cast<size_t>(Code::Blue)]; }

  static inline const std::string& magenta(){ return myFColors[static_cast<size_t>(Code::Magenta)]; }

  static inline const std::string& cyan(){ return myFColors[static_cast<size_t>(Code::Cyan)]; }

  static inline const std::string& white(){ return myFColors[static_cast<size_t>(Code::White)]; }

  static inline const std::string& brightRed(){ return myFColors[static_cast<size_t>(Code::BrightRed)]; }

  static inline const std::string& brightGreen(){ return myFColors[static_cast<size_t>(Code::BrightGreen)]; }

  static inline const std::string& brightBlue(){ return myFColors[static_cast<size_t>(Code::BrightBlue)]; }

  // Static color accessor functions
  static inline const std::string& bgBlack(){ return myBColors[static_cast<size_t>(Code::Black)]; }

  static inline const std::string& bgRed(){ return myBColors[static_cast<size_t>(Code::Red)]; }

  static inline const std::string& bgGreen(){ return myBColors[static_cast<size_t>(Code::Green)]; }

  static inline const std::string& bgYellow(){ return myBColors[static_cast<size_t>(Code::Yellow)]; }

  static inline const std::string& bgBlue(){ return myBColors[static_cast<size_t>(Code::Blue)]; }

  static inline const std::string& bgMagenta(){ return myBColors[static_cast<size_t>(Code::Magenta)]; }

  static inline const std::string& bgCyan(){ return myBColors[static_cast<size_t>(Code::Cyan)]; }

  static inline const std::string& bgWhite(){ return myBColors[static_cast<size_t>(Code::White)]; }

  static inline const std::string& bgBrightRed(){ return myBColors[static_cast<size_t>(Code::BrightRed)]; }

  static inline const std::string& bgBrightGreen(){ return myBColors[static_cast<size_t>(Code::BrightGreen)]; }

  static inline const std::string& bgBrightBlue(){ return myBColors[static_cast<size_t>(Code::BrightBlue)]; }

  /** Use a rgb color. */
  static inline std::string
  RGB(int r, int g, int b)
  {
    if (myUseColors) {
      return "\033[38;2;" + std::to_string(r) + ";"
             + std::to_string(g) + ";"
             + std::to_string(b) + "m";
    }
    return "";
  }

  /** Use a rgb background color */
  static inline std::string
  bRGB(int r, int g, int b)
  {
    if (myUseColors) {
      return "\033[48;2;" + std::to_string(r) + ";"
             + std::to_string(g) + ";"
             + std::to_string(b) + "m";
    }
    return "";
  }

  /** Use bold font */
  static inline std::string
  bold()
  {
    return fBold;
  }

  /** Simple quick bold string wrapper */
  static inline std::string
  bold(const std::string& text)
  {
    if (myUseColors) {
      // Bold off seems to fail on some terminals.
      // So we use normal.
      return fBold + text + fNormal;
    } else {
      return text;
    }
  }

  /** Use underline font */
  static inline std::string
  underline()
  {
    return fUnderline;
  }

  /** Simple quick underline string wrapper */
  static inline std::string
  underline(const std::string& text)
  {
    if (myUseColors) {
      // Underline off seems to fail on some terminals.
      // So we use normal.
      return fUnderline + text + fNormal;
    } else {
      return text;
    }
  }

private:

  /** The active color lookup */
  static std::array<std::string, static_cast<size_t>(Code::Count)> myFColors;
  /** The active color lookup */
  static std::array<std::string, static_cast<size_t>(Code::Count)> myBColors;

  /** Constants for foreground color codes */
  static constexpr std::array<const char *, static_cast<size_t>(Code::Count)> colorValues = { {
    "\033[30m", // Black
    "\033[31m", // Red
    "\033[32m", // Green
    "\033[33m", // Yellow
    "\033[34m", // Blue
    "\033[35m", // Magenta
    "\033[36m", // Cyan
    "\033[37m", // White
    "\033[90m", // Bright Black (Gray)
    "\033[91m", // Bright Red
    "\033[92m", // Bright Green
    "\033[93m", // Bright Yellow
    "\033[94m", // Bright Blue
    "\033[95m", // Bright Magenta
    "\033[96m", // Bright Cyan
    "\033[97m"  // Bright White
  } };

  /** Constants for background color codes */
  static constexpr std::array<const char *, static_cast<size_t>(Code::Count)> bgcolorValues = { {
    "\033[40m",  // Black
    "\033[41m",  // Red
    "\033[42m",  // Green
    "\033[43m",  // Yellow
    "\033[44m",  // Blue
    "\033[45m",  // Magenta
    "\033[46m",  // Cyan
    "\033[47m",  // White
    "\033[100m", // Bright Black (Gray)
    "\033[101m", // Bright Red
    "\033[102m", // Bright Green
    "\033[103m", // Bright Yellow
    "\033[104m", // Bright Blue
    "\033[105m", // Bright Magenta
    "\033[106m", // Bright Cyan
    "\033[107m"  // Bright White
  } };

  /** Normal/reset text */
  static std::string fNormal;

  /** Start bold text */
  static std::string fBold;

  /** Start underline text */
  static std::string fUnderline;

  /** Are we able and set to use colors? */
  static bool myUseColors;

  /** The width of all our command line output */
  static size_t myOutputWidth;
};
}
