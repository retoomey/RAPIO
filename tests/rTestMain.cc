#include "rBOOST.h"

// Have to define these before the unit_test,
// they give us the special main() functions the test
// needs.
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "RAPIO Unit Tests"

// We've set flags, etc. in our special rBOOST.h header,
// so it's safe to now use unit_test
#include <boost/test/unit_test.hpp>
