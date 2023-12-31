# Toomey Aug 2021
# Top cmake for RAPIO building
cmake_minimum_required (VERSION 3.10)

# Set type of build.  This has effect on different os it seems based on some settings here:
# CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel") google these
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type selected, defaulting to 'Release'")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (default Release)")
endif()

# My force of clang compiler for me, not doing it for everyone though
#set(CMAKE_C_COMPILER "/usr/bin/clang")
#set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
#set(CMAKE_VERBOSE_MAKEFILE)

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

###########################################################################
# Convenient add to a RAPIO note list
# I find a big list at the end more useful than lots of messages scrolling by
#
function(addRAPIONote note)
  # We could call message instead or extra to 'inline' instead
  # message(STATUS "${note}")
  get_property(FINAL_RAPIO_MESSAGE GLOBAL PROPERTY FINAL_RAPIO_MESSAGE)
  list(APPEND FINAL_RAPIO_MESSAGE "${note}")
  set_property(GLOBAL PROPERTY FINAL_RAPIO_MESSAGE "${FINAL_RAPIO_MESSAGE}")
endfunction()
  
# Generate a config.h (note this goes into the build so it's unique per build)
get_filename_component(SOURCE_PATH "${CMAKE_SOURCE_DIR}" ABSOLUTE)
get_filename_component(BUILD_PATH "${CMAKE_CURRENT_BINARY_DIR}" ABSOLUTE)
string(TIMESTAMP BUILD_DATE "%Y-%m-%d")
string(TIMESTAMP BUILD_TIME "%H:%M:%S" UTC)
execute_process(COMMAND hostname OUTPUT_VARIABLE MACHINE_NAME OUTPUT_STRIP_TRAILING_WHITESPACE)
configure_file(base/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/base/config.h @ONLY)

# Generate a RAPIO.h header file containing headers in base
# Note, not GLOB_RECURSE since we don't want plugin headers here.
set(PUBLIC_HEADER_BASE ${CMAKE_CURRENT_SOURCE_DIR}/base/)
file(GLOB PUBLIC_HEADERS_LIST RELATIVE ${PUBLIC_HEADER_BASE} ${PUBLIC_HEADER_BASE}*.h)
foreach(filename ${PUBLIC_HEADERS_LIST})
  set (PUBLIC_HEADERS "${PUBLIC_HEADERS}#include \"${filename}\"\n")
endforeach()
#string(REPLACE ";" " " PUBLIC_HEADERS_STR "%{PUBLIC_HEADERS}")
configure_file(base/RAPIO.h.in ${CMAKE_CURRENT_BINARY_DIR}/base/RAPIO.h @ONLY)

# Our package modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

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
  cmake_policy(SET CMP0074 NEW)
  set(ZLIB_ROOT "${CMAKE_INSTALL_PREFIX}/lib")

  # Use Jasper built in module
  #set(JASPER_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include;/usr/include")
  #set(JASPER_LIBRARY_RELEASE "${CMAKE_INSTALL_PREFIX}/lib;/usr/lib64")

  # BOOST we tell directly since we got a common cmake module
  set(BOOST_ROOT "${CMAKE_INSTALL_PREFIX}/boost")
  set(BOOST_INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/boost/include")
  set(BOOST_LIBRARYDIR "${CMAKE_INSTALL_PREFIX}/boost/lib")
endif()

###########################################################################
# We don't have a chance without zlib, bzip2 and boost.  NSSL data
# and our code relies on it.
#
# BOOST will dynamically fail on compression reading/writing these. So
# check these first before anything else.
# The question is if the installed apk/rpm/etc versions
# of boost support the XZ, ZLIB, BZIP2 compressions (did they build with them?)
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)

# Pretty sure boost links us to Threads::Threads, but ensure pthreads first
set(THREADS_PREFER_PTHREAD_FLAG ON)
#find_package(Threads REQUIRED) target_link_libraries(binary Threads::Threads)
find_package(Boost 1.66 REQUIRED COMPONENTS
  system log log_setup thread filesystem serialization iostreams timer
)

# Oracle 8 unfortunately will be used at NSSL, which is a 'attempt to be stable'
# variant of Redhat, and is therefore ancient using Boost 1.66
# It appears at first glance that LZMA wasn't built properly in the RPM.
# Is Boost version greater/equal to 1.71, then I'll build the LZMA
# We 'could' do a find_package(LibLZMA OPTIONAL) as well

# Older boost doesn't use the decimal version, so check for "."
# to get correct format
string(FIND "${Boost_VERSION}" "." dotindex)

if (${dotindex} GREATER_EQUAL 0)
  set(TEST_BOOST_VERSION "1.71.0")
else()
  set(TEST_BOOST_VERSION "107700")
endif()

if (${Boost_VERSION} VERSION_GREATER_EQUAL ${TEST_BOOST_VERSION})
  add_definitions(-DHAVE_LZMA)
  addRAPIONote("BOOST ${Boost_VERSION} with LZMA support for .xz and .lzma compressed files.")
else()
  addRAPIONote("BOOST ${Boost_VERSION} < ${TEST_BOOST_VERSION} no LZMA support.  Only matters if you want to read .xz or .lzma compressed files.")
endif()
set(RAPIO_DEFINITIONS
  -DBOOST_LOG_DYN_LINK
)

###########################################################################
# Start with header/linking the default libraries, we'll add more
#

# Includes
set(RAPIO_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/base
  ${CMAKE_INSTALL_PREFIX}/include
  ${ZLIB_INCLUDE_DIRS}
  ${BZip2_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}/base
)

# Libraries
set(RAPIO_THIRD_LIBRARIES
  ${ZLIB_LIBRARIES}
  ${BZip2_LIBRARIES}
  ${Boost_LIBRARIES}
)

###########################################################################
# Convenient add to RAPIO include and third party libraries
#
function(append_RAPIO dir libraries)
  list(FIND RAPIO_INCLUDE_DIRS "${dir}" index)
  if(index EQUAL -1) # Cut down on dups like /usr/include
    if(NOT "${dir}" STREQUAL "") # Don't add empty items
      list(APPEND RAPIO_INCLUDE_DIRS "${dir}")
      set(RAPIO_INCLUDE_DIRS ${RAPIO_INCLUDE_DIRS} PARENT_SCOPE)
    endif()
  endif()
  if(NOT "${libraries}" STREQUAL "") # Don't add empty items
    list(APPEND RAPIO_THIRD_LIBRARIES "${libraries}")
    set(RAPIO_THIRD_LIBRARIES ${RAPIO_THIRD_LIBRARIES} PARENT_SCOPE)
  endif()
endfunction()

###########################################################################
# Used by tile generator and even algorithms like fusion.  Projection
# ability.
find_package(Proj)
if (Proj_FOUND)
  add_definitions(-DHAVE_PROJLIB)
  append_RAPIO("${Proj_INCLUDE_DIR}" "${Proj_LIBRARIES}")
else() 
  addRAPIONote("No PROJ library.  Only matters for grib2 projection currently.")
endif(Proj_FOUND)

# Used by web pulling abilities like webindex reading or remote URL pulling
find_package(CURL) 
if (CURL_FOUND)
  add_definitions(-DHAVE_CURL)
  append_RAPIO("${CURL_INCLUDE_DIR}" "${CURL_LIBRARIES}")
else()
  addRAPIONote("No CURL library.  Will use BOOST::ASIO network library for network URL (experimental).")
endif(CURL_FOUND)

# Used by AWS stuff (optional)
find_package(OpenSSL)
if (OPENSSL_FOUND)
  add_definitions(-DHAVE_OPENSSL)
  append_RAPIO("${OPENSSL_INCLUDE_DIR}" "${OPENSSL_LIBRARIES}")
else()
  addRAPIONote("No OpenSLL library. Only matters for rAWS.cc and web AWS stuff (experimental).")
endif(OPENSSL_FOUND)

# Used by the base/rUnits for conversion, but we don't convert right now anyway
find_package(UDUnits2)
if (UDUnits2_FOUND)
  add_definitions(-DHAVE_UDUNITS2)
  append_RAPIO("${UDUnits2_INCLUDE_DIR}" "${UDUnits2_LIBRARIES}")
else()
  addRAPIONote("No UDUNITS2 library. Unit conversion ability disabled.  Not used 'yet'.")
endif(UDUnits2_FOUND)

###########################################################################
# Directories
add_subdirectory(base)

# Examples with option to turn off since it will grow probably
option(BUILD_EXAMPLES "Build example directory rexample" YES)
if (BUILD_EXAMPLES)
  add_subdirectory(rexample)
endif (BUILD_EXAMPLES)

###########################################################################
# Independent module building
#
option(BUILD_NETCDF_MODULE "Build netcdf module" YES)
if (BUILD_NETCDF_MODULE)
  add_subdirectory(base/ionetcdf)
endif (BUILD_NETCDF_MODULE)

option(BUILD_IMAGE_MODULE "Build image (GraphicsMagick) module" YES)
if (BUILD_IMAGE_MODULE)
  add_subdirectory(base/ioimage)
endif (BUILD_IMAGE_MODULE)

option(BUILD_GDAL_MODULE "Build GDAL module" YES)
if (BUILD_GDAL_MODULE)
  add_subdirectory(base/iogdal)
endif (BUILD_GDAL_MODULE)

option(BUILD_GRIB2_MODULE "Build Grib2 module (reading grib2 data)" YES)
option(USE_NCEPLIBS_GRIB2 "Use NCEPLIBS-g2c library instead of system grib2" YES)
if (BUILD_GRIB2_MODULE)

  # Go ahead and die now...otherwise it will die when pulling external project
  find_package(PNG REQUIRED)

  # Ok, so we should use jasper or openjpeg but not both.
  # And jasper is deprecated is seems. Depending on OS you might have issues
  # and have to play with turning on one vs the other.

  # Make sure off until found explicitly below
  set(USE_JASPER_NCEPLIBS NO)
  set(USE_OPENJPEG_NCEPLIBS NO)
  set(JPEG_COMPRESSION_FOUND NO)

  find_package(OpenJPEG QUIET)
  if (OpenJPEG_FOUND)
    set(USE_OPENJPEG_NCEPLIBS YES)
    set(JPEG_COMPRESSION_FOUND YES)
    message(STATUS "Found OpenJPEG2 for NCEPGRIB2.")
  else()
    set(USE_OPENJPEG_NCEPLIBS NO)

    # Ok try to fall back to jasper
    find_package(Jasper QUIET)
    if (Jasper_FOUND)
      set(USE_JASPER_NCEPLIBS YES)
      set(JPEG_COMPRESSION_FOUND YES)
      message(STATUS "Found Jasper for NCEPGRIB2 (if build fails you can install OpenJpeg2).")
    else()
      set(USE_JASPER_NCEPLIBS NO)
    endif()
  endif()

  # Manual override if needed, only use one:
  #set(USE_OPENJPEG_NCEPLIBS YES)
  #set(USE_JASPER_NCEPLIBS NO)

  if (NOT JPEG_COMPRESSION_FOUND)
    addRAPIONote("No OpenJPEG2 or Jasper library.  A LOT of grib2 has JPEG compression so these files may fail to read.")
  endif()
  
  add_subdirectory(base/iogrib)
endif (BUILD_GRIB2_MODULE)

option(BUILD_TEXT_MODULE "Build Text module (writing text DataTypes)" YES)
if (BUILD_TEXT_MODULE)
  add_subdirectory(base/iotext)
endif (BUILD_TEXT_MODULE)

# These two currently don't require any extra libraries, so we'll just
# build them.  Though python might at some point
add_subdirectory(base/iopython)
add_subdirectory(base/ioraw)

option(BUILD_HMRG_MODULE "Build HMRG module (reading MRMS binary grids)" YES)
if (BUILD_HMRG_MODULE)
  add_subdirectory(base/iohmrg)
endif (BUILD_HMRG_MODULE)

# Build subprograms.
add_subdirectory(programs)

# Tests are conditional.  We 'could' make a flag...here we just have to manual
# go into directory and make.  Not sure what the cmake community does yet
add_subdirectory(tests EXCLUDE_FROM_ALL)

# Dump final notes
get_property(FINAL_RAPIO_MESSAGE GLOBAL PROPERTY FINAL_RAPIO_MESSAGE)
if (FINAL_RAPIO_MESSAGE)
  message(STATUS "-----------------------------------------------")
  message(STATUS "Final Notes on your Configuration:")
  foreach(message_line IN LISTS FINAL_RAPIO_MESSAGE)
    message(STATUS "    -${message_line}")
  endforeach()
  message(STATUS "-----------------------------------------------")
endif()
