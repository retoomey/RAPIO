#include "rOS.h"

#include "rError.h"
#include "rIODataType.h"
#include "rDataFilter.h"
#include "rFactory.h"
#include "rStrings.h"
#include "rBOOST.h"

BOOST_WRAP_PUSH
#include <boost/asio.hpp>
#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/process.hpp>
// #include <boost/date_time/posix_time/posix_time.hpp>
BOOST_WRAP_POP

#include <string>
#include <iostream>
#include <fstream>

#include <sys/statvfs.h>

using namespace boost::interprocess;
using namespace rapio;

// Cleaner, but will test with all our compiler versions
namespace fs = boost::filesystem;
// namespace pt = boost::posix_time;

namespace {
static bool
isWSL()
{
  // Implement a function to detect if running in WSL
  // You can use any method that reliably detects WSL
  // For example, checking the existence of "/proc/sys/kernel/osrelease" file
  // or checking for specific environment variables.
  // Here's a simple example:
  return std::getenv("WSL_DISTRO_NAME") != nullptr;
}
}

const std::string&
OS::getHostName()
{
  static bool first = true;
  static std::string hostname;

  if (first) {
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::resolver resolver(io_service);
    hostname = boost::asio::ip::host_name();
    first    = false;
  }
  return hostname;
}

std::string
OS::getProcessName(const bool shortname)
{
  static bool first = true;
  static std::string process;
  static std::string processShort;

  if (first) {
    process = boost::dll::program_location().string();
    boost::filesystem::path p(process);
    processShort = p.filename().string();
    first        = false;
  }
  return shortname ? processShort : process;
}

std::string
OS::getProcessPath()
{
  return (boost::dll::program_location().parent_path().string());
}

int
OS::getProcessID()
{
  pid_t pid = getpid();

  return pid;
}

std::string
OS::getCurrentDirectory()
{
  fs::path boostpath(fs::current_path());
  std::string fullcwd = boostpath.string();

  return fullcwd;
}

bool
OS::isDirectory(const std::string& path)
{
  bool isDir = false;

  try {
    fs::file_status s = fs::status(path);
    isDir = fs::is_directory(s);
  }
  catch (const fs::filesystem_error &e)
  {
    // Just assume it's not a directory then
  }
  return isDir;
}

bool
OS::ensureDirectory(const std::string& dirpath)
{
  // Directory must exist. We need getDirName to get added subdirs
  // const std::string dir(URL(filepath).getDirName()); // isn't this dirpath??
  if (!isDirectory(dirpath) && !mkdirp(dirpath)) {
    LogSevere("Unable to create directory: " << dirpath << "\n");
    return false;
  }
  return true;
}

bool
OS::isRegularFile(const std::string& path)
{
  bool isRegularFile = false;

  try {
    fs::file_status status = fs::status(path);
    isRegularFile = fs::is_regular_file(status);
  }
  catch (const fs::filesystem_error &e)
  {
    // Just assume it's not a regular file then
  }
  return isRegularFile;
}

std::string
OS::canonical(const std::string& path)
{
  try {
    auto p = fs::canonical(path);
    return p.string();
  }
  catch (const fs::filesystem_error &e)
  {
    LogSevere("Couldn't convert it! " << e.what() << "\n");
    // Just return original, can't do anything with it
    return path;
  }
}

std::string
OS::validatePathCharacters(const std::string& path)
{
  // Get rid of spaces
  // FIXME: Filter anything else that can break a path.
  std::string pstart = path;

  Strings::replace(pstart, " ", "_");
  Strings::replace(pstart, "//", "/");

  return pstart;
}

bool
OS::mkdirp(const std::string& path)
{
  bool haveIt = false;

  try {
    fs::file_status s = fs::status(path);
    const bool isDir  = fs::is_directory(s);
    if (isDir) { // boost create is true only if directories were newly created
      haveIt = true;
    } else {
      haveIt = fs::create_directories(path);
    }
  }
  catch (const fs::filesystem_error &e) {
    haveIt = false;
  }
  return haveIt;
}

std::string
OS::getUniqueTemporaryFile(const std::string& base_in)
{
  // The '/' here is platform dependent to linux
  auto UNIQUE = fs::unique_path();
  auto TMP    = fs::temp_directory_path();
  auto full   = TMP.native() + "/" + base_in + UNIQUE.native();

  return full;
}

std::string
OS::getRootFileExtension(const std::string& path, std::string& root)
{
  std::string p = path; // test.xMl.Gz  or test.xMl
  boost::filesystem::path bpath(path);
  std::string e = bpath.extension().string(); // .Gz or .xMl

  Strings::removeSuffix(p, e); // root = test.xMl or test

  // Now lowercase e and check the datafilter for it
  Strings::toLower(e);           // .gz or empty
  Strings::removePrefix(e, "."); // gz
  std::shared_ptr<DataFilter> f = Factory<DataFilter>::get(e, "OS");

  // Here if no factory found, we have root='test' and e='xml'
  if (f != nullptr) { // found gz or another we auto handle
    // p=test.xMl and e = gz
    // Second extension, for example .xml.gz --> 'xml' case
    boost::filesystem::path bp(p);
    e = bp.extension().string();   // test.xMl --> .xMl
    Strings::removeSuffix(p, e);   // root = test
    Strings::toLower(e);           // .xml
    Strings::removePrefix(e, "."); // xml
  }
  root = p;
  return (e);
}

std::string
OS::getRootFileExtension(const std::string& path)
{
  std::string p, e;

  e = OS::getRootFileExtension(path, p);
  return (e);
}

std::string
OS::validateExe(const std::string& path)
{
  try {
    // BOOST search won't handle folder paths with /, so
    // we will only search if no / included.
    fs::path boostpath;

    if (path.find("/") != std::string::npos) {
      // Try absolute path match
      boostpath = fs::absolute(path);
    } else {
      // Try relative path search
      boostpath = boost::process::search_path(path);
    }
    if (boostpath.empty()) {
      return "";
    }

    // Check if regular file and executable
    const auto s = fs::status(boostpath);
    if (fs::is_regular_file(s)) {
      const auto p   = s.permissions();
      bool isExecute = ((p & fs::perms::owner_exe) != fs::perms::no_perms);
      if (isExecute) {
        return (boostpath.string());
      }
    }
  }catch (const fs::filesystem_error &e)
  {
    // Just assume it's not executable then
  }

  return "";
} // OS::validateExe

std::string
OS::findValidExe(const std::vector<std::string>& list)
{
  std::string binaryPath = "";

  for (auto p:list) {
    const auto search = OS::validateExe(p);
    if (!search.empty()) {
      binaryPath = search;
      break;
    }
  }
  return binaryPath;
}

int
OS::runProcess(const std::string& commandin, std::vector<std::string>& data)
{
  data.clear();
  std::string error = "None";

  try{
    namespace p = boost::process;

    // Properly break up command and args vector for boost child
    std::string command;
    std::vector<std::string> args;
    Strings::splitWithoutEnds(commandin, ' ', &args);
    // for(size_t i=0; i< args.size(); i++){
    //  LogDebug(i << " --> " << args[i] << "\n");
    // }
    if (args.size() < 1) { return -1; }

    if (args.size() > 0) {
      command = args[0];
      args.erase(args.begin());
    }

    // Check can find executable, because otherwise BOOST outright segfaults if
    // missing, which we really don't want in a realtime/recovering algorithm.
    // We can handle absolute and relative and we check if it is executable or not
    const std::string absolutePath = validateExe(command);

    //  const auto spath = p::search_path(command);
    if (absolutePath.empty()) {
      error = "Command not found or not executable: '" + command + "'";
    } else {
      // Have child send std_out and std_err to a stream for us. We're assuming
      // output is wanted.  FIXME: we could add a flag for this maybe?
      const auto spath = fs::absolute(absolutePath);
      p::ipstream is;
      p::child c(spath, p::args(args), (p::std_out & p::std_err) > is);

      // Call the child synchronously, capturing std_out and std_err
      std::string line;

      while (c.running() && std::getline(is, line) && !line.empty()) {
        data.push_back(line);
      }

      c.wait();

      // int childError = c.native_exit_code();
      int childError = c.exit_code();

      LogInfo("(" << childError << ") Running command '" << commandin << "'\n");
      return childError;
    }
  }catch (const std::exception& e)
  // Catch ALL exceptions deliberately and recover
  // since we want our algorithms to continue running
  {
    error = e.what();
  }
  // Failure
  LogSevere("Failure running command '" << commandin << "' Error: '" << error << "'\n");
  return -1;
} // OS::runProcess

std::vector<std::string>
OS::runDataProcess(const std::string& command, std::shared_ptr<DataGrid> datagrid)
{
  // FIXME: Rework/merge code properly with python module and write/filter ability
  auto pid = getProcessID();

  std::string jsonName = std::to_string(pid) + "-JSON";
  std::vector<std::string> arrayNames;
  std::vector<std::string> output;

  try{
    // ----------------------------------------------------
    // Write JSON out to shared for data process/python
    std::shared_ptr<PTreeData> theJson = datagrid->createMetadata();
    std::vector<char> buf; // FIXME: Buffer class instead?
    size_t aLength = IODataType::writeBuffer(theJson, buf, "json");
    if (aLength < 2) { // Check for empty buffer (buffer always ends with 0)
      LogSevere("DataGrid didn't generate JSON so aborting python call.\n");
      return std::vector<std::string>();
    }
    aLength -= 1; // Remove the ending buffer 0
    shared_memory_object shdmem2 { open_or_create, jsonName.c_str(), read_write };
    shdmem2.truncate(aLength);
    mapped_region region3 { shdmem2, read_write }; // read only, read_write?
    char * at2 = static_cast<char *>(region3.get_address());
    memcpy(at2, &buf[0], aLength);

    // ----------------------------------------------------
    // Write the arrays to shared memory
    // FIXME: We write the primary data array for now, if any.
    // FIXME: generalize by looping and handle the data TYPE such as float, int
    auto theDims = datagrid->getDims();
    auto dataptr = datagrid->getFloat2D(Constants::PrimaryDataName);
    if (dataptr == nullptr) {
      LogSevere("Python call only allowed on 2D LatLonGrid and RadialSet at moment :(\n");
      return std::vector<std::string>();
    }
    auto x       = theDims[0].size();
    auto y       = theDims[1].size();
    auto size    = x * y;                // actual data
    auto memsize = size * sizeof(float); // a lot bigger.  FIXME: This depends on type of data!
    auto& ref2   = dataptr->ref();
    // ref2[0][0] = 999.0;
    // ref2[1][1] = -99000.0;

    std::string key = std::to_string(pid) + "-array" + std::to_string(1);
    arrayNames.push_back(key);
    shared_memory_object shdmem { open_or_create, key.c_str(), read_write };
    shdmem.truncate(memsize);
    // shdmem.get_name()
    // shdmem.get_size()
    mapped_region region2 { shdmem, read_write }; // read only, read_write?
    float * at = static_cast<float *>(region2.get_address());
    memcpy(at, ref2.data(), size * sizeof(float));

    // ----------------------------------------------------
    // Call the python helper.
    runProcess(command, output);

    // Do we need to copy back?  Aren't we mapped to this?
    memcpy(ref2.data(), at, size * sizeof(float));
  }catch (const std::exception& e) {
    LogSevere("Failed to execute command " << command << "\n");
  }

  // Clean up all shared memory objects...
  // The boost object not supporting RAII?
  shared_memory_object::remove(jsonName.c_str());
  for (size_t i = 0; i < arrayNames.size(); i++) {
    std::string key = std::to_string(pid) + "-array" + std::to_string(i + 1);
    shared_memory_object::remove(key.c_str());
  }
  return output;
} // OS::runDataProcess

void
OS::getProcessSizeKB(double& vm_usage, double& resident_set)
{
  // https://stackoverflow.com/questions/669438/how-to-get-memory-usage-at-runtime-using-c
  // Credited to Don Wakefield
  vm_usage     = 0.0;
  resident_set = 0.0;
  std::ifstream stat_stream("/proc/self/stat", std::ios_base::in);

  // Fields we might want later actually...
  std::string pid, comm, state, ppid, pgrp, session, tty_nr,
    tpgid, flags, minflt, cminflt, majflt, cmajflt,
    utime, stime, cutime, cstime, priority, nice, O, itrealvalue, starttime;

  unsigned long vsize;
  long rss;

  stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
  >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
  >> utime >> stime >> cutime >> cstime >> priority >> nice
  >> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest
  stat_stream.close();

  long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages

  vm_usage     = vsize / 1024.0;
  resident_set = rss * page_size_kb;
}

namespace {
// Shared copy_file for different BOOST versions
void
os_copy_file(const fs::path& fromPath, const fs::path& toPath)
{
  #if BOOST_VERSION >= 107400
  // Use copy_options for Boost 1.74.0 and newer
  // https://www.boost.org/doc/libs/1_75_0/libs/filesystem/doc/release_history.html
  fs::copy_file(fromPath, toPath, fs::copy_options::overwrite_existing);
  #else
  fs::copy_file(fromPath, toPath, fs::copy_option::overwrite_if_exists);
  #endif
}
}
bool
OS::copyFile(const std::string& from, const std::string& to)
{
  bool ok = false;

  try {
    const fs::path fromPath(from);
    const fs::path toPath(to);

    // If from doesn't exist, give error and return
    const fs::path parentFrom = fromPath.parent_path();
    if (!fs::exists(fromPath)) {
      LogSevere(from << " does not exist to move.\n");
      return false;
    }

    // Make sure path exists at destination
    const fs::path parentTo = toPath.parent_path();
    if (!OS::ensureDirectory(parentTo.native())) {
      return false;
    }

    // Always copy to a TMP and then rename (atomic for readers)
    const std::string to2 = to + "_TMP";
    const fs::path to2Path(to2);
    os_copy_file(fromPath, to2Path);
    fs::rename(to2Path, toPath);
    ok = true;
  }catch (const fs::filesystem_error &e)
  {
    LogSevere("Failed to copy " << from << " to " << to << ": " << e.what() << "\n");
  }
  return ok;
} // OS::copyFile

bool
OS::moveFile(const std::string& from, const std::string& to, bool quiet)
{
  bool ok = false;

  const fs::path fromPath(from);
  const fs::path toPath(to);

  // If from doesn't exist, give error and return
  if (!fs::exists(fromPath)) {
    if (!quiet) {
      LogSevere(from << " does not exist to move.\n");
    }
    return false;
  }

  // Make sure path exists at destination
  const fs::path parentTo = toPath.parent_path();

  if (!OS::ensureDirectory(parentTo.native())) {
    return false;
  }

  try {
    // Rename will fail between devices. Tempted to stat compare devices..
    // since 'maybe' it would be faster.
    fs::rename(fromPath, toPath);
    ok = true;
  }catch (const fs::filesystem_error &e)
  {
    try {
      const std::string to2 = to + "_TMP";
      const fs::path to2Path(to2);
      os_copy_file(fromPath, to2Path);
      fs::rename(to2Path, toPath);

      fs::remove(fromPath); // remove original file
      ok = true;
    }catch (const fs::filesystem_error &e)
    {
      if (!quiet) {
        LogSevere("Failed to move/copy " << from << " to " << to << ": " << e.what() << "\n");
      }
    }
  }
  return ok;
} // OS::moveFile

bool
OS::deleteFile(const std::string& file)
{
  bool ok = false;

  try {
    fs::remove(file);
    ok = true;
  }
  catch (const fs::filesystem_error &e)
  { }
  return ok;
}

size_t
OS::getFileSize(const std::string& filepath)
{
  size_t aSize = 0;

  try {
    fs::path path(filepath);
    if (fs::exists(path)) {
      aSize = fs::file_size(path);
    }
  }
  catch (const fs::filesystem_error &e)
  { }
  return aSize;
}

bool
OS::getFileModificationTime(const std::string& filepath, Time& aTime)
{
  try {
    fs::path path(filepath);
    if (fs::exists(path)) {
      time_t lastWriteTime = fs::last_write_time(path);
      aTime = Time::SecondsSinceEpoch(lastWriteTime, 0);
      return true;
    }
  }
  catch (const fs::filesystem_error &e)
  { }

  return false;
}

std::string
OS::getEnvVar(const std::string& name)
{
  std::string ret;
  const char * envPath = ::getenv(name.c_str());

  if (envPath) {
    ret = envPath;
  } else {
    ret = "";
  }
  return (ret);
}

void
OS::setEnvVar(const std::string& envVarName, const std::string& value)
{
  setenv(envVarName.c_str(), value.c_str(), 1);
  LogDebug(envVarName << " = " << value << "\n");
}

bool
OS::isContainer()
{
  static bool check       = true;
  static bool isContainer = false;

  if (check) {
    // Various checks.  This doesn't seem to be 100% standard anywhere,
    // even AI didn't give a good answer, lol.

    // If you are designing a new container you can explicitly mark it with a file:
    if (isRegularFile("/etc/container.txt")) {
      isContainer = true;

      // Otherwise some various checks, feel free to add more
    } else if (getEnvVar("container") == "oci") { // redhat/fedora do this it seems
      isContainer = true;
    } else if (isRegularFile("/run/.containerenv")) { // podman
      isContainer = true;
    } else if (isRegularFile("/run/.dockerenv")) { // docker
      isContainer = true;
    }
    if (isContainer) {
      LogInfo("You appear to be running on a container.");
    }
    check = false;
  }
  return isContainer;
}

bool
OS::isWSL()
{
  // Implement a function to detect if running in WSL
  // You can use any method that reliably detects WSL
  // For example, checking the existence of "/proc/sys/kernel/osrelease" file
  // or checking for specific environment variables.
  // Here's a simple example:
  return std::getenv("WSL_DISTRO_NAME") != nullptr;
}

bool
OS::runCommandOnFile(const std::string& postCommandIn, const std::string& finalFile, bool captureOut)
{
  // Run a command with macro for %filename%.  Could generalize even more I think.
  // This is called by postwrite option for data files, and postfml for the fml notifier
  // though it is becoming more generalized

  if (!postCommandIn.empty()) {
    std::string postCommand = postCommandIn;
    // ----------------------------------------------------------------
    // Macros.  Could be in configuration file?  This is our silly standard ldm insert
    //
    if (postCommandIn == "ldm") {
      postCommand = "pqinsert -v -f EXP %filename%"; // stock ldm and in path
    }

    // ----------------------------------------------------------------
    // Key substitution. Usually, things like final filename would be important
    // for post success
    Strings::replace(postCommand, "%filename%", finalFile);

    // ----------------------------------------------------------------
    // Run command and log the output
    //
    std::vector<std::string> output;
    bool successful = (OS::runProcess(postCommand, output) != -1);
    if (captureOut) {
      if (successful) {
        for (auto& i: output) {
          LogInfo("   " << i << "\n");
        }
      }
    }
    return successful;
  }
  return true;
} // OS::runCommandOnFile
