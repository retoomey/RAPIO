# We wants headers in the current directory, even if bracketed. FIXME: Maybe we shouldn't bracket
# Get headers from core above us, or where ever RAPIO headers are
include_directories(. ${RAPIO_INCLUDE_DIRS})

# Not sure why we need this with cmake..we should be linking to rapio base
link_directories(../../lib)

# Guard against multiple inclusions or redundant setup
if(NOT TARGET Boost::unit_test_framework)
  find_package(Boost 1.66 REQUIRED COMPONENTS unit_test_framework)
endif()

# Define all standalone tests (rTestMain.cc is removed)
set(RAPIO_TESTS
  rTestArray
  rTestBimap
  rTestBinaryIO
  rTestBitset
  rTestColorMap
  rTestFactory
  rTestGrid
  rTestHeartbeat
  rTestIODataType
  rTestIOPostProcessor
  rTestPipeline
  rTestOptions
  rTestRandomForest
  rTestSparseVector
  rTestSparse
  rTestTileJoin
  rTestTime
  rTestNetwork
  rTestURL
  rTestValueCompressor
)

# Loop through and create an executable/test target for each standalone file
foreach(TEST_NAME IN LISTS RAPIO_TESTS)
  add_executable(${TEST_NAME} ${TEST_NAME}.cc)
  target_link_libraries(${TEST_NAME} PRIVATE
    Boost::unit_test_framework
    rapio
    ZLIB::ZLIB
  )
  addRAPIOTest(${TEST_NAME} COMMAND ${TEST_NAME} --log_level=test_suite)
endforeach()

# Special case for rTestStage2Data since it compiles an extra source file
add_executable(rTestStage2Data
  rTestStage2Data.cc
  ../programs/fusion/rStage2Data.cc
)
target_link_libraries(rTestStage2Data PRIVATE
  Boost::unit_test_framework
  rapio
  ZLIB::ZLIB
)
addRAPIOTest(rTestStage2Data COMMAND rTestStage2Data --log_level=test_suite)

# Files we need in the build folder
configure_file(testxml.xml testxml.xml)
configure_file(testjson.json testjson.json)
