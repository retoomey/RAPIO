#pragma once
#include "rBOOST.h"
#include "rRAPIORuntime.h"

// 1. Keep dynamic linking to ensure fast compile times!
#define BOOST_TEST_DYN_LINK

// 2. Trigger Boost to generate a main() function automatically.
// We use #ifndef so individual tests can override the suite name if they want.
#ifndef BOOST_TEST_MODULE
#define BOOST_TEST_MODULE RAPIO_Standalone_Test
#endif

BOOST_WRAP_PUSH
#include <boost/test/unit_test.hpp>
BOOST_WRAP_POP

// 3. Inject the global runtime initialization automatically
struct GlobalRuntimeFixture {
  GlobalRuntimeFixture() {
    rapio::RAPIORuntime::initialize();
  }
  ~GlobalRuntimeFixture() = default;
};
BOOST_TEST_GLOBAL_FIXTURE(GlobalRuntimeFixture);
