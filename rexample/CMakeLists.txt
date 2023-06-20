##################################################################
# CMakeLists for building this subdirectory of examples
#
link_directories(${RAPIO_INSTALL_LIB})
include_directories(. ${RAPIO_INCLUDE_DIRS})

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
# The grib example 
add_executable(rgribexample
  rGribExample.cc
)
target_link_libraries(rgribexample rapio)
install(TARGETS rgribexample
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
