# Not sure why we need this with cmake..we should be linking to rapio base
#link_directories(../../lib)
# I want absolute library paths if I'm building with a custom prefix/third party so
# that I don't grab the system libraries by accident
cmake_policy(SET CMP0060 NEW)

link_directories(${CMAKE_INSTALL_PREFIX}/lib)

add_executable(rsimplealg
  rsimplealg.cc rSimpleAlg.cc
)
target_link_libraries(rsimplealg rapio)

add_executable(rtile
  rTile.cc
)
target_link_libraries(rtile rapio)

# Get custom headers from our installed stuff one folder up.  How to handle our local build vs system for hunting libraries?
include_directories(. ../../include ../base/)

# Need this to add the BOOST setting or the rError.cc will complain about v2
add_definitions(-DBOOST_LOG_DYN_LINK)

# Install the algorithms
install(TARGETS rsimplealg
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
install(TARGETS rtile
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

