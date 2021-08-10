#message(FATAL_ERROR "I'm currently migrating to cmake from autotools, use autogen.sh --prefix=/install/path to invoke autotool build")

cmake_minimum_required (VERSION 3.10)

# I want absolute library paths if I'm building with a custom prefix/third party so
# that I don't grab the system libraries by accident
# Note if building with all system third party this only hurts if you want to move the
# binary around.
cmake_policy(SET CMP0060 NEW)
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# Set the project info
project (RAPIO 
    DESCRIPTION "RAPIO Real time algorithm parameter and IO"
    HOMEPAGE_URL "https://github.com/retoomey/RAPIO"
    LANGUAGES CXX C
    VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Directories
add_subdirectory(base)
add_subdirectory(rexample)

# Tests are conditional
add_subdirectory(tests EXCLUDE_FROM_ALL)
