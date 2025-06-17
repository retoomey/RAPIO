#include "rWgribCallback.h"
#include "rURL.h"
#include "rError.h"
#include "rIOWgrib.h"

#include <sstream>

using namespace rapio;

void
WgribCallback::execute()
{
  RAPIOCallbackCPP catalog(this);

  // Create the args for wgrib2
  std::vector<std::string> args;

  args.push_back("wgrib2");
  args.push_back(myFilename);

  // Allow subclasses to add extra args such as -match
  addExtraArgs(args);

  // Convert pointer to integer for callback
  args.push_back("-rapio");
  std::ostringstream oss;

  oss << reinterpret_cast<uintptr_t>(&catalog);
  std::string pointer = oss.str();

  args.push_back(pointer);

  // wgrib2 wants a c style array, so convert,
  // the vector will be in scope
  std::vector<const char *> argv;

  for (const auto& arg : args) {
    argv.push_back(arg.c_str());
  }
  int argc = argv.size();

  // Print the arguments to command line
  std::ostringstream param;

  for (size_t i = 0; i < argc; ++i) {
    param << argv[i];
    if (i < argc - 1) { param << " "; }
  }
  LogInfo("Calling: " << param.str() << "\n");

  // FIXME: Mode flag to toggle capture vs direct output
  // Execute and capture output
  std::vector<std::string> result =
    IOWgrib::capture_vstdout_of_wgrib2(argv.size(), argv.data());

  for (size_t i = 0; i < result.size(); ++i) {
    LogInfo("[wgrib2] " << result[i] << "\n");
  }
} // WgribCallback::execute
