#pragma once

#include <rUtility.h>

namespace rapio {
/** Handles colors and terminal pretty printing abilities for terminal.
 * Used by algorithm to do the pretty printing of algorithm help, etc.
 *
 * @author Robert Toomey
 **/
class ColorTerm : public Utility {
public:

  ColorTerm();
  ~ColorTerm(){ }

  /** A console width grabbed from terminal */
  static unsigned int console_width;

  static std::string fNormal;
  static std::string fBold;
  static std::string fBoldOff;
  static std::string fUnderline;
  static std::string fUnderlineOff;
  static std::string fGreen;
  static std::string fRed;
  static std::string fBlue;

  /** Pretty print with wrap */
  static void
  wrapWithIndent(size_t currentIndent,
    size_t              indent,
    const std::string   & input);

  /** Turn on/off color output */
  static void
  setColors(bool flag);

  /** See if terminal supports colors */
  static bool
  haveColorSupport();

  /** Get output width (usually based on console if available) */
  static size_t
  getOutputWidth();

  /** Simple quick bold string wrapper */
  static std::string
  bold(const std::string& in);

  /** Simple quick underline string wrapper */
  static std::string
  underline(const std::string& in);

private:

  /** The width of all our command line output */
  static size_t myOutputWidth;
};
}
