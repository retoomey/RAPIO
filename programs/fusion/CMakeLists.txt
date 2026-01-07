##################################################################
# CMakeLists for building programs
#
##################################################################
# Shared code
add_library(fusionshared STATIC 
  rStage2Data.cc
  rFusionCache.cc
)
target_link_libraries(fusionshared PRIVATE rapio_interface)
target_include_directories(fusionshared PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

##################################################################
# Fusion1
add_executable(rFusion1
  rRobertLinear1Resolver.cc
  rLak2DResolver.cc
  rLakResolver1.cc
  rVelResolver.cc
  rLakRadialSmoother.cc
  rFusion1.cc
)
target_link_libraries(rFusion1 PRIVATE fusionshared rapio)
install(TARGETS rFusion1
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# Fusion2
add_executable(rFusion2
  rFusionDatabase.cc
  rFusion2.cc
)

target_link_libraries(rFusion2 PRIVATE fusionshared rapio)
install(TARGETS rFusion2
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# FusionAlgs
add_executable(rFusionAlgs
  rFusionAlgs.cc
)

target_link_libraries(rFusionAlgs PRIVATE fusionshared rapio)
install(TARGETS rFusionAlgs
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# FusionRoster
add_executable(rFusionRoster
  rFusionRoster.cc
)

target_link_libraries(rFusionRoster PRIVATE fusionshared rapio)
install(TARGETS rFusionRoster
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# TileJoin
add_executable(rTileJoin
  rTileJoin.cc
)

target_link_libraries(rTileJoin PRIVATE fusionshared rapio)
install(TARGETS rTileJoin
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# PointCloud
add_executable(rPointCloud1
  rPointCloud.cc
)

target_link_libraries(rPointCloud1 PRIVATE fusionshared rapio)
install(TARGETS rPointCloud1
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
