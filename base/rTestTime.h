#pragma once

#include <rTest.h>

namespace rapio {
/** A test of anything time related.
 *
 *  @see Time
 *  @author Robert Toomey
 *
 *  The first test class.  Will need work
 */
class TestTime : public Test {
public:
  virtual void
  logicTest() override;
  virtual std::string
  name() override { return "Time class"; }
};
}
