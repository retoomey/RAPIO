##################################################################
# CMakeLists for building programs
#
link_directories(${RAPIO_INSTALL_LIB})
include_directories(. ${RAPIO_INCLUDE_DIRS})

# Need this to add the BOOST setting or the rError.cc will complain about v2
# FIXME: not sure needed anymore
add_definitions(-DBOOST_LOG_DYN_LINK)

##################################################################
# Tools
add_executable(rdump
  rDump.cc
)
target_link_libraries(rdump rapio)
install(TARGETS rdump
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
add_executable(rcopy
  rCopy.cc
)
target_link_libraries(rcopy rapio)
install(TARGETS rcopy
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
add_executable(rwatch
  rWatch.cc
)
target_link_libraries(rwatch rapio)
install(TARGETS rwatch
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# Shared code
add_library(fusionshared STATIC 
  fusion/rStage2Data.cc
  fusion/rFusionCache.cc
)

##################################################################
# Fusion1
add_executable(rFusion1
  fusion/rRobertLinear1Resolver.cc
  fusion/rLakResolver1.cc
  fusion/rLakRadialSmoother.cc
  fusion/rFusion1.cc
)
target_link_libraries(rFusion1 PRIVATE fusionshared PUBLIC rapio)
install(TARGETS rFusion1
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# Fusion2
add_executable(rFusion2
  fusion/rFusionDatabase.cc
  fusion/rFusion2.cc
)

target_link_libraries(rFusion2 PRIVATE fusionshared PUBLIC rapio)
install(TARGETS rFusion2
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# FusionRoster
add_executable(rFusionRoster
  fusion/rFusionRoster.cc
)

target_link_libraries(rFusionRoster PRIVATE fusionshared PUBLIC rapio)
install(TARGETS rFusionRoster
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
# Grib2Reader 
add_executable(rGrib2Reader
	rGrib2Reader.cc
)
target_link_libraries(rGrib2Reader rapio)
install(TARGETS rGrib2Reader
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
