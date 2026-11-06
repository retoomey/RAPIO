// Add this at top for any BOOST test
#include "rBOOSTTest.h"

#include "rStrings.h"

using namespace rapio;

BOOST_AUTO_TEST_SUITE(_STRINGS_)

BOOST_AUTO_TEST_CASE(_STRINGS_PREFIX_SUFFIX_)
{
  // Use a non-null-terminated string_view slice into a larger buffer
  std::string buffer = "SparseLatLonGrid_EXTRA_TAIL";
  std::string_view sv(buffer.data(), 16); // "SparseLatLonGrid" (not null-terminated at index 16)

  BOOST_CHECK(Strings::beginsWith(sv, "Sparse"));
  BOOST_CHECK(!Strings::beginsWith(sv, "Dense"));
  BOOST_CHECK(Strings::endsWith(sv, "Grid"));
  BOOST_CHECK(!Strings::endsWith(sv, "TAIL"));

  std::string s(sv);
  BOOST_CHECK(Strings::removePrefix(s, "Sparse"));
  BOOST_CHECK_EQUAL(s, "LatLonGrid");
  BOOST_CHECK(!Strings::removePrefix(s, "Sparse"));

  BOOST_CHECK(Strings::removeSuffix(s, "Grid"));
  BOOST_CHECK_EQUAL(s, "LatLon");
  BOOST_CHECK(!Strings::removeSuffix(s, "Grid"));
}

BOOST_AUTO_TEST_CASE(_STRINGS_PEEL_AND_SPLIT_)
{
  // Test std::string peel
  std::string s = "http://localhost:8080/path";
  BOOST_CHECK_EQUAL(Strings::peel(s, "://"), "http");
  BOOST_CHECK_EQUAL(Strings::peel(s, ":"), "localhost");
  BOOST_CHECK_EQUAL(Strings::peel(s, "/"), "8080");
  BOOST_CHECK_EQUAL(s, "path");

  // Test zero-copy std::string_view peel
  std::string raw = "key1=val1&key2=val2#frag";
  std::string_view sv(raw);
  std::string_view query = Strings::peel(sv, "#");
  BOOST_CHECK_EQUAL(query, "key1=val1&key2=val2");
  BOOST_CHECK_EQUAL(sv, "frag");

  std::string_view pair1 = Strings::peel(query, "&");
  BOOST_CHECK_EQUAL(Strings::peel(pair1, "="), "key1");
  BOOST_CHECK_EQUAL(pair1, "val1");
  BOOST_CHECK_EQUAL(Strings::peel(query, "="), "key2");
  BOOST_CHECK_EQUAL(query, "val2");

  // Test splitWithoutEnds and split
  std::vector<std::string> pieces;
  BOOST_CHECK_EQUAL(Strings::splitWithoutEnds("/hello/", '/', &pieces), 1u);
  BOOST_CHECK_EQUAL(pieces[0], "hello");

  pieces.clear();
  BOOST_CHECK_EQUAL(Strings::split("/hello/", '/', &pieces), 3u);
  BOOST_CHECK_EQUAL(pieces[0], "");
  BOOST_CHECK_EQUAL(pieces[1], "hello");
  BOOST_CHECK_EQUAL(pieces[2], "");

  pieces.clear();
  BOOST_CHECK_EQUAL(Strings::split("  one   two  three ", &pieces), 3u);
  BOOST_CHECK_EQUAL(pieces[0], "one");
  BOOST_CHECK_EQUAL(pieces[1], "two");
  BOOST_CHECK_EQUAL(pieces[2], "three");

  // Test splitOnFirst and splitKeyParam
  std::vector<std::string> firstSplit;
  BOOST_CHECK(Strings::splitOnFirst("alpha,beta,gamma", ",", firstSplit));
  BOOST_CHECK_EQUAL(firstSplit.size(), 2u);
  BOOST_CHECK_EQUAL(firstSplit[0], "alpha");
  BOOST_CHECK_EQUAL(firstSplit[1], "beta,gamma");

  std::string key, params;
  Strings::splitKeyParam("algName,param1,param2", key, params);
  BOOST_CHECK_EQUAL(key, "algName");
  BOOST_CHECK_EQUAL(params, "param1,param2");

  // Test self-aliasing in splitOnFirst (input string_view points into setme[1])
  BOOST_CHECK(Strings::splitOnFirst(firstSplit[1], ",", firstSplit));
  BOOST_CHECK_EQUAL(firstSplit.size(), 2u);
  BOOST_CHECK_EQUAL(firstSplit[0], "beta");
  BOOST_CHECK_EQUAL(firstSplit[1], "gamma");

  // Test self-aliasing in splitKeyParam (commandline aliases params or key)
  Strings::splitKeyParam(params, key, params);
  BOOST_CHECK_EQUAL(key, "param1");
  BOOST_CHECK_EQUAL(params, "param2");
}

BOOST_AUTO_TEST_CASE(_STRINGS_UTILITIES_)
{
  BOOST_CHECK_EQUAL(Strings::makeUpper("AbCd"), "ABCD");
  BOOST_CHECK_EQUAL(Strings::makeLower("AbCd"), "abcd");

  std::string star;
  BOOST_CHECK(Strings::matchPattern("Vel*", "Velocity", star));
  BOOST_CHECK_EQUAL(star, "ocity");
  BOOST_CHECK(!Strings::matchPattern("Velocity", "Vel", star));
  BOOST_CHECK(!Strings::matchPattern("Velocity", "VelocityOther", star));

  BOOST_CHECK_EQUAL(Strings::removeNonNumber("val=-12.34e+5km"), "-12.34e+5");
  BOOST_CHECK_EQUAL(Strings::removeANSII("\033[31mRedText\033[0m"), "RedText");

  std::string repl = "foo_bar_foo";
  Strings::replace(repl, "foo", "baz");
  BOOST_CHECK_EQUAL(repl, "baz_bar_baz");
}

BOOST_AUTO_TEST_SUITE_END();
