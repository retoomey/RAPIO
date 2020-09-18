// Add this at top for any BOOST test
#include <boost/test/unit_test.hpp>

#include "rURL.h"

using namespace rapio;

BOOST_AUTO_TEST_SUITE(_URL_)

BOOST_AUTO_TEST_CASE(_URL_FIELDS_)
{
  URL u = URL(
    "https://www.mrms.noaa.com/data1/data2/ref=test_1?z=cool&ie=UTF8&qid=1430344305&sr=1-1&keywords=nozick#frag123");

  BOOST_CHECK_EQUAL(u.getScheme(), "https");
  BOOST_CHECK_EQUAL(u.getHost(), "www.mrms.noaa.com");
  BOOST_CHECK_EQUAL(u.getPort(), 443); // ok default ports if missing, right?
  BOOST_CHECK_EQUAL(u.getPath(), "/data1/data2/ref=test_1");
  BOOST_CHECK_EQUAL(u.getFragment(), "frag123");

  URL v = URL("http://bob:secret@www.mrms.noaa.com:8080/somepath.asp?test=123&size=2");

  BOOST_CHECK_EQUAL(v.getScheme(), "http");
  BOOST_CHECK_EQUAL(v.getUser(), "bob");
  BOOST_CHECK_EQUAL(v.getPassword(), "secret");
  BOOST_CHECK_EQUAL(v.getHost(), "www.mrms.noaa.com");
  BOOST_CHECK_EQUAL(v.getPort(), 8080);
  BOOST_CHECK_EQUAL(v.getPath(), "/somepath.asp");

  // getBaseName()
  // getDirName()
  // getSuffix()
  // getSuffixLC()

  // BOOST_CHECK(t.getSecondsSinceEpoch() == 220);
}

BOOST_AUTO_TEST_SUITE_END();
