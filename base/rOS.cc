#include "rOS.h"

#include "rError.h"

#include <string>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>

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
  }catch (std::exception e)
  // Catch ALL exceptions deliberately and recover
  // since we want our algorithms to continue running
  {
    // std::cerr << "Error caught " << e.what() << "\n";
  }
  // Failure
  LogSevere("Failure running command '" << command << "'\n");
  return std::vector<std::string>();
} // OS::runProcess
