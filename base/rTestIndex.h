#pragma once

#include <rTest.h>

namespace rapio {
/** A test of anything index related.
 *
 *  @see Index
 *  @author Robert Toomey
 *
 */
class TestIndex : public Test {
public:
  virtual void
  logicTest() override;
  virtual std::string
  name() override { return "Index class"; }
};
}
