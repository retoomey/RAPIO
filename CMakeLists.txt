# Toomey Aug 2021
# Top cmake for RAPIO building
cmake_minimum_required (VERSION 3.10)

###########################################################################
# Options
# Can use "ccmake ." in the BUILD directory to toggle, sometimes it's
# convenient to just modify things here
#
# Some things like .gz support and netcdf we're dead in the water at NSSL without
# these, though in theory you don't strictly require them.  Setting no here
# will increase compile chances.
option(BUILD_MRMS "Force libraries for MRMS NSSL environment" YES)

# Webserver stuff are conditional for moment. This is my imgui/emscripten
# experiment alpha
# Since it uses a cross compiler (em++) we make it an external project
# to avoid stomping on us.
# Building this oracle 9.  I'm sure on other OS it will need a ton of work
# Turn on if you love experiments.
#
option(BUILD_WEB_MODULE "Build Experimental IMGU/EMSCRIPTEN Display" NO)
# Examples with option to turn off since it will grow probably
option(BUILD_EXAMPLES "Build example directory rexample" YES)

# These two for NSSL common data files
option(BUILD_NETCDF_MODULE "Build netcdf module" YES)
option(BUILD_HDF5_MODULE "Build general hdf5 module" YES)
option(BUILD_HMRG_MODULE "Build HMRG module (reading MRMS binary grids)" YES)

option(BUILD_IMAGE_MODULE "Build image (GraphicsMagick) module" YES)

option(BUILD_GDAL_MODULE "Build GDAL module" YES)
option(BUILD_GRIB2_MODULE "Build Grib2 module (reading grib2 data)" YES)
option(USE_NCEPLIBS_GRIB2 "Use NCEPLIBS-g2c library instead of system grib2" YES)
option(BUILD_TEXT_MODULE "Build Text module (writing text DataTypes)" YES)

###########################################################################

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

#add_compile_options(-ftime-report)  # For GCC, shows time per compilation unit

##################################################################################################
# Set the project info
project (RAPIO 
    DESCRIPTION "RAPIO Real time algorithm parameter and IO"
    HOMEPAGE_URL "https://github.com/retoomey/RAPIO"
    LANGUAGES CXX C
    VERSION 1.0)

# Specify the C++ standard
# Currently tested from 11 to 23. Will avoid new features for now
# for legacy compatibility.
set(CMAKE_CXX_STANDARD 11)
# Allow it to go higher to compiler default
set(CMAKE_CXX_STANDARD_REQUIRED False)

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
  
###########################################################################
# A python script install function that strips the .py and installs in bin
#
function(install_python SCRIPT SOURCE_DIR)
    # Remove the file extension
    string(REGEX REPLACE "\\.py$" "" SCRIPT_NAME ${SCRIPT})

    # Copy the script from the source directory to the destination directory
    configure_file(${SOURCE_DIR}/${SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/${SCRIPT_NAME} COPYONLY)

    # Set permissions and install the script
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SCRIPT_NAME}
            PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                        GROUP_EXECUTE GROUP_READ
                        WORLD_EXECUTE WORLD_READ
			DESTINATION bin)
endfunction()

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

# Function for generating the global RAPIO.h list
# We flatten the directories into RAPIO.h and add to RAPIO_INCLUDE_DIRS
function(append_RAPIO_HEADERS FOLDER FILE_LIST)

  # Compute full path to the header directory
  set(PUBLIC_HEADER_BASE "${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER}")
  set(PUBLIC_HEADERS "${PUBLIC_HEADERS}// Headers from ${FOLDER}\n")

  foreach(filename ${FILE_LIST})
    set(PUBLIC_HEADERS "${PUBLIC_HEADERS}#include \"${filename}\"\n")
  endforeach()

  # Expose results to parent scope
  set(PUBLIC_HEADERS ${PUBLIC_HEADERS} PARENT_SCOPE)

  list(APPEND RAPIO_INCLUDE_DIRS "${PUBLIC_HEADER_BASE}")
  set(RAPIO_INCLUDE_DIRS "${RAPIO_INCLUDE_DIRS}" PARENT_SCOPE)

message(STATUS "${RAPIO_INCLUDE_DIRS}")
endfunction()

# Add .h files in a folder to the RAPIO.h and includes
function(append_RAPIO_HEADERS_GLOB_H FOLDER)
  set(PUBLIC_HEADER_BASE "${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER}")
  file(GLOB FILE_LIST RELATIVE ${PUBLIC_HEADER_BASE} ${PUBLIC_HEADER_BASE}*.h)
  append_RAPIO_HEADERS("${FOLDER}" "${FILE_LIST}")
  set(PUBLIC_HEADERS ${PUBLIC_HEADERS} PARENT_SCOPE)
  set(RAPIO_INCLUDE_DIRS "${RAPIO_INCLUDE_DIRS}" PARENT_SCOPE)
endfunction()

# Generate a config.h (note this goes into the build so it's unique per build)
get_filename_component(SOURCE_PATH "${CMAKE_SOURCE_DIR}" ABSOLUTE)
get_filename_component(BUILD_PATH "${CMAKE_CURRENT_BINARY_DIR}" ABSOLUTE)
string(TIMESTAMP BUILD_DATE "%Y-%m-%d")
string(TIMESTAMP BUILD_TIME "%H:%M:%S" UTC)
execute_process(COMMAND hostname OUTPUT_VARIABLE MACHINE_NAME OUTPUT_STRIP_TRAILING_WHITESPACE)
configure_file(base/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/base/config.h @ONLY)

# Define CMAKE_INSTALL_LIBDIR, INCLUDEDIR, etc.
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Our package modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# My hack for a container system build.  I specify /usr in order
# to install using system libraries and path.
if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
  set(RAPIO_INSTALL_LIB "/usr/${CMAKE_INSTALL_LIBDIR}")
else()
  # I want some absolute library paths if I'm building with a custom prefix/third party so
  # that I don't grab the system libraries by accident
  # Note if building with all system third party this only hurts if you want to move the
  # binary around.  We typically have several different builds on systems with different
  # versions of supporting libraries that have to be a particular version.  We don't
  # want these to change if IT patches a system, etc.
  cmake_policy(SET CMP0060 NEW)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

  set(RAPIO_INSTALL_LIB "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

  # Give the third party build folder search location to my packages
  # If you replace the FindNetcdf say then force the include/lib here
  # like BOOST below
  set(CUSTOM_THIRDDIR "${CMAKE_INSTALL_PREFIX}")

  # Use the ZLIB built in module and tell it to hunt local
  set(ZLIB_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include")
  cmake_policy(SET CMP0074 NEW)
  set(ZLIB_ROOT "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

  # Use Jasper built in module
  #set(JASPER_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include;/usr/include")
  #set(JASPER_LIBRARY_RELEASE "${CMAKE_INSTALL_PREFIX}/lib;/usr/lib64")

  # BOOST we tell directly since we got a common cmake module
  #  set(Boost_ROOT "${CMAKE_INSTALL_PREFIX}/boost")
  #set(Boost_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/boost/include")
  #set(Boost_LIBRARY_DIR "${CMAKE_INSTALL_PREFIX}/boost/lib")
endif()

# Pretty sure boost links us to Threads::Threads, but ensure pthreads first
set(THREADS_PREFER_PTHREAD_FLAG ON)
#find_package(Threads REQUIRED) target_link_libraries(binary Threads::Threads)
find_package(Boost 1.66 REQUIRED COMPONENTS
  system log log_setup thread filesystem serialization iostreams timer
)
set(RAPIO_DEFINITIONS
  -DBOOST_LOG_DYN_LINK
)

if (BUILD_MRMS)
  addRAPIONote("Stricter build requirements for MRMS Environment: ENABLED")
else()
  addRAPIONote("Stricter build requirements for MRMS Environment: ***DISABLED***")
endif()

###########################################################################
# Various compression/decompression libraries
#

addRAPIONote("Compression support:")
find_package(ZLIB REQUIRED)
addRAPIONote("      Found: (.z) ZLIB")

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
  addRAPIONote("      Found: (.xz, .lzma) LZMA")
else()
  addRAPIONote("      Missing: (.xz, .lzma) LZMA")
endif()

if (BUILD_MRMS)
  find_package(BZip2 REQUIRED)
else()
  # FIXME: Probably need flags/checks in code.  Have zlib I think in various places.
  find_package(BZip2 QUIET)
endif(BUILD_MRMS)
if (BZip2_FOUND)
  addRAPIONote("      Found: (.bz2) BZIP2")
else()
  addRAPIONote("      Missing: (.bz2) BZIP2")
endif(BZip2_FOUND)

# Snappy in Oracle 9 at least
find_package(Snappy QUIET)
if (Snappy_FOUND)
  add_definitions(-DHAVE_SNAPPY)
  # At least on oracle 9 it's not setting the libraries/headers.  It should 'normally' be
  # just snappy, so we'll do that for now.
  set(Snappy_LIBRARIES "-lsnappy")
  addRAPIONote("      Found: (.sz) Google Snappy ${Snappy_VERSION}")
  append_RAPIO("${Snappy_INCLUDE_DIRS}" "${Snappy_LIBRARIES}")
else()
  addRAPIONote("      Missing: (.sz) Google Snappy")
endif(Snappy_FOUND)

###########################################################################
# Creating RAPIO_INCLUDE_DIRS and RAPIO.h
#
append_RAPIO_HEADERS_GLOB_H("base/")
append_RAPIO_HEADERS_GLOB_H("base/config/")
append_RAPIO_HEADERS_GLOB_H("base/watcher/")
append_RAPIO_HEADERS_GLOB_H("base/index/")
append_RAPIO_HEADERS_GLOB_H("base/notifier/")
append_RAPIO_HEADERS_GLOB_H("base/datatype/")
append_RAPIO_HEADERS_GLOB_H("base/plugin/")
# Bleh.
append_RAPIO_HEADERS("webserver/" "server_http.hpp;asio_compatibility.hpp;mutex.hpp;utility.hpp;status_code.hpp")

append_RAPIO("${ZLIB_INCLUDE_DIRS}" "${ZLIB_LIBRARIES}")
append_RAPIO("${BZip2_INCLUDE_DIRS}" "${BZip2_LIBRARIES}")
append_RAPIO("${Boost_INCLUDE_DIRS}" "${Boost_LIBRARIES}")

# Use the generated RAPIO.h file
append_RAPIO("${CMAKE_CURRENT_BINARY_DIR}/base" "")
# Only would matter with third party install, but make it last
append_RAPIO("${CMAKE_INSTALL_PREFIX}/include" "")

# Debug look at the includes
#addRAPIONote("INCLUDE:\n ${RAPIO_INCLUDE_DIRS}\n\n")

# Create the RAPIO.h file (after all append_RAPIO_HEADER)
configure_file(base/RAPIO.h.in ${CMAKE_CURRENT_BINARY_DIR}/base/RAPIO.h @ONLY)

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
addRAPIONote("Network support (pulling webindex data):")
find_package(CURL) 
if (CURL_FOUND)
  add_definitions(-DHAVE_CURL)
  append_RAPIO("${CURL_INCLUDE_DIR}" "${CURL_LIBRARIES}")
  addRAPIONote("      Found: CURL")
else()
  addRAPIONote("      Missing: CURL.  Will use BOOST::ASIO network library for network URL (experimental).")
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

if (BUILD_EXAMPLES)
  add_subdirectory(rexample)
endif (BUILD_EXAMPLES)

###########################################################################
# Independent module building
#
addRAPIONote("File format support:")
if (BUILD_NETCDF_MODULE)
  add_subdirectory(modules/ionetcdf)
endif (BUILD_NETCDF_MODULE)

if (BUILD_HDF5_MODULE)
  add_subdirectory(modules/iohdf5)
endif (BUILD_HDF5_MODULE)

if (BUILD_IMAGE_MODULE)
  add_subdirectory(modules/ioimage)
endif (BUILD_IMAGE_MODULE)

if (BUILD_GDAL_MODULE)
  add_subdirectory(modules/iogdal)
endif (BUILD_GDAL_MODULE)

set(jasper OFF)
set(openjpeg OFF)
if (BUILD_GRIB2_MODULE)

  # Go ahead and die now...otherwise it will die when pulling external project
  find_package(PNG REQUIRED)

  # Ok, so we should use jasper or openjpeg but not both.
  # And jasper is deprecated is seems. Depending on OS you might have issues
  # and have to play with turning on one vs the other.

  find_package(OpenJPEG QUIET)
  if (OpenJPEG_FOUND)
    set(openjpeg ON)
    message(STATUS "Found OpenJPEG2 for NCEPGRIB2.")
    message(STATUS "INSIDE VALUE = ${openjpeg}")
  else()

    # Ok try to fall back to jasper
    find_package(Jasper QUIET)
    if (Jasper_FOUND)
      set(jasper ON)
      message(STATUS "Found Jasper for NCEPGRIB2 (if build fails you can install OpenJpeg2).")
    endif()
  endif()

  # Manual override if needed, only use one:
  #set(jasper ON)
  #set(openjpeg OFF)
endif (BUILD_GRIB2_MODULE)

# Make sure and set once to prevent rebuilding
set(USE_JASPER_NCEPLIBS ${jasper} CACHE BOOL "Use Jasper JPEG compression library")
set(USE_OPENJPEG_NCEPLIBS ${openjpeg} CACHE BOOL "Use OpenJPEG JPEG compression library")

if (BUILD_GRIB2_MODULE)
  #add_subdirectory(modules/iowgrib)
  add_subdirectory(modules/iogrib)  # deprecated
  if(jasper)
    addRAPIONote("          -->JPEG: Jasper")
  endif()
  if(openjpeg)
    addRAPIONote("          -->JPEG: OpenJPEG")
  endif()
endif (BUILD_GRIB2_MODULE)

if (BUILD_TEXT_MODULE)
  add_subdirectory(modules/iotext)
endif (BUILD_TEXT_MODULE)

# These two currently don't require any extra libraries, so we'll just
# build them.  Though python might at some point
add_subdirectory(modules/iopython)
add_subdirectory(modules/ioraw)

if (BUILD_HMRG_MODULE)
  add_subdirectory(modules/iohmrg)
endif (BUILD_HMRG_MODULE)

# Build subprograms.
add_subdirectory(programs)

# Tests are conditional.  We 'could' make a flag...here we just have to manual
# go into directory and make.  Not sure what the cmake community does yet
add_subdirectory(tests EXCLUDE_FROM_ALL)

if (BUILD_WEB_MODULE)
  ExternalProject_Add(
    rapioweb
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/webexample
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    # Always build (so if we change the webexample/* it will catch it)
    BUILD_ALWAYS 1
  )

  add_custom_target(buildweb
    COMMAND "ls -l"
    DEPENDS rapioweb
  )
  # Bad: add_subdirectory(webexample)
endif (BUILD_WEB_MODULE)

# Install the library and headers from the base library
# Supposedly best practice is to do it top level.
# GNU will pick lib or lib64 for the install folder depending on OS
install(TARGETS rapio
  EXPORT rapioTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}  # for static libs, if any
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # for Windows
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rapio
)


# ---------------------------------------------------
# Create cmake files for find_package
#
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/rapioConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/rapioConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rapio
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/rapioConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(EXPORT rapioTargets
  FILE rapioTargets.cmake
  NAMESPACE rapio::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rapio
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/rapioConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/rapioConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rapio
)
# ---------------------------------------------------

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
