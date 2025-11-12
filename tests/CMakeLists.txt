# We wants headers in the current directory, even if bracketed. FIXME: Maybe we shouldn't bracket
# Get headers from core above us, or where ever RAPIO headers are
include_directories(. ${RAPIO_INCLUDE_DIRS})

# Not sure why we need this with cmake..we should be linking to rapio base
link_directories(../../lib)

# Need this to add the BOOST setting or the rError.cc will complain about v2
#add_definitions(-DBOOST_LOG_DYN_LINK)

# Build source as a shared library
enable_testing()

# Guard against multiple inclusions or redundant setup
if(NOT TARGET Boost::unit_test_framework)
  find_package(Boost 1.66 REQUIRED COMPONENTS unit_test_framework)
endif()

add_executable(testrapio
  rTestArray.cc
  rTestBimap.cc
  rTestBinaryIO.cc
  rTestBitset.cc
  rTestGrid.cc
  rTestIODataType.cc
  rTestMain.cc
  rTestSparseVector.cc
  rTestTime.cc
  rTestNetwork.cc
  rTestURL.cc
  rTestValueCompressor.cc
  rTestStage2Data.cc
  ../programs/fusion/rStage2Data.cc
)

target_link_libraries(testrapio PRIVATE
  Boost::unit_test_framework
  rapio
)

add_executable(rTestNetcdf
  rTestNetcdf.cc
)

# Link to other libraries
target_link_libraries(rTestNetcdf
  rapio
)

# Files we need in the build folder
configure_file(runtests.sh runtests.sh)
configure_file(testxml.xml testxml.xml)
configure_file(testjson.json testjson.json)

add_test(NAME RAPIOTESTS COMMAND testrapio --log_level=test_suite)
