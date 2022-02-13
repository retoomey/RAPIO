#include "rOS.h"

#include "rError.h"
#include "rIODataType.h"
#include "rDataFilter.h"
#include "rFactory.h"
#include "rStrings.h"

#include <string>
#include <iostream>
#include <fstream>

#include <boost/asio.hpp>
#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/process.hpp>

using namespace boost::interprocess;
using namespace rapio;

// Cleaner, but will test with all our compiler versions
namespace fs = boost::filesystem;

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
OS::getProcessName()
{
  static bool first = true;
  static std::string process;

  if (first) {
    process = boost::dll::program_location().string();
    first   = false;
  }
  return process;
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
OS::getRootFileExtension(const std::string& path)
{
  // We want to auto remove the compression field
  // .xml.gz --> 'xml' .xml --> 'xml'
  std::string e = fs::extension(path);

  Strings::toLower(e);
  Strings::removePrefix(e, ".");
  std::shared_ptr<DataFilter> f = Factory<DataFilter>::get(e, "OS");

  if (f != nullptr) {
    std::string p = path;
    Strings::removeSuffix(p, "." + e);
    e = fs::extension(p);
    Strings::toLower(e);
    Strings::removePrefix(e, ".");
  }
  return (e);
}

std::string
OS::validateExe(const std::string& path)
{
  try {
    const auto boostpath = fs::absolute(path);
    const auto s         = fs::status(boostpath);

    // Check if regular file and executable
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
}

std::vector<std::string>
OS::runProcess(const std::string& command)
{
  std::vector<std::string> data;

  try{
    namespace p = boost::process;
    p::ipstream is;
    // p::child c("gcc --version", p::std_out > pipe_stream);
    p::child c(command, p::std_out > is);

    std::string line;

    // while (pipe_stream && std::getline(pipe_stream, line) && !line.empty()){
    while (c.running() && std::getline(is, line) && !line.empty()) {
      //  std::cerr << line << std::endl;
      // line += "\n";
      // LogSevere(line);
      data.push_back(line);
    }

    c.wait();

    LogInfo("Success running command '" << command << "'\n");
    return data;
  }catch (const std::exception& e)
  // Catch ALL exceptions deliberately and recover
  // since we want our algorithms to continue running
  {
    // std::cerr << "Error caught " << e.what() << "\n";
  }
  // Failure
  LogSevere("Failure running command '" << command << "'\n");
  return std::vector<std::string>();
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
    output = runProcess(command);

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
OS::getProcessSize(double& vm_usage, double& resident_set)
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

bool
OS::moveFile(const std::string& from, const std::string& to)
{
  // On same file system, rename is fine, on two different you need to copy instead.
  // Would it be quicker to check if from/to is same filesystem?
  bool ok = false;

  try {
    fs::rename(from, to);
    ok = true;
  }
  catch (const fs::filesystem_error &e)
  {
    // Error might be cross file system reason, try a copy now
    try {
      fs::copy_file(from, to, fs::copy_option::overwrite_if_exists);
      fs::remove(from);
      ok = true;
    }catch (const fs::filesystem_error &e)
    {
      LogSevere("Failed to move/copy " << from << " to " << to << ": " << e.what() << "\n");
    }
  }
  return ok;
}

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
