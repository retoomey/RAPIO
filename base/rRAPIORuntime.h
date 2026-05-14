#pragma once
#include <mutex>

namespace rapio {
/** Runtime is the collection of critical start up libraries and
 * abilities for running RAPIO
 *
 * @author Robert Toomey
 */
class RAPIORuntime {
public:
  /** Safe to call multiple times; will only execute once. */
  static void
  initialize();

private:
  /** Thread safe status of initialize */
  static std::once_flag ourInitFlag;

  /** Initialize system wide base parsers like XML/JSON
   * these are typically critical for initial setup */
  static void
  initializeBaseParsers();

  /** Initialize any base modules requiring configuration */
  static void
  initializeBaseline();
};
}
