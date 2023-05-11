# We wants headers in the current directory, even if bracketed. FIXME: Maybe we shouldn't bracket
# Get headers from core above us, or where ever RAPIO headers are
#include_directories(. ../ ${CMAKE_INSTALL_PREFIX}/include ${CMAKE_INSTALL_PREFIX}/include/rapiobase)
include_directories(. ../base)

# Not sure why we need this with cmake..we should be linking to rapio base
link_directories(../../lib)

# Need this to add the BOOST setting or the rError.cc will complain about v2
#add_definitions(-DBOOST_LOG_DYN_LINK)

# Build source as a shared library
enable_testing()

add_executable(testrapio
  rTestMain.cc
  rTestTime.cc
  rTestURL.cc
  rTestIODataType.cc
  rTestArray.cc
  rTestSparseVector.cc
)

# Link to other libraries
target_link_libraries(testrapio
  boost_unit_test_framework
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
