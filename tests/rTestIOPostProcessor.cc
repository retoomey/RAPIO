#include "rBOOSTTest.h"
#include "rIOPostProcessor.h"
#include "rOS.h"
#include "rDataFilter.h"
#include <fstream>
#include <map>
#include <sys/stat.h> // for chmod
#include <cstdlib>    // for getend, setenv

using namespace rapio;

// Helper to create a dummy file for the pipeline to process
std::string
createDummyFile(const std::string& prefix)
{
  std::string path = OS::getUniqueTemporaryFile(prefix);
  std::ofstream out(path);

  out << "Dummy data for pipeline test.\n";
  out.close();
  return path;
}

BOOST_AUTO_TEST_SUITE(IO_POST_PROCESSOR_TESTS)

BOOST_AUTO_TEST_CASE(TEST_ATOMIC_RENAME_ONLY)
{
  std::string tempFile  = createDummyFile("test-temp-");
  std::string finalFile = OS::getUniqueTemporaryFile("test-final-");

  std::map<std::string, std::string> keys;

  keys["filename"] = finalFile; // Target destination

  IOPostProcessor pipeline;

  pipeline.buildPipeline(keys); // Should only add AtomicRenameStep

  bool success = pipeline.run(tempFile, keys);

  BOOST_CHECK(success);
  BOOST_CHECK(!OS::isRegularFile(tempFile)); // Temp should be gone
  BOOST_CHECK(OS::isRegularFile(finalFile)); // Final should exist

  // Cleanup
  OS::deleteFile(finalFile);
}

BOOST_AUTO_TEST_CASE(TEST_COMPRESSION_AND_RENAME)
{
  // Hack.  Initialize the DataFilter factory so we can find "gz"
  rapio::DataFilter::introduceSelf();

  std::string tempFile      = createDummyFile("test-temp-");
  std::string finalBaseFile = OS::getUniqueTemporaryFile("test-final-");

  std::map<std::string, std::string> keys;

  keys["filename"]    = finalBaseFile;
  keys["compression"] = "gz"; // Trigger the CompressionStep

  IOPostProcessor pipeline;

  pipeline.buildPipeline(keys);

  bool success = pipeline.run(tempFile, keys);

  // The pipeline modifies the filename key to append the extension
  std::string expectedFinalFile = finalBaseFile + ".gz";

  BOOST_CHECK(success);
  BOOST_CHECK_EQUAL(keys["filename"], expectedFinalFile);
  BOOST_CHECK(!OS::isRegularFile(tempFile));
  BOOST_CHECK(OS::isRegularFile(expectedFinalFile));

  // Cleanup
  OS::deleteFile(expectedFinalFile);
}

BOOST_AUTO_TEST_CASE(TEST_SAFE_COMMAND_FAILURE_ABORTS_PIPELINE)
{
  std::string tempFile  = createDummyFile("test-temp-");
  std::string finalFile = OS::getUniqueTemporaryFile("test-final-");

  std::map<std::string, std::string> keys;

  keys["filename"]  = finalFile;
  keys["postwrite"] = "some_non_existent_command %filename%";

  IOPostProcessor pipeline;

  pipeline.buildPipeline(keys);

  // Pipeline should run the rename (which succeeds), but fail on the post-write command
  bool success = pipeline.run(tempFile, keys);

  BOOST_CHECK(!success);

  // Cleanup (the rename step succeeded before the command failed, so we clean the final file)
  OS::deleteFile(finalFile);
}

BOOST_AUTO_TEST_CASE(TEST_LDM_INSERTION_FAKE_BINARY)
{
  // 1. Setup the fake binary environment
  std::string currentDir     = rapio::OS::getCurrentDirectory();
  std::string fakeBinaryPath = currentDir + "/pqinsert";
  std::string outputFile     = currentDir + "/fake_pqinsert_output.txt";

  // Clean up any lingering output from previous runs
  rapio::OS::deleteFile(outputFile);

  // Create a fake bash script that writes its arguments to our output file
  std::ofstream script(fakeBinaryPath);

  script << "#!/bin/bash\n";
  script << "echo \"$@\" > " << outputFile << "\n";
  script << "exit 0\n";
  script.close();

  // Make the fake script executable (chmod 755)
  chmod(fakeBinaryPath.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

  // 2. Hijack the PATH variable so OS::validateExe finds our fake script first
  const char * oldPath = getenv("PATH");
  std::string newPath  = currentDir + ":" + (oldPath ? oldPath : "");

  setenv("PATH", newPath.c_str(), 1);

  // 3. Run the pipeline
  std::string tempFile  = createDummyFile("test-ldm-");
  std::string finalFile = rapio::OS::getUniqueTemporaryFile("test-final-");

  std::map<std::string, std::string> keys;

  keys["filename"]  = finalFile;
  keys["postwrite"] = "ldm"; // Triggers our custom LDMInsertStep

  rapio::IOPostProcessor pipeline;

  pipeline.buildPipeline(keys);

  bool success = pipeline.run(tempFile, keys);

  BOOST_CHECK(success);

  // Verify the exact arguments passed to the fake binary
  std::ifstream resultFile(outputFile);

  BOOST_REQUIRE(resultFile.is_open());
  std::string argsPassed;

  std::getline(resultFile, argsPassed);
  resultFile.close();

  // Generate the expected output using the Single Source of Truth!
  std::vector<std::string> expectedArgVector = rapio::LDMInsertStep::buildLdmArgs(finalFile);
  std::string expectedArgs;

  // The bash script's "$@" echoes everything EXCEPT the executable name (index 0).
  // We join the rest with spaces to match the bash output.
  for (size_t i = 1; i < expectedArgVector.size(); ++i) {
    expectedArgs += expectedArgVector[i];
    if (i < expectedArgVector.size() - 1) {
      expectedArgs += " ";
    }
  }

  BOOST_CHECK_EQUAL(argsPassed, expectedArgs);

  // 5. Teardown and Cleanup
  if (oldPath) {
    setenv("PATH", oldPath, 1);
  }
  rapio::OS::deleteFile(fakeBinaryPath);
  rapio::OS::deleteFile(outputFile);
  rapio::OS::deleteFile(finalFile);
}

BOOST_AUTO_TEST_SUITE_END()
