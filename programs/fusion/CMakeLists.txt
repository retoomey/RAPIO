##################################################################
# CMakeLists for building programs
#
link_directories(${RAPIO_INSTALL_LIB})
include_directories(. ${RAPIO_INCLUDE_DIRS})

##################################################################
# Shared code
add_library(fusionshared STATIC 
  rStage2Data.cc
  rFusionCache.cc
)

##################################################################
# Fusion1
add_executable(rFusion1
  rRobertLinear1Resolver.cc
  rLakResolver1.cc
  rVelResolver.cc
  rLakRadialSmoother.cc
  rFusion1.cc
)
target_link_libraries(rFusion1 PRIVATE fusionshared PUBLIC rapio)
install(TARGETS rFusion1
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# Fusion2
add_executable(rFusion2
  rFusionDatabase.cc
  rFusion2.cc
)

target_link_libraries(rFusion2 PRIVATE fusionshared PUBLIC rapio)
install(TARGETS rFusion2
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# FusionRoster
add_executable(rFusionRoster
  rFusionRoster.cc
)

target_link_libraries(rFusionRoster PRIVATE fusionshared PUBLIC rapio)
install(TARGETS rFusionRoster
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# PointCloud
add_executable(rPointCloud1
  rPointCloud.cc
)

target_link_libraries(rPointCloud1 PRIVATE fusionshared PUBLIC rapio)
install(TARGETS rPointCloud1
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
