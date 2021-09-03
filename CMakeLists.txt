# Toomey Aug 2021
# Top cmake for RAPIO building
cmake_minimum_required (VERSION 3.10)

##################################################################################################
# Set the project info
project (RAPIO 
    DESCRIPTION "RAPIO Real time algorithm parameter and IO"
    HOMEPAGE_URL "https://github.com/retoomey/RAPIO"
    LANGUAGES CXX C
    VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Our package modules
set(CMAKE_MODULE_PATH cmake)

# My hack for a container system build.  I specify /usr in order
# to install using system libraries and path.
if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
  set(RAPIO_INSTALL_LIB "/usr/lib64") # FIXME whatever is cmake default system lib
else()
  # I want some absolute library paths if I'm building with a custom prefix/third party so
  # that I don't grab the system libraries by accident
  # Note if building with all system third party this only hurts if you want to move the
  # binary around.  We typically have several different builds on systems with different
  # versions of supporting libraries that have to be a particular version.  We don't
  # want these to change if IT patches a system, etc.
  cmake_policy(SET CMP0060 NEW)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

  set(RAPIO_INSTALL_LIB "${CMAKE_INSTALL_PREFIX}/lib")

  # Give the third party build folder search location to my packages
  # If you replace the FindNetcdf say then force the include/lib here
  # like BOOST below
  set(CUSTOM_THIRDDIR "${CMAKE_INSTALL_PREFIX}")

  # Use the ZLIB built in module and tell it to hunt local
  set(ZLIB_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include")
  cmake_policy(SET CMP0074 OLD)
  set(ZLIB_ROOT "${CMAKE_INSTALL_PREFIX}/lib")

  # Use Jasper built in module
  set(JASPER_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")
  set(JASPER_LIBRARY_RELEASE "${CMAKE_INSTALL_PREFIX}/lib")

  # BOOST we tell directly since we got a common cmake module
  set(BOOST_ROOT "${CMAKE_INSTALL_PREFIX}/boost")
  set(BOOST_INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/boost/include")
  set(BOOST_LIBRARYDIR "${CMAKE_INSTALL_PREFIX}/boost/lib")
endif()


###########################################################################
# Note my custom cmake finders which work for my setup
find_package(UDUnits2 REQUIRED)
find_package(Proj REQUIRED)
find_package(Netcdf REQUIRED)
find_package(Magick)
# There's a cmake one but it doesn't seem to allow me to force a library path
find_package(CURL REQUIRED)   # FIXME:  I 'might' go pure boost ASIO at some point here
###########################################################################


###########################################################################
# Cmake provided modules.  Either I've copied to cmake folder or system
# cmake default.  I think some of these are only installed in the latest
# OS systems.
find_package(Boost 1.71 REQUIRED COMPONENTS
  system log log_setup thread filesystem serialization iostreams timer
)
# The FindJasper module is giving me paths and semicolons not the actual
# jasper.  Either it's bugged or I'm still learning cmake
find_package(Jasper REQUIRED)
if (JASPER_FOUND)
  if (NOT "${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
    set(JASPER_LIBRARIES "${JASPER_LIBRARY}/libjasper.so")
  endif()
endif()
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(BZip2 REQUIRED)
find_package(Threads REQUIRED)
find_package(GDAL REQUIRED)
###########################################################################

# Directories
add_subdirectory(base)
add_subdirectory(rexample)

# Tests are conditional
add_subdirectory(tests EXCLUDE_FROM_ALL)