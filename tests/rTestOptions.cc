#include "rBOOSTTest.h"
#include "rRAPIOOptions.h"
#include "rError.h"
#include "rOS.h"

using namespace rapio;

// Helper lambda to safely convert vector of strings into argv format
auto makeArgv = [](std::vector<std::string>& args) {
    std::vector<char *> argv;

    for (auto& arg : args) {
      argv.push_back(&arg[0]);
    }
    argv.push_back(nullptr);
    return argv;
  };

BOOST_AUTO_TEST_SUITE(OPTIONS_TESTS)

BOOST_AUTO_TEST_CASE(TEST_BASIC_DECLARATION_AND_DEFAULTS)
{
  RAPIOOptions o("TestAlg");

  o.optional("threshold", "15.5", "A threshold value");
  o.boolean("enable_filter", "Turn on the filter");
  o.require("input_dir", "/data", "Required input directory");

  o.setIsProcessed();

  BOOST_CHECK_EQUAL(o.getString("threshold"), "15.5");
  BOOST_CHECK_CLOSE(o.getFloat("threshold"), 15.5f, 0.001f);
  BOOST_CHECK_EQUAL(o.getBoolean("enable_filter"), false);
}

BOOST_AUTO_TEST_CASE(TEST_CLI_PARSING)
{
  RAPIOOptions o("TestAlg");

  o.optional("threshold", "15.5", "A threshold value");
  o.boolean("enable_filter", "Turn on the filter");
  o.require("input_dir", "/data", "Required input directory");

  std::vector<std::string> mockArgs = {
    "TestAlg",
    "-threshold=99.9",
    "-enable_filter",
    "-input_dir=/tmp/test"
  };

  auto argv      = makeArgv(mockArgs);
  bool wantsHelp = o.processArgs(mockArgs.size(), argv.data());

  // Wants help should be false
  BOOST_CHECK(!wantsHelp);

  // finalizeArgs handles the validation. It returns true if everything is valid!
  BOOST_CHECK(o.finalizeArgs(wantsHelp));

  BOOST_CHECK_EQUAL(o.getString("input_dir"), "/tmp/test");
  BOOST_CHECK_CLOSE(o.getFloat("threshold"), 99.9f, 0.001f);
  BOOST_CHECK_EQUAL(o.getBoolean("enable_filter"), true);
}

BOOST_AUTO_TEST_CASE(TEST_MISSING_REQUIRED_FAILS)
{
  RAPIOOptions o("TestAlg");

  o.require("input_dir", "/data", "Required input directory");

  std::vector<std::string> mockArgs = { "TestAlg" };
  auto argv = makeArgv(mockArgs);

  bool wantsHelp = o.processArgs(mockArgs.size(), argv.data());

  // finalizeArgs should return FALSE because we missed a required argument
  BOOST_CHECK(!o.finalizeArgs(wantsHelp));
}

BOOST_AUTO_TEST_CASE(TEST_SUBOPTION_ENFORCEMENT)
{
  // --- GOOD CASE ---
  RAPIOOptions o("TestAlg");

  o.optional("mode", "fast", "Processing mode");
  o.addSuboption("mode", "fast", "Run fast");
  o.addSuboption("mode", "slow", "Run slow");

  std::vector<std::string> argsValid = { "TestAlg", "-mode=slow" };
  auto argvValid = makeArgv(argsValid);

  bool wantsHelpV = o.processArgs(argsValid.size(), argvValid.data());

  BOOST_CHECK(o.finalizeArgs(wantsHelpV)); // Should pass

  // --- BAD CASE ---
  RAPIOOptions oBad("TestAlg");

  oBad.optional("mode", "fast", "Processing mode");
  oBad.addSuboption("mode", "fast", "Run fast");
  oBad.addSuboption("mode", "slow", "Run slow");

  std::vector<std::string> argsBad = { "TestAlg", "-mode=ludicrous" };
  auto argvBad = makeArgv(argsBad);

  bool wantsHelpB = oBad.processArgs(argsBad.size(), argvBad.data());

  BOOST_CHECK(!oBad.finalizeArgs(wantsHelpB)); // Should fail validation
}

BOOST_AUTO_TEST_CASE(TEST_MACRO_PWD_EXPANSION)
{
  RAPIOOptions o("TestAlg");

  // Include {PWD} in the default value to see if the Facade catches it
  o.optional("work_dir", "{PWD}/output", "Working directory");

  std::vector<std::string> args = { "TestAlg" };
  auto argv = makeArgv(args);

  o.processArgs(args.size(), argv.data());
  o.finalizeArgs(false);

  // Build what we expect {PWD} to expand to
  std::string expectedPath = rapio::OS::getCurrentDirectory() + "/output";

  BOOST_CHECK_EQUAL(o.getString("work_dir"), expectedPath);
}

BOOST_AUTO_TEST_CASE(TEST_POSITIONAL_ARGS_AND_TEXT_MACROS)
{
  RAPIOOptions o("TestAlg");

  o.optional("i", "", "input file");
  o.optional("o", "", "output directory");
  o.boolean("dry_run", "do not actually write files");

  // Simulate the rDump / rGetLegend text macro substitution
  o.setTextOnlyMacro("-i file=%s -o /tmp/out.nc");

  // We pass a standard flag AND a positional argument
  std::vector<std::string> args = {
    "TestAlg",
    "--dry_run",
    "my_radar_file.raw"
  };
  auto argv = makeArgv(args);

  bool wantsHelp = o.processArgs(args.size(), argv.data());

  BOOST_CHECK(o.finalizeArgs(wantsHelp)); // Should pass

  // 1. Verify standard flag survived the macro injection
  BOOST_CHECK_EQUAL(o.getBoolean("dry_run"), true);

  // 2. Verify the positional argument correctly injected into the '-i' flag
  BOOST_CHECK_EQUAL(o.getString("i"), "file=my_radar_file.raw");

  // 3. Verify the rest of the text macro was parsed (the '-o')
  BOOST_CHECK_EQUAL(o.getString("o"), "/tmp/out.nc");

  // 4. Verify the positional args were CONSUMED by the macro!
  auto posArgs = o.getPositionalArgs();

  BOOST_REQUIRE_EQUAL(posArgs.size(), 0); // They are gone now!
  BOOST_CHECK(o.isMacroApplied());
}

BOOST_AUTO_TEST_CASE(TEST_POSITIONAL_ARGS_WITHOUT_MACROS)
{
  RAPIOOptions o("TestAlg");

  o.boolean("dry_run", "do not actually write files");

  std::vector<std::string> args = {
    "TestAlg",
    "--dry_run",
    "some_leftover_file.raw"
  };
  auto argv = makeArgv(args);

  o.processArgs(args.size(), argv.data());

  // We don't call finalizeArgs here because it will purposefully
  // error out on unrecognized leftovers. We just want to check they exist!

  auto posArgs = o.getPositionalArgs();

  BOOST_REQUIRE_EQUAL(posArgs.size(), 1);
  BOOST_CHECK_EQUAL(posArgs[0], "some_leftover_file.raw");
}

BOOST_AUTO_TEST_SUITE_END()
