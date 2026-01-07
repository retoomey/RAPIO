# Toomey Aug 2021
# Top cmake for RAPIO building
cmake_minimum_required (VERSION 3.10)

project (RAPIO 
    DESCRIPTION "RAPIO Real time algorithm parameter and IO"
    HOMEPAGE_URL "https://github.com/retoomey/RAPIO"
    LANGUAGES CXX C
    VERSION 1.0)

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
option(BUILD_NIDS_MODULE "Build NIDS(LevelIII) module" YES)
option(BUILD_HDF5_MODULE "Build general hdf5 module" YES)
option(BUILD_HMRG_MODULE "Build HMRG module (reading MRMS binary grids)" YES)

option(BUILD_IMAGE_MODULE "Build image (GraphicsMagick) module" YES)

option(BUILD_GDAL_MODULE "Build GDAL module" YES)
option(BUILD_GRIB2_MODULE "Build Grib2 module (reading grib2 data)" YES)
option(USE_NCEPLIBS_GRIB2 "Use NCEPLIBS-g2c library instead of system grib2" YES)
option(BUILD_TEXT_MODULE "Build Text module (writing text DataTypes)" YES)

# This option is for validating all symbols in modules and that 'enough' libraries
# have been linked. To reduce size, compile time and duplication we want this off,
# but if a module say gets an undefined symbol when running, we can turn this on to
# see what we're missing and then fix our CMakeLists.txt for the module.
option(RAPIO_STRICT_SYMBOL_CHECK "Link modules to librapio to verify all symbols" OFF)

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

# Specify the C++ standard
# Currently tested from 11 to 23. Will avoid new features for now
# for legacy compatibility.
set(CMAKE_CXX_STANDARD 11)
# Allow it to go higher to compiler default
set(CMAKE_CXX_STANDARD_REQUIRED False)

###########################################################################
# --- Fetch and configure fmt library ---
# We have to support older c++ versions, std::format would be nice though
#
include(FetchContent)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        12.1.0 # Stable version
)

FetchContent_MakeAvailable(fmt)
add_library(rapio_fmt_interface INTERFACE)
target_link_libraries(rapio_fmt_interface INTERFACE fmt::fmt-header-only)
###########################################################################

# A function to do the basics of a dynamic module setup
# This doesn't link extra libraries per modules, but the basics for rapio
function(addRAPIOModule MODULE_NAME)
    # The first argument is the module name, everything after are the source files
    set(SOURCE_FILES ${ARGN})

    # 1. Create the library
    add_library(${MODULE_NAME} SHARED ${SOURCE_FILES})

    # 2. Basic target requirements
    target_link_libraries(${MODULE_NAME} PRIVATE rapio_interface)
    target_include_directories(${MODULE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    # 3. Apply the Strict Symbol Check logic globally
    if(RAPIO_STRICT_SYMBOL_CHECK)
        if(NOT APPLE)
            target_link_options(${MODULE_NAME} PRIVATE "-Wl,--no-undefined")
        endif()
        target_link_libraries(${MODULE_NAME} PRIVATE rapio)
    endif()

    # 4. Standard Installation
    install(TARGETS ${MODULE_NAME}
            DESTINATION ${CMAKE_INSTALL_LIBDIR})
            
    message(STATUS "Configured RAPIO module: ${MODULE_NAME}")
endfunction()

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

endfunction()

# Add .h files in a folder to the RAPIO.h and includes
function(append_RAPIO_HEADERS_GLOB_H FOLDER)
  set(PUBLIC_HEADER_BASE "${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER}")
  file(GLOB FILE_LIST RELATIVE ${PUBLIC_HEADER_BASE} ${PUBLIC_HEADER_BASE}*.h)
  append_RAPIO_HEADERS("${FOLDER}" "${FILE_LIST}")
  set(PUBLIC_HEADERS ${PUBLIC_HEADERS} PARENT_SCOPE)
  #set(RAPIO_INCLUDE_DIRS "${RAPIO_INCLUDE_DIRS}" PARENT_SCOPE)
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
endif()

if (BUILD_MRMS)
  addRAPIONote("Stricter build requirements for MRMS Environment: ENABLED")
else()
  addRAPIONote("Stricter build requirements for MRMS Environment: ***DISABLED***")
endif()

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

# Create the RAPIO.h file (after all append_RAPIO_HEADER)
configure_file(base/RAPIO.h.in ${CMAKE_CURRENT_BINARY_DIR}/base/RAPIO.h @ONLY)

###########################################################################
# Directories
add_subdirectory(base)

if (BUILD_EXAMPLES)
  add_subdirectory(rexample)
endif (BUILD_EXAMPLES)

###########################################################################
# Independent module building
#
addRAPIONote("Loggers:")
# We could add different logging systems
add_subdirectory(modules/logspd)

addRAPIONote("File format support:")
if (BUILD_NETCDF_MODULE)
  add_subdirectory(modules/ionetcdf)
endif (BUILD_NETCDF_MODULE)

if (BUILD_NIDS_MODULE)
  add_subdirectory(modules/ionids)
endif (BUILD_NIDS_MODULE)

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
  add_subdirectory(modules/iowgrib)
  #add_subdirectory(modules/iogrib)  # deprecated
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
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/programs/RESTserver
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    # Always build (so if we change the RESTserver/* it will catch it)
    BUILD_ALWAYS 1
  )

  add_custom_target(buildweb
    COMMAND "ls -l"
    DEPENDS rapioweb
  )
  # Bad: add_subdirectory(RESTserver)
endif (BUILD_WEB_MODULE)

# Install the library and headers from the base library
# Supposedly best practice is to do it top level.
# GNU will pick lib or lib64 for the install folder depending on OS
install(TARGETS rapio rapio_interface rapio_fmt_interface fmt-header-only
  EXPORT rapioTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}  # for static libs, if any
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # for Windows
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rapio
)
install(DIRECTORY "${fmt_SOURCE_DIR}/include/fmt"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/rapio"
        FILES_MATCHING PATTERN "*.h")

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
