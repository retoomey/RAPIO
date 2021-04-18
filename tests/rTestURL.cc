// Add this at top for any BOOST test
#include <boost/test/unit_test.hpp>

#include "rURL.h"

using namespace rapio;

BOOST_AUTO_TEST_SUITE(_URL_)

BOOST_AUTO_TEST_CASE(_URL_FIELDS_)
{
  URL x = URL("https://www.google.com");

  BOOST_CHECK_EQUAL(x.getHost(), "www.google.com");

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

  // Testing query options...
  BOOST_CHECK_EQUAL(v.getQuery("test"), "123");
  BOOST_CHECK_EQUAL(v.getQuery("size"), "2");

  // std::cout << v.toString() << "\n";
  BOOST_CHECK_EQUAL(v.toString(), "http://bob:secret@www.mrms.noaa.com:8080/somepath.asp?size=2&test=123");
  v.setQuery("test", "testvalue");
  v.setQuery("goop", "goop@v  alue");
  v.setUser("tom");
  v.setPassword("hello!!");
  v.setPort(82);
  v.setQuery("fred", "&&&&");
  v.setFragment("fragment");
  BOOST_CHECK_EQUAL(v.getQuery("test"), "testvalue");
  BOOST_CHECK_EQUAL(v.getQuery("goop"), "goop@v  alue");
  BOOST_CHECK_EQUAL(v.getQuery("fred"), "&&&&");
  BOOST_CHECK_EQUAL(v.getUser(), "tom");
  BOOST_CHECK_EQUAL(v.getPassword(), "hello!!");
  BOOST_CHECK_EQUAL(v.getPort(), 82);
  BOOST_CHECK_EQUAL(
    v.toString(),
    "http://tom:hello%21%21@www.mrms.noaa.com:82/somepath.asp?fred=%26%26%26%26&goop=goop%40v%20%20alue&size=2&test=testvalue#fragment");

  v.setHost("vm-aws-mr@ms-sr1d");
  v.clearQuery();
  v.setQuery("again", "somevalue");
  v.setPassword("badpassword");
  v.setUser("");
  v.setQuery("username", "peter@nable.com");
  v.setScheme("https");
  v.setPath("newpath.asp");
  std::string host = v.getHost();
  BOOST_CHECK_EQUAL(v.getHost(), "vm-aws-mr@ms-sr1d");
  BOOST_CHECK_EQUAL(
    v.toString(), "https://vm-aws-mr%40ms-sr1d/newpath.asp?again=somevalue&username=peter%40nable.com#fragment");
  BOOST_CHECK_EQUAL(v.isLocal(), false);

  // Base name stuff
  URL file = URL("/path/path2/path3/file.tar.gz");
  BOOST_CHECK_EQUAL(file.getBaseName(), "file.tar.gz");
  BOOST_CHECK_EQUAL(file.getDirName(), "/path/path2/path3");
  BOOST_CHECK_EQUAL(file.isLocal(), true);

  URL file2 = URL("http://localhost/path/path2/path3/file.tar.gz");
  BOOST_CHECK_EQUAL(file2.getBaseName(), "file.tar.gz");
  BOOST_CHECK_EQUAL(file2.getDirName(), "/path/path2/path3");
  BOOST_CHECK_EQUAL(file2.isLocal(), true);

  // getSuffixLC -- I might remove this
}

BOOST_AUTO_TEST_SUITE_END();
