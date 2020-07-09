#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "RAPIO Unit Tests"
#include <boost/test/unit_test.hpp>

/*
 * int add(int i, int j)
 * {
 *  return i + j;
 * }
 *
 **
 * struct DATATHINGIE
 * {
 *    int m; // accessible variable
 * }
 **
 **/
/ BOOST_FIXTURE_TEST_SUITE(Robert1, DATATHINGIE);
**
**BOOST_AUTO_TEST_SUITE(Robert1)
* *
**BOOST_AUTO_TEST_CASE(universeInOrder)
* *{
  **BOOST_CHECK(add(2, 2) == 4);
  **
}
**
**BOOST_AUTO_TEST_SUITE_END();
*
/
