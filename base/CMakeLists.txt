# Toomey Sept 2021
# RAPIO base cmake file
#

# Set variable of all sources we use
# ls -1 *.c *.cc
# We 'could' generate this, but I feel
# explicitly defining what to compile is
# probably better. We do generate headers
# below though.
set(SOURCE_FILES
ccronexpr.c
rAlgConfigFile.cc
rArray.cc
rArrayAlgorithm.cc
rHelpFormatter.cc
image/rArraySampler.cc
image/rNearestNeighbor.cc
image/rCressman.cc
image/rBilinear.cc
image/rThresholdFilter.cc
image/rPercentFilter.cc
datatype/rAttributeDataType.cc
rAWS.cc
rBimap.cc
rBinaryIO.cc
datatype/rBinaryTable.cc
rBitset.cc
rBoostConnection.cc
rColor.cc
rColorMap.cc
rColorTerm.cc
config/rConfig.cc
config/rConfigColorMap.cc
config/rConfigDirectoryMapping.cc
config/rConfigIODataType.cc
config/rConfigLogging.cc
config/rConfigParamGroup.cc
config/rConfigRadarInfo.cc
config/rConfigRecord.cc
rConstants.cc
rCurlConnection.cc
rDataArray.cc
rDataFilter.cc
datatype/rDataGrid.cc
rDataProjection.cc
datatype/rMergerBinaryTable.cc
datatype/rMultiDataType.cc
datatype/rDataTable.cc
datatype/rDataType.cc
rDataTypeHistory.cc
rDimensionMapper.cc
rDirWalker.cc
watcher/rDirWatcher.cc
rElevationVolume.cc
rError.cc
rEventLoop.cc
rEventTimer.cc
notifier/rEXERecordNotifier.cc
notifier/rRedisRecordNotifier.cc
watcher/rEXEWatcher.cc
watcher/rFAMWatcher.cc
index/rFakeIndex.cc
index/rFileIndex.cc
index/rFMLIndex.cc
index/rRedisIndex.cc
notifier/rFMLRecordNotifier.cc
datatype/rFusionBinaryTable.cc
datatype/rGDALDataType.cc
datatype/rGribDataType.cc
rHeartbeat.cc
rIJK.cc
datatype/rImageDataType.cc
index/rIndexType.cc
rIODataType.cc
rIOFakeDataGenerator.cc
rIOFakeDataType.cc
rIOFile.cc
rIOPostProcessor.cc
index/rIOIndex.cc
rIOJSON.cc
rIOURL.cc
watcher/rIOWatcher.cc
watcher/rRedisWatcher.cc
rIOXML.cc
datatype/rLatLonArea.cc
datatype/rLatLonGrid.cc
datatype/rLatLonGridProjection.cc
datatype/rLatLonGridIterator.cc
datatype/rLatLonHeightGrid.cc
datatype/rLatLonHeightGridIterator.cc
datatype/rLatLonHeightGridProjection.cc
rLength.cc
rLogger.cc
rLL.cc
rLLCoverageArea.cc
rLLH.cc
datatype/rLLHGridN2D.cc
rNamedAny.cc
datatype/rNetcdfDataType.cc
rNetwork.cc
rOption.cc
rOptionList.cc
rOS.cc
rPartitionInfo.cc
rPolarAlgorithm.cc
rProcessTimer.cc
rProject.cc
datatype/rPTreeData.cc
datatype/rRadialSet.cc
datatype/rRadialSetIterator.cc
datatype/rRadialSetProjection.cc
rRAPIOAlgorithm.cc
rRAPIOData.cc
rRAPIOOptions.cc
rRAPIORuntime.cc
plugin/rRAPIOPlugin.cc
plugin/rPluginHeartbeat.cc
plugin/rPluginWebserver.cc
plugin/rPluginNotifier.cc
plugin/rPluginIngestor.cc
plugin/rPluginRealTime.cc
plugin/rPluginRecordFilter.cc
plugin/rPluginProductOutput.cc
plugin/rPluginProductOutputFilter.cc
plugin/rPluginVolumeValueResolver.cc
plugin/rPluginTerrainBlockage.cc
plugin/rPluginVolume.cc
plugin/rPluginPartition.cc
rRAPIOProgram.cc
rRecord.cc
rRecordFilter.cc
notifier/rRecordNotifier.cc
rRecordQueue.cc
rSecure.cc
rSignals.cc
rSparseVector.cc
index/rStreamIndex.cc
rStrings.cc
rTerrainBlockage2me.cc
rTerrainBlockage.cc
rTerrainBlockageLak.cc
rTime.cc
rThreadGroup.cc
rTimeDuration.cc
rUnit.cc
rURL.cc
rValueCompressor.cc
rVolumeAlgorithm.cc
rVolumeValueResolver.cc
index/rWebIndex.cc
watcher/rWebIndexWatcher.cc
rWebServer.cc
index/rXMLIndex.cc
rXYZ.cc
)

# Glob all headers in the current directory and subdirectories
# Note: Using RELATIVE helps keep the install paths clean
file(GLOB_RECURSE ALL_HEADERS_ON_DISK 
     RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
     "*.h")

# Maintain a "Blacklist" of headers that should NEVER be installed
set(PRIVATE_HEADERS
    "rapio_pch.h"
    "test_internal.h"   # Example of a temp/internal header
)

# Generate the actual install list by removing the blacklist from the glob
set(RELATIVE_HEADERS ${ALL_HEADERS_ON_DISK})
if(PRIVATE_HEADERS)
    list(REMOVE_ITEM RELATIVE_HEADERS ${PRIVATE_HEADERS})
endif()

##################################################################################################
# Target: Header only interface for modules, doesn't link to the librapio
#
add_library(rapio_interface INTERFACE)

# Resolve the ../webserver path to an absolute path
get_filename_component(RAPIO_WEBSERVER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../webserver" ABSOLUTE)

# Declare the RAPIO headers for installation
target_include_directories(rapio_interface INTERFACE

  # Headers during build.  We flatten them because we want
  # the ability to change the organization without doing
  #  say #include "config/file.h"
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/config
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/datatype
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/index
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/notifier
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/plugin
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/watcher
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/image

  # Web server files
  $<BUILD_INTERFACE:${RAPIO_WEBSERVER_DIR}>

  # Get the generated config.h
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>

  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDE_DIR}/rapio>
)

# Definition for rapio_interface
target_compile_definitions(rapio_interface INTERFACE BOOST_LOG_DYN_LINK BOOST_STACKTRACE_DYN_LINK)

# header interface has fmt now
target_link_libraries(rapio_interface INTERFACE rapio_fmt_interface)

##################################################################################################
# Target: Create RAPIO shared library (for things linking to rapio code)
#
add_library(rapio SHARED
    "${SOURCE_FILES}"
)

# Inject the CMake build type into the C++ preprocessor
target_compile_definitions(rapio PRIVATE RAPIO_BUILD_TYPE="${CMAKE_BUILD_TYPE}")

target_link_libraries(rapio
  # Go ahead and make our linked libraries public for now.  In theory,
  # would be better some private
  PUBLIC rapio_interface # Inherit the interface headers
  # Explicitly add public dlsys for any binaries c
  # calling dynamic loading methods.  rMakeIndex calls factory creation
  PUBLIC dl
)

##################################################################################################
# Precompile some 3rd party for speed
#
if(RAPIO_USE_PCH)
  if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.0")
    target_precompile_headers(rapio PRIVATE 
      $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_SOURCE_DIR}/rapio_pch.h">
    )
    message(STATUS "Precompiled Headers (PCH) Enabled for rapio target")
  else()
    message(WARNING "RAPIO_USE_PCH is ON, but CMake version is < 3.16. PCH disabled.")
  endif()
else()
  message(STATUS "Precompiled Headers (PCH) Disabled")
endif()

##################################################################################################
# Link libraries to interface and shared targets
#

# BOOST is the big one.
# Pretty sure boost links us to Threads::Threads, but ensure pthreads first
set(THREADS_PREFER_PTHREAD_FLAG ON)
#find_package(Threads REQUIRED) target_link_libraries(binary Threads::Threads)


# Add a dropdown option for the stacktrace backend (defaulting to 'basic')
set(RAPIO_STACKTRACE_BACKEND "basic" CACHE STRING "Boost.Stacktrace backend to use (basic, addr2line, noop)")
set_property(CACHE RAPIO_STACKTRACE_BACKEND PROPERTY STRINGS "basic" "addr2line" "noop")

# Construct the exact component name CMake needs to look for
set(STACKTRACE_COMPONENT "stacktrace_${RAPIO_STACKTRACE_BACKEND}")

# Pull in Boost, appending the dynamically selected stacktrace component
find_package(Boost 1.66 REQUIRED COMPONENTS
  system log log_setup thread filesystem serialization iostreams timer ${STACKTRACE_COMPONENT}
)

#find_package(Boost 1.66 REQUIRED COMPONENTS
#  system log log_setup thread filesystem serialization iostreams timer stacktrace_basic
#)

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

#target_compile_definitions(rapio_interface INTERFACE HAVE_CURL)
target_include_directories(rapio_interface INTERFACE ${Boost_INCLUDE_DIRS})
target_link_libraries(rapio PUBLIC ${Boost_LIBRARIES})

# Used by the base/rUnits for conversion, but we don't convert right now anyway
addRAPIONote("Utility libraries:")

# Look for redis optional for the redis index,watcher,notifier
find_path(HIREDIS_INCLUDE_DIR NAMES hiredis/hiredis.h)
find_library(HIREDIS_LIB NAMES hiredis)
if(HIREDIS_INCLUDE_DIR AND HIREDIS_LIB)
  addRAPIONote("      Found: REDIS")
  target_include_directories(rapio_interface INTERFACE ${HIREDIS_INCLUDE_DIRS})
  target_link_libraries(rapio PRIVATE ${HIREDIS_LIB})
  target_compile_definitions(rapio PUBLIC HAVE_HIREDIS)
else()
  addRAPIONote("      Missing: REDIS. Redis index, notifier, watcher disabled")
endif()

find_package(CURL) 
if (CURL_FOUND)
  target_compile_definitions(rapio_interface INTERFACE HAVE_CURL)
  target_include_directories(rapio_interface INTERFACE ${CURL_INCLUDE_DIR})
  target_link_libraries(rapio PRIVATE ${CURL_LIBRARIES})
  addRAPIONote("      Found: CURL")
else()
  addRAPIONote("      Missing: CURL.  Will use BOOST::ASIO network library for network URL (experimental).")
endif(CURL_FOUND)

find_package(UDUnits2)
if (UDUnits2_FOUND)
  target_compile_definitions(rapio_interface INTERFACE HAVE_UDUNITS2)
  target_include_directories(rapio_interface INTERFACE ${UDUnits2_INCLUDE_DIR})
  target_link_libraries(rapio PRIVATE ${UDUnits2_LIBRARIES})
  addRAPIONote("      Found: UDUnits2")
else()
  addRAPIONote("      Missing: UDUnits2. Unit conversion ability disabled.  Not used 'yet'.")
endif(UDUnits2_FOUND)

# Used by AWS stuff (optional)
find_package(OpenSSL)
if (OPENSSL_FOUND)
  target_compile_definitions(rapio_interface INTERFACE HAVE_OPENSSL)
  target_include_directories(rapio_interface INTERFACE ${OPENSSL_INCLUDE_DIR})

  # Public for openssl because libcurl links to it and curl target isn't working
  # Kinda annoying curl seems to link to everything, bloats code a bit.
  # Considering other libraries, perhaps cpp-httplib
  target_link_libraries(rapio PUBLIC ${OPENSSL_LIBRARIES})
  #target_link_libraries(rapio PUBLIC OpenSSL:SSL OpenSSL::Crypto)
  addRAPIONote("      Found: OpenSSL")
else()
  addRAPIONote("      Missing: OpenSSL. Only matters for rAWS.cc and web AWS stuff (experimental).")
endif(OPENSSL_FOUND)

find_package(Proj)
if (Proj_FOUND)
  target_compile_definitions(rapio_interface INTERFACE HAVE_PROJLIB)
  target_include_directories(rapio_interface INTERFACE ${Proj_INCLUDE_DIR})
  target_link_libraries(rapio PRIVATE ${Proj_LIBRARIES})
  addRAPIONote("      Found: Proj")
else() 
  addRAPIONote("      Missing: Proj. Needed for some projections.")
endif(Proj_FOUND)

addRAPIONote("Compression support:")

find_package(ZLIB REQUIRED)
if (ZLIB_FOUND)
  target_include_directories(rapio_interface INTERFACE ${ZLIB_INCLUDE_DIRS})
  target_link_libraries(rapio PRIVATE ${ZLIB_LIBRARIES})
  addRAPIONote("      Found: (.z) ZLIB")
else()
  addRAPIONote("      Missing: ZLIB")
endif(ZLIB_FOUND)

if (BUILD_MRMS)
  find_package(BZip2 REQUIRED)
else()
  find_package(BZip2 QUIET)
endif(BUILD_MRMS)
if (BZip2_FOUND)
  target_include_directories(rapio_interface INTERFACE ${BZip2_INCLUDE_DIRS})
  target_link_libraries(rapio PRIVATE ${BZip2_LIBRARIES})
  addRAPIONote("      Found: (.bz2) BZIP2")
else()
  addRAPIONote("      Missing: (.bz2) BZIP2")
endif(BZip2_FOUND)

# Lzma support in boost
if (${Boost_VERSION} VERSION_GREATER_EQUAL ${TEST_BOOST_VERSION})
  add_definitions(-DHAVE_LZMA)
  addRAPIONote("      Found: (.xz, .lzma) LZMA")
else()
  addRAPIONote("      Missing: (.xz, .lzma) LZMA")
endif()

# Snappy in Oracle 9 at least
find_package(Snappy QUIET)
if (Snappy_FOUND)
  # At least on oracle 9 it's not setting the libraries/headers.  It should 'normally' be
  # just snappy, so we'll do that for now.
  target_compile_definitions(rapio_interface INTERFACE HAVE_SNAPPY)
  target_include_directories(rapio_interface INTERFACE ${Snappy_INCLUDE_DIRS})
  set(Snappy_LIBRARIES "-lsnappy")
  target_link_libraries(rapio PRIVATE ${Snappy_LIBRARIES})
  addRAPIONote("      Found: (.sz) Google Snappy ${Snappy_VERSION}")
else()
  addRAPIONote("      Missing: (.sz) Google Snappy")
endif(Snappy_FOUND)

##################################################################################################
# Flatten headers in the install
#

foreach(hdr IN LISTS RELATIVE_HEADERS)
  list(APPEND HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${hdr}")
endforeach()
# Add the webserver header from webserver
list(APPEND HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../webserver/server_http.hpp")
list(APPEND HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../webserver/asio_compatibility.hpp")
list(APPEND HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../webserver/mutex.hpp")
list(APPEND HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../webserver/utility.hpp")
list(APPEND HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../webserver/status_code.hpp")

# Add the created headers
list(APPEND HEADER_FILES "${CMAKE_CURRENT_BINARY_DIR}/RAPIO.h")
list(APPEND HEADER_FILES "${CMAKE_CURRENT_BINARY_DIR}/config.h")

set_target_properties(rapio PROPERTIES
  PUBLIC_HEADER "${HEADER_FILES}"
)
