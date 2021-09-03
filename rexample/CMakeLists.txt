link_directories(${RAPIO_INSTALL_LIB})

add_executable(rsimplealg
  rsimplealg.cc rSimpleAlg.cc
)
target_link_libraries(rsimplealg rapio)

add_executable(rtile
  rTile.cc
)
target_link_libraries(rtile rapio)

include_directories(. ${CMAKE_INSTALL_PREFIX}/include  ../base/)

# Need this to add the BOOST setting or the rError.cc will complain about v2
add_definitions(-DBOOST_LOG_DYN_LINK)

# Install the algorithms
install(TARGETS rsimplealg
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
install(TARGETS rtile
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

