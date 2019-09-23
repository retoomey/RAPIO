#include "rOS.h"

#include "rError.h"

#include <string>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/dll.hpp>
#include <boost/filesystem.hpp>

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
