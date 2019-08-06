#include "rTest.h"
#include "rFactory.h"

#include "rTestTime.h"
#include "rTestIndex.h"

#include <iostream>

using namespace rapio;
using namespace std;

static size_t passCount;
static size_t failCount;
static size_t totalCount;

// Note: could get some crazy compile errors if you goof the condition.
// We're trusting the coder to not hack the macro
bool
Test::checkEqual(bool test, const std::string& condition, const std::string& file, int line)
{
  totalCount++;
  bool pass = false;
  if (!test) {
    failCount++;
  } else {
    passCount++;
    pass = true;
    return true;
  }
  if (pass) {
    // Blank
  } else {
    std::cout << "Test: " << file << " " << line << " \"" << condition << "\" ";
    std::cout << "-->FAIL\n";
  }

  return test;
}

void
Test::doTests()
{
  // FIXME: need to be able to add tests from others
  // so separate introduction from running
  std::shared_ptr<Test> t(new TestTime());
  introduce(t->name(), t);
  std::shared_ptr<Test> t2(new TestIndex());
  introduce(t2->name(), t2);

  // Run all tests all children, for now
  passCount  = 0;
  failCount  = 0;
  totalCount = 0;
  auto children = Factory<Test>::getAll();
  for (auto& c:children) {
    LogInfo("Running test(s) for " << c.second->name() << "\n");
    c.second->logicTest();
  }
  LogInfo("Total tests ran: " << totalCount << "\n");
  LogInfo("Passed: " << passCount << "\n");
  LogInfo("Failed: " << failCount << "\n");
}
