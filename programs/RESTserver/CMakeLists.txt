##################################################################
# Build utilities
#
link_directories(${RAPIO_INSTALL_LIB})
include_directories(. ${RAPIO_INCLUDE_DIRS})

##################################################################
# Single tile generation 
#
add_executable(rGetTile
  rTile.cc
)
target_link_libraries(rGetTile rapio)
install(TARGETS rGetTile
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

##################################################################
# Simple GUI experimental REST web server 
# A quick tool to allow quick viewing of local files
#
add_executable(rWebGUI
  rWebGUI.cc
)
target_link_libraries(rWebGUI rapio)
install(TARGETS rWebGUI
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
