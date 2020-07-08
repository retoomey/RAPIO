#include "rOS.h"

#include "rError.h"
#include "rIOJSON.h"

#include <string>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/process.hpp>

using namespace boost::interprocess;
using namespace rapio;

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

int
OS::getProcessID()
{
  pid_t pid = getpid();

  return pid;
}

std::string
OS::getCurrentDirectory()
{
  boost::filesystem::path boostpath(boost::filesystem::current_path());
  std::string fullcwd = boostpath.string();
  return fullcwd;
}

bool
OS::isDirectory(const std::string& path)
{
  bool isDir = false;

  try {
    boost::filesystem::file_status s = boost::filesystem::status(path);
    isDir = boost::filesystem::is_directory(s);
  }
  catch (boost::filesystem::filesystem_error &e)
  {
    // Just assume it's not a directory then
  }
  return isDir;
}

bool
OS::mkdirp(const std::string& path)
{
  bool haveIt = false;

  try {
    boost::filesystem::file_status s = boost::filesystem::status(path);
    const bool isDir = boost::filesystem::is_directory(s);
    if (isDir) { // boost create is true only if directories were newly created
      haveIt = true;
    } else {
      haveIt = boost::filesystem::create_directories(path);
    }
  }
  catch (boost::filesystem::filesystem_error &e) {
    haveIt = false;
  }
  return haveIt;
}

std::string
OS::getUniqueTemporaryFile(const std::string& base_in)
{
  // FIXME: what's the best way to do this in boost?
  // The '/' here is platform dependent
  auto UNIQUE = boost::filesystem::unique_path();
  auto TMP    = boost::filesystem::temp_directory_path();
  auto full   = TMP.native() + "/" + base_in + UNIQUE.native();

  return full;
}

std::string
OS::getFileExtension(const std::string& path)
{
  return (boost::filesystem::extension(path));
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
  }catch (std::exception& e)
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
  // FIXME: can we create boost arrays as shared to begin with?
  // and thus avoid copies? Maybe
  auto pid = getProcessID();

  std::string jsonName = std::to_string(pid) + "-JSON";
  std::vector<std::string> arrayNames;
  std::vector<std::string> output;

  try{
    // ----------------------------------------------------
    // Write JSON out to shared for data process/python
    // We're not allowing write to attributes at moment.  FIXME?
    std::shared_ptr<JSONData> theJson = IOJSON::createJSON(datagrid);
    std::vector<char> buf; // FIXME: Buffer class instead?
    size_t aLength = theJson->writeBuffer(buf);
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
    auto dataptr = datagrid->getFloat2D("primary");
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

    std::string key = std::to_string(pid) + "-array" + std::to_string(1); // FIXME: loop
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
  }catch (std::exception& e) {
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
