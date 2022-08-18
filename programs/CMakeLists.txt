##################################################################
# CMakeLists for building programs
#
link_directories(${RAPIO_INSTALL_LIB})
include_directories(. ${CMAKE_INSTALL_PREFIX}/include  ../base/)

# Need this to add the BOOST setting or the rError.cc will complain about v2
# FIXME: not sure needed anymore
add_definitions(-DBOOST_LOG_DYN_LINK)

##################################################################
# Fusion1
add_executable(rFusion1
  fusion/rFusion1.cc
)
target_link_libraries(rFusion1 rapio)
install(TARGETS rFusion1
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# MakeFakeRadarData 
add_executable(rMakeFakeRadarData
  fusion/rMakeFakeRadarData.cc
)
target_link_libraries(rMakeFakeRadarData rapio)
install(TARGETS rMakeFakeRadarData
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# RESTServer for tile generation 
add_executable(rtile
  RESTserver/rTile.cc
)
target_link_libraries(rtile rapio)
install(TARGETS rtile
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)