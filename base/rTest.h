#pragma once

#include <rRapio.h>
#include <rFactory.h>

namespace rapio {
/** Base class for all unit tests
 * Building them into the algorithm for the moment since it will allow me
 * to develop quickly.  Probably better to separate the compiling of tests
 * later on once/if they get large...
 *
 * @author Robert Toomey
 */
class Test : public Rapio {
public:

  virtual bool
  checkEqual(bool test, const std::string& condition, const std::string& file, int line);

  virtual void
  logicTest(){ }

  virtual std::string
  name(){ return "Test group"; }

  // Factory methods
  static void
  doTests();

  static void
  introduce(const std::string& name, std::shared_ptr<Test> test)
  {
    Factory<Test>::introduce(name, test);
  }
};

// Pass test to function so we can do more with it
#define check(A) \
  { \
    checkEqual(A, #A, __FILE__, __LINE__); \
  }
}
