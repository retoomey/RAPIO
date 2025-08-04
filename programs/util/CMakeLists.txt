##################################################################
# Build utilities
#
link_directories(${RAPIO_INSTALL_LIB})
include_directories(. ${RAPIO_INCLUDE_DIRS})

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

add_executable(rRemap
  rRemap.cc
)
target_link_libraries(rRemap rapio)
install(TARGETS rRemap
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

add_executable(rMakeIndex
  rMakeIndex.cc
)
target_link_libraries(rMakeIndex rapio)
install(TARGETS rMakeIndex
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# MakeFakeRadarData 
# FIXME: Not sure if we're keeping this. I'm experimenting with
# a fakeIndex generator API which may be better
add_executable(rMakeFakeRadarData
  rMakeFakeRadarData.cc
)
target_link_libraries(rMakeFakeRadarData rapio)
install(TARGETS rMakeFakeRadarData
   DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

##################################################################
# Single tile generation 
#
add_executable(rGetTile
  rGetTile.cc
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

# Install the makeindex script
install_python("makeindex.py" ${CMAKE_CURRENT_SOURCE_DIR})

