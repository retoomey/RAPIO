#include "rBOOSTTest.h"
#include "rRAPIOOptions.h"
#include "rError.h"

using namespace rapio;

BOOST_AUTO_TEST_SUITE(OPTIONS_TESTS)

BOOST_AUTO_TEST_CASE(TEST_BASIC_DECLARATION_AND_DEFAULTS)
{
  RAPIOOptions o("TestAlg");

  o.optional("threshold", "15.5", "A threshold value");
  o.boolean("enable_filter", "Turn on the filter");
  o.require("input_dir", "/data", "Required input directory");

  // Force processed state so we can read defaults without parsing CLI args
  o.setIsProcessed();

  // Check defaults
  BOOST_CHECK_EQUAL(o.getString("threshold"), "15.5");
  BOOST_CHECK_CLOSE(o.getFloat("threshold"), 15.5f, 0.001f);

  // Boolean default is "false" unless specified
  BOOST_CHECK_EQUAL(o.getBoolean("enable_filter"), false);
}

BOOST_AUTO_TEST_CASE(TEST_CLI_PARSING)
{
  RAPIOOptions o("TestAlg");

  o.optional("threshold", "15.5", "A threshold value");
  o.boolean("enable_filter", "Turn on the filter");
  o.require("input_dir", "/data", "Required input directory");

  // Mock up some command line arguments
  std::vector<std::string> mockArgs = {
    "TestAlg",             // argv[0] usually the program name
    "-threshold=99.9",     // Override default
    "-enable_filter",      // Set boolean to true
    "-input_dir=/tmp/test" // Provide required arg
  };

  // Convert to C-style argv array
  std::vector<char *> argv;

  for (auto& arg : mockArgs) {
    argv.push_back(&arg[0]);
  }
  argv.push_back(nullptr); // Strictly compliant null-termination

  // Process them directly using the updated C-style signature
  bool wantsHelp = o.processArgs(mockArgs.size(), argv.data());

  BOOST_CHECK(!wantsHelp);
  // Verify using the new DTO getters
  BOOST_CHECK(o.getMissingRequired().empty());
  BOOST_CHECK(o.getUnrecognizedOptions().empty());

  // Verify the parsed values
  BOOST_CHECK_EQUAL(o.getString("input_dir"), "/tmp/test");
  BOOST_CHECK_CLOSE(o.getFloat("threshold"), 99.9f, 0.001f);
  BOOST_CHECK_EQUAL(o.getBoolean("enable_filter"), true);
}

BOOST_AUTO_TEST_CASE(TEST_MISSING_REQUIRED_FAILS)
{
  RAPIOOptions o("TestAlg");

  o.require("input_dir", "/data", "Required input directory");

  // Don't provide the required argument
  std::vector<std::string> mockArgs = { "TestAlg" };
  std::vector<char *> argv = { &mockArgs[0][0], nullptr };

  o.processArgs(mockArgs.size(), argv.data());

  // getMissingRequired should return 1 item because input_dir is missing
  BOOST_CHECK(!o.getMissingRequired().empty());
}

BOOST_AUTO_TEST_CASE(TEST_SUBOPTION_ENFORCEMENT)
{
  RAPIOOptions o("TestAlg");

  o.optional("mode", "fast", "Processing mode");
  o.addSuboption("mode", "fast", "Run fast");
  o.addSuboption("mode", "slow", "Run slow");

  // 1. Test Valid Suboption
  std::vector<std::string> argsValid = { "TestAlg", "-mode=slow" };
  std::vector<char *> argvValid      = { &argsValid[0][0], &argsValid[1][0], nullptr };

  o.processArgs(argsValid.size(), argvValid.data());

  // Valid suboption should leave the invalid DTO vector empty
  BOOST_CHECK(o.getInvalidSuboptions().empty());

  // 2. Test Invalid Suboption
  RAPIOOptions oBad("TestAlg");

  oBad.optional("mode", "fast", "Processing mode");
  oBad.addSuboption("mode", "fast", "Run fast");
  oBad.addSuboption("mode", "slow", "Run slow");

  std::vector<std::string> argsBad = { "TestAlg", "-mode=ludicrous" };
  std::vector<char *> argvBad      = { &argsBad[0][0], &argsBad[1][0], nullptr };

  oBad.processArgs(argsBad.size(), argvBad.data());

  // getInvalidSuboptions should catch the invalid "ludicrous" choice
  BOOST_CHECK(!oBad.getInvalidSuboptions().empty());
}

BOOST_AUTO_TEST_SUITE_END()
