#include "rBOOST.h"

// Have to define these before the unit_test,
// they give us the special main() functions the test
// needs.
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "RAPIO Unit Tests"

// We've set flags, etc. in our special rBOOST.h header,
// so it's safe to now use unit_test
BOOST_WRAP_PUSH
#include <boost/test/unit_test.hpp>
BOOST_WRAP_POP

// Add the runtime header
#include "rRAPIORuntime.h"

// Define a global fixture to initialize the RAPIO environment once
// This loads XML, JSON and other abilities
struct GlobalRuntimeFixture {
  GlobalRuntimeFixture()
  {
    rapio::RAPIORuntime::initialize();
  }

  ~GlobalRuntimeFixture() = default;
};

// Register it with Boost.Test
BOOST_TEST_GLOBAL_FIXTURE(GlobalRuntimeFixture);
