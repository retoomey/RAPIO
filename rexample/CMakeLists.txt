##################################################################
# CMakeLists for building this subdirectory of examples
#
link_directories(${RAPIO_INSTALL_LIB})
include_directories(. ${CMAKE_INSTALL_PREFIX}/include  ../base/)

# Need this to add the BOOST setting or the rError.cc will complain about v2
# FIXME: not sure needed anymore
add_definitions(-DBOOST_LOG_DYN_LINK)

##################################################################
# The simple algorithm example
add_executable(rsimplealg
  rSimpleAlg.cc
)
target_link_libraries(rsimplealg rapio)
install(TARGETS rsimplealg
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# The simple tool/executable example
add_executable(rsimpleprogram
  rSimpleProgram.cc
)
target_link_libraries(rsimpleprogram rapio)
install(TARGETS rsimpleprogram
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# The tile experiment. This will probably move somewhere else
# since if we develop this it will grow to decent size
add_executable(rtile
  rTile.cc
)
target_link_libraries(rtile rapio)
install(TARGETS rtile
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
