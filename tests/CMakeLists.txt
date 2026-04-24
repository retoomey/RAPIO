# We wants headers in the current directory, even if bracketed. FIXME: Maybe we shouldn't bracket
# Get headers from core above us, or where ever RAPIO headers are
include_directories(. ${RAPIO_INCLUDE_DIRS})

# Not sure why we need this with cmake..we should be linking to rapio base
link_directories(../../lib)

# Need this to add the BOOST setting or the rError.cc will complain about v2
#add_definitions(-DBOOST_LOG_DYN_LINK)

# Guard against multiple inclusions or redundant setup
if(NOT TARGET Boost::unit_test_framework)
  find_package(Boost 1.66 REQUIRED COMPONENTS unit_test_framework)
endif()

add_executable(rTestRAPIO
  rTestArray.cc
  rTestBimap.cc
  rTestBinaryIO.cc
  rTestBitset.cc
  rTestFactory.cc
  rTestGrid.cc
  rTestIODataType.cc
  rTestIOPostProcessor.cc
  rTestMain.cc
  rTestOptions.cc
  rTestSparseVector.cc
  rTestTime.cc
  rTestNetwork.cc
  rTestURL.cc
  rTestValueCompressor.cc
  rTestStage2Data.cc
# Not 100% sure where to put alg tests. Right now if
# it uses boost test putting here. If it's
# integration or main put with the code as
# unique ctest -N
  rTestTileJoin.cc
  ../programs/fusion/rStage2Data.cc
)

target_link_libraries(rTestRAPIO PRIVATE
  Boost::unit_test_framework
  rapio
  ZLIB::ZLIB
)

# Files we need in the build folder
configure_file(testxml.xml testxml.xml)
configure_file(testjson.json testjson.json)

add_test(NAME rTestRAPIO COMMAND rTestRAPIO --log_level=test_suite)
