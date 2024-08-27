##################################################################
# Build utilities
#
link_directories(${RAPIO_INSTALL_LIB})
include_directories(. ${RAPIO_INCLUDE_DIRS})

##################################################################
# RESTServer for tile generation 
add_executable(rtile
  rTile.cc
)
target_link_libraries(rtile rapio)
install(TARGETS rtile
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# Legend generator (SVG)
#
add_executable(rGetLegend
  rGetLegend.cc
)
target_link_libraries(rGetLegend rapio)
install(TARGETS rGetLegend
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# Colormap generator (JSON)
#
add_executable(rGetColorMap
  rGetColorMap.cc
)
target_link_libraries(rGetColorMap rapio)
install(TARGETS rGetColorMap
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
